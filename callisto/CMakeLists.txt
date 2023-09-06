# CMakeList.txt : CMake project for callisto, include source and define
# project specific logic here.
#

# Add source to this project's executable.

cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 2)

add_definitions(-DCALLISTO_VERSION_MAJOR=${VERSION_MAJOR})
add_definitions(-DCALLISTO_VERSION_MINOR=${VERSION_MINOR})
add_definitions(-DCALLISTO_VERSION_PATCH=${VERSION_PATCH})

set(CALLISTO_LINK_OPTIONS "")
set(CALLISTO_COMPILE_OPTIONS "")
set(CALLISTO_COMPILE_DEFINITIONS "")

set(RC_CONTENTS "1 ICON \"icons\\callisto1.ico\" 
2 ICON \"icons\\callisto2.ico\"
3 ICON \"icons\\callisto3.ico\"
4 ICON \"icons\\callisto4.ico\"
5 ICON \"icons\\callisto5.ico\"
6 ICON \"icons\\callisto6.ico\"
1 VERSIONINFO 
      FILEVERSION     ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
      PRODUCTVERSION  ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
{
    BLOCK \"StringFileInfo\" 
    {
        BLOCK \"040904b0\" // en-US with UTF-8
        {
            VALUE \"CompanyName\", \"callisto developers\\0\" 
            VALUE \"FileDescription\", \"callisto - SMW build system\\0\"
            VALUE \"FileVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
            VALUE \"InternalName\", \"callisto\\0\" 
            VALUE \"OriginalFilename\", \"callisto.exe\\0\" 
            VALUE \"ProductName\", \"callisto\\0\" 
            VALUE \"ProductVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
        }
    }
    BLOCK \"VarFileInfo\" 
    {
        VALUE \"Translation\", 0x409, 1200 // en-US with UTF-8
    }
}"
)

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/callisto.rc" "${RC_CONTENTS}")

include(FetchContent)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ASAR_GEN_EXE OFF)
set(ASAR_GEN_EXE_TEST OFF)
set(ASAR_GEN_DLL_TEST OFF)
FetchContent_Declare(
	asar
	GIT_REPOSITORY https://github.com/Underrout/asar
	GIT_TAG v1.81
	SOURCE_SUBDIR src
)
FetchContent_MakeAvailable(asar)
set_target_properties(asar PROPERTIES CXX_STANDARD 14)

set(CALLISTO_SOURCE_FILES "" "configuration/color_configuration.h")

list(APPEND CALLISTO_SOURCE_FILES
"callisto.rc" "callisto.cpp" "callisto.h" "insertable.h" "cli/cli_handler.h" "cli/cli_handler.cpp"
"callisto_exception.h" "insertion_exception.h"  
 "insertables/exgraphics.h" "insertables/exgraphics.cpp"
  "insertables/graphics.h" "insertables/graphics.cpp" 
"not_found_exception.h" "insertables/lunar_magic_insertable.h" "insertables/lunar_magic_insertable.cpp" 
"insertables/levels.cpp" "insertables/levels.h"  
"insertables/rom_insertable.h" "insertables/shared_palettes.h" "insertables/shared_palettes.cpp" 
"insertables/flips_insertable.h" "insertables/flips_insertable.cpp" "insertables/overworld.h" 
 "insertables/title_screen.h"  "insertables/global_exanimation.h" "insertables/credits.h" 
 "insertables/title_moves.h" "insertables/title_moves.cpp" "colors.h"
 "insertables/binary_map16.h" "insertables/binary_map16.cpp" "insertables/text_map16.h" "insertables/text_map16.cpp" 
    "insertables/external_tool.h" "insertables/external_tool.cpp" "insertables/patch.h" "insertables/patch.cpp"
"configuration/config_exception.h" "configuration/config_variable.cpp" "configuration/config_variable.h" "configuration/configuration.h"
"configuration/configuration_level.h" "configuration/configuration.cpp" "configuration/tool_configuration.h" "configuration/emulator_configuration.h" "insertables/module.h"  "insertables/module.cpp" "configuration/configuration_manager.h" "configuration/configuration_manager.cpp" 
"dependency/resource_dependency.h" "dependency/dependency_exception.h" "builders/builder.h" "builders/builder.cpp" "builders/rebuilder.h" "path_util.h" "builders/rebuilder.cpp" "symbol.h"
"human_map16/arrays.h" "human_map16/data_error.h" "human_map16/filesystem_error.h" "human_map16/from_map16.cpp"
"human_map16/header_error.h" "human_map16/human_map16_exception.h" "human_map16/human_readable_map16.cpp" "human_map16/human_readable_map16.h"
"human_map16/tile_error.h" "human_map16/tile_format.h" "human_map16/to_map16.cpp"  "insertables/initial_patch.h" "insertables/initial_patch.cpp" "dependency/policy.h" "builders/quick_builder.h"  "builders/quick_builder.cpp" "builders/must_rebuild_exception.h" "saver/extractable_type.h" "extractables/lunar_magic_extractable.h" "extractables/lunar_magic_extractable.cpp" "extractables/flips_extractable.h" "extractables/flips_extractable.cpp" "extractables/extraction_exception.h" "extractables/global_exanimation.h" "extractables/credits.h" "extractables/overworld.h" "extractables/shared_palettes.h" "extractables/shared_palettes.cpp" "extractables/binary_map16.h" "extractables/binary_map16.cpp" "extractables/text_map16.h" "extractables/text_map16.cpp" "extractables/levels.h" "extractables/levels.cpp" "extractables/level.h" "extractables/level.cpp" "saver/extractable_type.h" "saver/saver.h" "saver/marker.cpp" "saver/saver.cpp" "emulators/emulators.h" "emulators/emulators.cpp" "tui/tui.h" "tui/tui.cpp" "extractables/exgraphics.cpp" "extractables/exgraphics.h" "extractables/graphics.cpp" "extractables/graphics.h"
"${asar_SOURCE_DIR}/src/asar-dll-bindings/c/asardll.c" "${asar_SOURCE_DIR}/src/asar-dll-bindings/c/asardll.h" "graphics_util.h" "graphics_util.cpp" "time_util.h" "lunar_magic/lunar_magic_wrapper.h" "lunar_magic/lunar_magic_wrapper.cpp")

if (MSVC) 
  list(APPEND CALLISTO_SOURCE_FILES
    "junction/libntfslinks/include/CharUtils.h"
    "junction/libntfslinks/include/Junction.h"
    "junction/libntfslinks/include/ntfstypes.h"
    "junction/libntfslinks/include/StringUtils.h"
    "junction/libntfslinks/source/StringUtils.cpp"
    "junction/libntfslinks/source/Junction.cpp"
    "junction/libntfslinks/source/CharUtils.cpp"
    "process_info.h" "process_info.cpp")
endif()

add_executable (callisto ${CALLISTO_SOURCE_FILES} "recent_projects/recent_projects_manager.h" "cli/cli_handler.h" "prompt_util.h"  "configuration/module_configuration.h" "colors.cpp" "globals.h" "globals.cpp")
target_link_options(callisto PRIVATE ${CALLISTO_LINK_OPTIONS})
target_compile_options(callisto PRIVATE ${CALLISTO_COMPILE_OPTIONS})
target_compile_definitions(callisto PRIVATE ${CALLISTO_COMPILE_DEFINITIONS})

target_include_directories(callisto PUBLIC ${asar_SOURCE_DIR}/src)

if (MSVC) 
  target_link_options(callisto PRIVATE $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
else()
  find_package(TBB REQUIRED COMPONENTS tbb)
  target_link_libraries(callisto PUBLIC tbb)

  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
endif()

FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG v2.3.1
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
    platform_folders
    GIT_REPOSITORY https://github.com/sago007/PlatformFolders
    GIT_TAG 4.2.0
)
FetchContent_MakeAvailable(platform_folders)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v4.0.0
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
 
target_link_libraries(
        callisto
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
)

FetchContent_Declare(
        toml11
        GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
        GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(toml11)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.11.0
)
FetchContent_MakeAvailable(spdlog)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.79.0 REQUIRED
    COMPONENTS
        system
        filesystem
        thread
        program_options
)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        ordered_map
        GIT_REPOSITORY https://github.com/Tessil/ordered-map.git
        GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(ordered_map)

target_link_libraries(callisto PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(callisto PUBLIC CLI11::CLI11)
target_link_libraries(callisto PUBLIC toml11::toml11)
target_link_libraries(callisto PUBLIC spdlog::spdlog)
target_link_libraries(callisto PRIVATE Boost::system Boost::filesystem Boost::thread Boost::program_options)
target_link_libraries(callisto PUBLIC ordered_map)
target_link_libraries(callisto PRIVATE fmt::fmt)
target_link_libraries(callisto PUBLIC platform_folders)
target_link_libraries(callisto
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

if (MSVC)
    set(RC_CONTENTS_ELOPER "1 ICON \"icons\\eloper.ico\" 
    1 VERSIONINFO 
          FILEVERSION     ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
          PRODUCTVERSION  ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
    {
        BLOCK \"StringFileInfo\" 
        {
            BLOCK \"040904b0\" // en-US with UTF-8
            {
                VALUE \"CompanyName\", \"callisto developers\\0\" 
                VALUE \"FileDescription\", \"eloper - callisto subsystem\\0\"
                VALUE \"FileVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
                VALUE \"InternalName\", \"eloper\\0\" 
                VALUE \"OriginalFilename\", \"eloper.exe\\0\" 
                VALUE \"ProductName\", \"eloper\\0\" 
                VALUE \"ProductVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
            }
        }
        BLOCK \"VarFileInfo\" 
        {
            VALUE \"Translation\", 0x409, 1200 // en-US with UTF-8
        }
    }"
    )

    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/eloper.rc" "${RC_CONTENTS_ELOPER}")

    add_executable(eloper WIN32 "eloper/eloper.h" "eloper/eloper.cpp" "process_info.h" "process_info.cpp" "eloper.rc" 
    "configuration/config_exception.h" "configuration/config_variable.cpp" "configuration/config_variable.h" "configuration/configuration.h"
    "configuration/configuration_level.h" "configuration/configuration.cpp" "configuration/tool_configuration.h" "configuration/emulator_configuration.h"
    "configuration/configuration_manager.h" "configuration/configuration_manager.cpp"
    "path_util.h"
    "descriptor.h"
    "configuration/color_configuration.h"
    "colors.h" "colors.cpp"
    "globals.h" "globals.cpp")

    target_link_libraries(eloper PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(eloper PUBLIC ordered_map)
    target_link_libraries(eloper PUBLIC platform_folders)
    target_link_libraries(eloper PUBLIC toml11::toml11)
    target_link_libraries(eloper PRIVATE fmt::fmt)
    target_link_libraries(eloper PUBLIC spdlog::spdlog)
    target_link_libraries(eloper PRIVATE Boost::system Boost::filesystem Boost::thread Boost::program_options)
    target_include_directories(eloper PUBLIC ${CMAKE_SOURCE_DIR})
endif()

add_custom_command(TARGET callisto POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:asar> $<TARGET_FILE_DIR:callisto>
)

add_custom_target(toml_config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/toml_config ${CMAKE_CURRENT_BINARY_DIR}/config
)
add_dependencies(callisto toml_config)

add_custom_target(initial_patches
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/initial_patches ${CMAKE_CURRENT_BINARY_DIR}/initial_patches
)
add_dependencies(callisto initial_patches)

add_custom_command(TARGET callisto POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/ASAR_LICENSE" $<TARGET_FILE_DIR:callisto>
)

add_custom_command(TARGET callisto POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" $<TARGET_FILE_DIR:callisto>
)
