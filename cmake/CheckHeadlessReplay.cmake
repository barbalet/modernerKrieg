if(NOT DEFINED MK_HEADLESS_RUN)
  message(FATAL_ERROR "MK_HEADLESS_RUN is required")
endif()

if(NOT DEFINED MK_REPLAY_PATH)
  message(FATAL_ERROR "MK_REPLAY_PATH is required")
endif()

if(NOT DEFINED MK_REPLAY_VALIDATE)
  message(FATAL_ERROR "MK_REPLAY_VALIDATE is required")
endif()

file(REMOVE "${MK_REPLAY_PATH}")
execute_process(
  COMMAND "${MK_HEADLESS_RUN}" --ai-only --max-ticks 3 --quiet --aar --replay "${MK_REPLAY_PATH}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "mk_headless_run failed: ${error}\n${output}")
endif()

if(NOT EXISTS "${MK_REPLAY_PATH}")
  message(FATAL_ERROR "replay file was not created: ${MK_REPLAY_PATH}")
endif()

file(READ "${MK_REPLAY_PATH}" replay)
foreach(required
    "mk_replay version=1"
    "event tick=0 kind=start"
    "event tick=1 kind=unit"
    "event tick=3 kind=score"
    "event tick=3 kind=end")
  string(FIND "${replay}" "${required}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "missing replay record: ${required}")
  endif()
endforeach()

execute_process(
  COMMAND "${MK_REPLAY_VALIDATE}" --quiet --expect-result MK_OK --expect-outcome failure "${MK_REPLAY_PATH}"
  RESULT_VARIABLE validate_result
  OUTPUT_VARIABLE validate_output
  ERROR_VARIABLE validate_error
)

if(NOT validate_result EQUAL 0)
  message(FATAL_ERROR "mk_replay_validate rejected generated replay: ${validate_error}\n${validate_output}")
endif()

execute_process(
  COMMAND "${MK_REPLAY_VALIDATE}" --playback --from-tick 2 --to-tick 3 --expect-result MK_OK --expect-outcome failure "${MK_REPLAY_PATH}"
  RESULT_VARIABLE playback_result
  OUTPUT_VARIABLE playback_output
  ERROR_VARIABLE playback_error
)

if(NOT playback_result EQUAL 0)
  message(FATAL_ERROR "mk_replay_validate failed replay playback: ${playback_error}\n${playback_output}")
endif()

foreach(required_playback
    "replay tick=2"
    "replay tick=3"
    "score=-93"
    "outcome=failure")
  string(FIND "${playback_output}" "${required_playback}" found_playback)
  if(found_playback EQUAL -1)
    message(FATAL_ERROR "missing replay playback output: ${required_playback}\n${playback_output}")
  endif()
endforeach()

set(invalid_replay_path "${MK_REPLAY_PATH}.invalid")
file(WRITE "${invalid_replay_path}"
  "mk_replay version=1 scenario=\"Invalid\" seed=1 steps=1 ai_only=1\n"
  "event tick=0 kind=start units=1 objectives=1 contacts=0\n"
  "event tick=1 kind=unit id=1 side=player order=hold status=ready x=0 y=0 target_x=0 target_y=0 has_target=0 hidden=0 revealed=0\n"
  "event tick=1 kind=end result=MK_OK score=0 outcome=failure\n"
)

execute_process(
  COMMAND "${MK_REPLAY_VALIDATE}" --quiet "${invalid_replay_path}"
  RESULT_VARIABLE invalid_result
  OUTPUT_VARIABLE invalid_output
  ERROR_VARIABLE invalid_error
)

if(invalid_result EQUAL 0)
  message(FATAL_ERROR "mk_replay_validate accepted an invalid replay: ${invalid_output}\n${invalid_error}")
endif()
