if(NOT DEFINED MK_PROJECT_SOURCE_DIR)
  message(FATAL_ERROR "MK_PROJECT_SOURCE_DIR is required")
endif()

set(forbidden_include_patterns
  "#[ \t]*include[ \t]*[<\"]SDL"
  "#[ \t]*include[ \t]*[<\"]Cocoa/"
  "#[ \t]*include[ \t]*[<\"]AppKit/"
  "#[ \t]*include[ \t]*[<\"]UIKit/"
  "#[ \t]*include[ \t]*[<\"]Metal/"
  "#[ \t]*include[ \t]*[<\"]CoreGraphics/"
  "#[ \t]*include[ \t]*[<\"][Ww]indows\\.h"
  "#[ \t]*include[ \t]*[<\"]d3d"
)

file(GLOB_RECURSE core_files
  "${MK_PROJECT_SOURCE_DIR}/engine/core/include/*.h"
  "${MK_PROJECT_SOURCE_DIR}/engine/core/src/*.c"
)

foreach(core_file IN LISTS core_files)
  file(READ "${core_file}" contents)
  foreach(pattern IN LISTS forbidden_include_patterns)
    if(contents MATCHES "${pattern}")
      message(FATAL_ERROR "engine/core must not include platform frontend headers: ${core_file}")
    endif()
  endforeach()
endforeach()

message(STATUS "engine/core platform-header smoke check passed")
