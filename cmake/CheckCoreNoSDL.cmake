if(NOT DEFINED MK_PROJECT_SOURCE_DIR)
  message(FATAL_ERROR "MK_PROJECT_SOURCE_DIR is required")
endif()

file(GLOB_RECURSE core_files
  "${MK_PROJECT_SOURCE_DIR}/engine/core/include/*.h"
  "${MK_PROJECT_SOURCE_DIR}/engine/core/src/*.c"
)

foreach(core_file IN LISTS core_files)
  file(READ "${core_file}" contents)
  if(contents MATCHES "#[ \t]*include[ \t]*[<\"]SDL")
    message(FATAL_ERROR "engine/core must not include SDL headers: ${core_file}")
  endif()
endforeach()

message(STATUS "engine/core SDL include smoke check passed")
