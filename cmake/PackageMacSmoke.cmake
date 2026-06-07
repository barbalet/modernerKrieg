if(NOT DEFINED MK_APP)
  message(FATAL_ERROR "MK_APP is required")
endif()

if(NOT DEFINED MK_PROJECT_SOURCE_DIR)
  message(FATAL_ERROR "MK_PROJECT_SOURCE_DIR is required")
endif()

if(NOT DEFINED MK_PACKAGE_ROOT)
  message(FATAL_ERROR "MK_PACKAGE_ROOT is required")
endif()

set(MK_PACKAGE_NAME "modernerKrieg-macos-smoke")
set(MK_PACKAGE_DIR "${MK_PACKAGE_ROOT}/${MK_PACKAGE_NAME}")
set(MK_PACKAGE_ARCHIVE "${MK_PACKAGE_ROOT}/${MK_PACKAGE_NAME}.zip")

function(mk_package_copy_project_file relative_path)
  get_filename_component(relative_dir "${relative_path}" DIRECTORY)
  file(MAKE_DIRECTORY "${MK_PACKAGE_DIR}/${relative_dir}")
  file(COPY_FILE
    "${MK_PROJECT_SOURCE_DIR}/${relative_path}"
    "${MK_PACKAGE_DIR}/${relative_path}"
    ONLY_IF_DIFFERENT
  )
endfunction()

file(REMOVE_RECURSE "${MK_PACKAGE_DIR}")
file(REMOVE "${MK_PACKAGE_ARCHIVE}")
file(MAKE_DIRECTORY "${MK_PACKAGE_DIR}/bin")

file(COPY_FILE "${MK_APP}" "${MK_PACKAGE_DIR}/bin/modernerKrieg" ONLY_IF_DIFFERENT)

mk_package_copy_project_file("README.md")
mk_package_copy_project_file("PLAN.md")
mk_package_copy_project_file("game/mosul/scenarios/README.md")
mk_package_copy_project_file("game/mosul/scenarios/market_commercial_streets_2003.mkscenario")
mk_package_copy_project_file("assets/mosul/manifests/README.md")
mk_package_copy_project_file("assets/mosul/manifests/market_commercial_streets_2003.mapmanifest")
mk_package_copy_project_file("assets/mosul/manifests/mosul_2003_sprites.spritemanifest")
mk_package_copy_project_file("assets/mosul/manifests/mosul_2003_markers.markermanifest")
mk_package_copy_project_file("assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/01_ground_level.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/02_level_2_alpha.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/03_level_3_alpha.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/04_roof_access_alpha.png")
mk_package_copy_project_file("assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/05_multistorey_mask.png")
mk_package_copy_project_file("assets/mosul/source/sprite_sheets/12_us_ally_troops_topdown_128.png")
mk_package_copy_project_file("assets/mosul/source/sprite_sheets/08_combatants_topdown_128.png")
mk_package_copy_project_file("assets/mosul/source/sprite_sheets/18_combatants_stances_topdown_128.png")
mk_package_copy_project_file("assets/mosul/source/sprite_sheets/14_us_ally_vehicles_topdown_128.png")

file(WRITE "${MK_PACKAGE_DIR}/run_modernerKrieg.sh"
"#!/bin/sh
cd \"$(dirname \"$0\")\"
exec ./bin/modernerKrieg --project-root . \"$@\"
")
file(CHMOD "${MK_PACKAGE_DIR}/run_modernerKrieg.sh"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${MK_PACKAGE_DIR}/bin/modernerKrieg" --project-root "${MK_PACKAGE_DIR}" --smoke-frames 2
  WORKING_DIRECTORY "${MK_PACKAGE_DIR}"
  RESULT_VARIABLE smoke_result
  OUTPUT_VARIABLE smoke_output
  ERROR_VARIABLE smoke_error
)

if(NOT smoke_result EQUAL 0)
  message(FATAL_ERROR "Packaged SDL smoke failed: ${smoke_error}\n${smoke_output}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${MK_PACKAGE_ARCHIVE}" --format=zip "${MK_PACKAGE_NAME}"
  WORKING_DIRECTORY "${MK_PACKAGE_ROOT}"
  RESULT_VARIABLE archive_result
  OUTPUT_VARIABLE archive_output
  ERROR_VARIABLE archive_error
)

if(NOT archive_result EQUAL 0)
  message(FATAL_ERROR "Could not create package archive: ${archive_error}\n${archive_output}")
endif()

message(STATUS "Created ${MK_PACKAGE_ARCHIVE}")
