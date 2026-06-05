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

add_library(modernerKriegCore STATIC
  engine/core/src/mk_core.c
)

target_include_directories(modernerKriegCore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include>
)

if(NOT MSVC)
  target_link_libraries(modernerKriegCore PUBLIC m)
endif()

if(MSVC)
  target_compile_options(modernerKriegCore PRIVATE /W4)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegCore PRIVATE /WX)
  endif()
else()
  target_compile_options(modernerKriegCore PRIVATE -Wall -Wextra -Wpedantic)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegCore PRIVATE -Werror)
  endif()
endif()

add_library(modernerKriegRender STATIC
  engine/render/src/mk_board_view.c
)

target_include_directories(modernerKriegRender
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/render/include>
)

target_link_libraries(modernerKriegRender PUBLIC modernerKriegCore)

if(MSVC)
  target_compile_options(modernerKriegRender PRIVATE /W4)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegRender PRIVATE /WX)
  endif()
else()
  target_compile_options(modernerKriegRender PRIVATE -Wall -Wextra -Wpedantic)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegRender PRIVATE -Werror)
  endif()
endif()

add_library(modernerKriegMosul STATIC
  game/mosul/src/mk_mosul_demo.c
)

target_include_directories(modernerKriegMosul
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/game/mosul/include>
)

target_link_libraries(modernerKriegMosul PUBLIC modernerKriegCore)

if(MSVC)
  target_compile_options(modernerKriegMosul PRIVATE /W4)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegMosul PRIVATE /WX)
  endif()
else()
  target_compile_options(modernerKriegMosul PRIVATE -Wall -Wextra -Wpedantic)
  if(MK_WARNINGS_AS_ERRORS)
    target_compile_options(modernerKriegMosul PRIVATE -Werror)
  endif()
endif()

if(MK_BUILD_TESTS)
  enable_testing()

  add_executable(mk_core_tests
    tests/core/test_core.c
  )
  target_link_libraries(mk_core_tests PRIVATE modernerKriegMosul)

  add_executable(mk_board_view_tests
    tests/render/test_board_view.c
  )
  target_link_libraries(mk_board_view_tests PRIVATE modernerKriegRender modernerKriegMosul)

  if(MSVC)
    target_compile_options(mk_core_tests PRIVATE /W4)
    target_compile_options(mk_board_view_tests PRIVATE /W4)
  else()
    target_compile_options(mk_core_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(mk_board_view_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME mk_core_tests COMMAND mk_core_tests)
  add_test(NAME mk_board_view_tests COMMAND mk_board_view_tests)
endif()

if(MK_BUILD_SDL_APP)
  find_package(SDL3 CONFIG QUIET)

  if(SDL3_FOUND)
    add_executable(modernerKrieg
      engine/platform/sdl3/main.c
    )
    target_link_libraries(modernerKrieg PRIVATE modernerKriegRender modernerKriegMosul SDL3::SDL3)

    if(MSVC)
      target_compile_options(modernerKrieg PRIVATE /W4)
    else()
      target_compile_options(modernerKrieg PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  else()
    message(STATUS "SDL3 was not found; skipping the modernerKrieg SDL app target.")
    message(STATUS "Install SDL3 or set SDL3_DIR to enable the app target.")
  endif()
endif()
