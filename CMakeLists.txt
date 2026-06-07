cmake_minimum_required(VERSION 3.21)

project(
  modernerKrieg
  VERSION 0.1.0
  DESCRIPTION "Portable tactical wargame engine for the Mosul demo"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MK_BUILD_TESTS "Build modernerKrieg headless tests" ON)
option(MK_BUILD_SDL_APP "Build the SDL3 application when SDL3 is available" ON)
option(MK_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

function(mk_apply_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
    if(MK_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    if(MK_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()

add_library(modernerKriegCore STATIC
  engine/core/src/mk_core.c
  engine/core/src/mk_log.c
)

target_include_directories(modernerKriegCore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include>
)

if(NOT MSVC)
  target_link_libraries(modernerKriegCore PUBLIC m)
endif()

mk_apply_warnings(modernerKriegCore)

add_library(modernerKriegAssets STATIC
  engine/assets/src/mk_asset_manifest.c
)

target_include_directories(modernerKriegAssets
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/assets/include>
)

target_link_libraries(modernerKriegAssets PUBLIC modernerKriegCore)

mk_apply_warnings(modernerKriegAssets)

add_library(modernerKriegAi STATIC
  engine/ai/src/mk_ai.c
)

target_include_directories(modernerKriegAi
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/ai/include>
)

target_link_libraries(modernerKriegAi PUBLIC modernerKriegCore)

mk_apply_warnings(modernerKriegAi)

add_library(modernerKriegRender STATIC
  engine/render/src/mk_board_view.c
)

target_include_directories(modernerKriegRender
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/render/include>
)

target_link_libraries(modernerKriegRender PUBLIC modernerKriegCore)

mk_apply_warnings(modernerKriegRender)

add_library(modernerKriegMosul STATIC
  game/mosul/src/mk_mosul_demo.c
  game/mosul/src/mk_mosul_scenario_data.c
)

target_include_directories(modernerKriegMosul
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/game/mosul/include>
)

target_link_libraries(modernerKriegMosul PUBLIC modernerKriegCore modernerKriegAssets)

target_compile_definitions(modernerKriegMosul
  PRIVATE
    MK_PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

mk_apply_warnings(modernerKriegMosul)

add_executable(mk_headless_run
  engine/tools/autoplay/mk_headless_run.c
)
target_link_libraries(mk_headless_run PRIVATE modernerKriegAi modernerKriegMosul)
target_compile_definitions(mk_headless_run
  PRIVATE
    MK_PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
mk_apply_warnings(mk_headless_run)

add_executable(mk_ai_battle
  engine/tools/autoplay/mk_ai_battle.c
)
target_link_libraries(mk_ai_battle PRIVATE modernerKriegAi modernerKriegRender modernerKriegMosul)
target_compile_definitions(mk_ai_battle
  PRIVATE
    MK_PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
mk_apply_warnings(mk_ai_battle)

if(MK_BUILD_TESTS)
  enable_testing()

  add_executable(mk_core_tests
    tests/core/test_core.c
  )
  target_link_libraries(mk_core_tests PRIVATE modernerKriegMosul)
  target_include_directories(mk_core_tests PRIVATE tests)

  add_executable(mk_board_view_tests
    tests/render/test_board_view.c
  )
  target_link_libraries(mk_board_view_tests PRIVATE modernerKriegRender modernerKriegMosul)
  target_include_directories(mk_board_view_tests PRIVATE tests)

  add_executable(mk_fixed_loop_tests
    tests/autoplay/test_fixed_loop.c
  )
  target_link_libraries(mk_fixed_loop_tests PRIVATE modernerKriegMosul)
  target_include_directories(mk_fixed_loop_tests PRIVATE tests)

  add_executable(mk_basic_ai_tests
    tests/autoplay/test_basic_ai.c
  )
  target_link_libraries(mk_basic_ai_tests PRIVATE modernerKriegAi modernerKriegMosul)
  target_include_directories(mk_basic_ai_tests PRIVATE tests)

  add_executable(mk_asset_manifest_tests
    tests/assets/test_asset_manifest.c
  )
  target_link_libraries(mk_asset_manifest_tests PRIVATE modernerKriegAssets)
  target_include_directories(mk_asset_manifest_tests PRIVATE tests)
  target_compile_definitions(mk_asset_manifest_tests
    PRIVATE
      MK_TEST_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
      MK_TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  )

  add_executable(mk_mosul_scenario_data_tests
    tests/mosul/test_mosul_scenario_data.c
  )
  target_link_libraries(mk_mosul_scenario_data_tests PRIVATE modernerKriegMosul)
  target_include_directories(mk_mosul_scenario_data_tests PRIVATE tests)
  target_compile_definitions(mk_mosul_scenario_data_tests
    PRIVATE
      MK_TEST_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
      MK_TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  )

  mk_apply_warnings(mk_core_tests)
  mk_apply_warnings(mk_board_view_tests)
  mk_apply_warnings(mk_fixed_loop_tests)
  mk_apply_warnings(mk_basic_ai_tests)
  mk_apply_warnings(mk_asset_manifest_tests)
  mk_apply_warnings(mk_mosul_scenario_data_tests)

  add_test(NAME mk_core_tests COMMAND mk_core_tests)
  add_test(NAME mk_board_view_tests COMMAND mk_board_view_tests)
  add_test(NAME mk_fixed_loop_tests COMMAND mk_fixed_loop_tests)
  add_test(NAME mk_basic_ai_tests COMMAND mk_basic_ai_tests)
  add_test(NAME mk_asset_manifest_tests COMMAND mk_asset_manifest_tests)
  add_test(NAME mk_mosul_scenario_data_tests COMMAND mk_mosul_scenario_data_tests)
  add_test(NAME mk_headless_run_smoke COMMAND mk_headless_run --steps 3 --quiet)
  add_test(NAME mk_headless_run_ai_smoke COMMAND mk_headless_run --steps 3 --quiet --ai-only)
  add_test(NAME mk_ai_battle_smoke COMMAND mk_ai_battle --battles 1 --ticks 5 --quiet)
  add_test(
    NAME mk_headless_run_debug_balance
    COMMAND mk_headless_run
      --steps 3
      --quiet
      --ai-only
      --aar
      --debug-log
      --expect-objective opfor
      --expect-min-score -10
  )
  add_test(
    NAME mk_headless_run_replay_output
    COMMAND ${CMAKE_COMMAND}
      -DMK_HEADLESS_RUN=$<TARGET_FILE:mk_headless_run>
      -DMK_REPLAY_PATH=${CMAKE_CURRENT_BINARY_DIR}/mosul_replay.mkreplay
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckHeadlessReplay.cmake
  )
  add_test(
    NAME mk_headless_run_control_balance
    COMMAND mk_headless_run
      --scenario game/mosul/scenarios/market_control_smoke_2003.mkscenario
      --steps 8
      --quiet
      --ai-only
      --expect-objective player
      --expect-min-score 490
  )
  add_test(
    NAME mk_core_no_sdl_headers
    COMMAND ${CMAKE_COMMAND}
      -DMK_PROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckCoreNoSDL.cmake
  )
endif()

if(MK_BUILD_SDL_APP)
  find_package(SDL3 CONFIG QUIET)

  if(SDL3_FOUND)
    add_executable(modernerKrieg
      engine/platform/sdl3/main.c
    )
    target_link_libraries(modernerKrieg PRIVATE modernerKriegAssets modernerKriegRender modernerKriegMosul SDL3::SDL3)

    find_package(SDL3_image CONFIG QUIET)
    if(SDL3_image_FOUND)
      target_link_libraries(modernerKrieg PRIVATE SDL3_image::SDL3_image)
      target_compile_definitions(modernerKrieg PRIVATE MK_HAS_SDL3_IMAGE=1)
    else()
      message(STATUS "SDL3_image was not found; the SDL app will use the manifest-driven fallback map renderer.")
    endif()

    mk_apply_warnings(modernerKrieg)
  else()
    message(STATUS "SDL3 was not found; skipping the modernerKrieg SDL app target.")
    message(STATUS "Install SDL3 or set SDL3_DIR to enable the app target.")
  endif()
endif()
