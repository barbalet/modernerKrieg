if(NOT DEFINED MK_HEADLESS_RUN)
  message(FATAL_ERROR "MK_HEADLESS_RUN is required")
endif()

if(NOT DEFINED MK_REPLAY_PATH)
  message(FATAL_ERROR "MK_REPLAY_PATH is required")
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
