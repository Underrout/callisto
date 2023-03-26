# CMakeList.txt : CMake project for stardust, include source and define
# project specific logic here.
#

# Add source to this project's executable.

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

SET(RC_CONTENTS "1 ICON \"stardust.ico\" 
1 VERSIONINFO 
      FILEVERSION     ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
      PRODUCTVERSION  ${VERSION_MAJOR}, ${VERSION_MINOR}, ${VERSION_PATCH}, 0
{
    BLOCK \"StringFileInfo\" 
    {
        BLOCK \"040904b0\" // en-US with UTF-8
        {
            VALUE \"CompanyName\", \"stardust developers\\0\" 
            VALUE \"FileDescription\", \"SMW build system\\0\"
            VALUE \"FileVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
            VALUE \"InternalName\", \"stardust\\0\" 
            VALUE \"OriginalFilename\", \"stardust.exe\\0\" 
            VALUE \"ProductName\", \"stardust\\0\" 
            VALUE \"ProductVersion\", \"v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\0\"
        }
    }
    BLOCK \"VarFileInfo\" 
    {
        VALUE \"Translation\", 0x409, 1200 // en-US with UTF-8
    }
}"
)

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/stardust.rc" "${RC_CONTENTS}")

add_executable (stardust "stardust.rc" "stardust.cpp" "stardust.h" "insertable.h" 
"stardust_exception.h" "insertion_exception.h" "insertables/graphics.h" 
"insertables/graphics.cpp" "insertables/exgraphics.h" "insertables/exgraphics.cpp" 
"not_found_exception.h" "insertables/lunar_magic_insertable.h" "insertables/lunar_magic_insertable.cpp" 
"insertables/rom_insertable.h" "insertables/shared_palettes.h" "insertables/shared_palettes.cpp" 
"insertables/flips_insertable.h" "insertables/flips_insertable.cpp" "insertables/overworld.h" 
 "insertables/title_screen.h"  "insertables/global_exanimation.h" "insertables/credits.h" 
 "insertables/title_moves.h" "insertables/title_moves.cpp" "insertables/level.h" "insertables/level.cpp" 
 "insertables/binary_map16.h" "insertables/binary_map16.cpp" "insertables/text_map16.h" "insertables/text_map16.cpp" 
  "insertables/pixi.cpp" "insertables/pixi.h" "insertables/external_tool.h" "insertables/external_tool.cpp" "insertables/patch.h" "insertables/patch.cpp"
"configuration/config_exception.h" "configuration/config_variable.cpp" "configuration/config_variable.h" "configuration/configuration.h"
"configuration/configuration_level.h" "configuration/configuration.cpp" "configuration/tool_configuration.h" "configuration/emulator_configuration.h" "insertables/globule.h"  "insertables/globule.cpp" "configuration/configuration_manager.h" "configuration/configuration_manager.cpp" 
"dependency/resource_dependency.h" "dependency/dependency_exception.h" "builders/builder.h" "builders/builder.cpp" "builders/rebuilder.h" "path_util.h" "builders/rebuilder.cpp" "symbol.h"
"human_map16/arrays.h" "human_map16/data_error.h" "human_map16/filesystem_error.h" "human_map16/from_map16.cpp"
"human_map16/header_error.h" "human_map16/human_map16_exception.h" "human_map16/human_readable_map16.cpp" "human_map16/human_readable_map16.h"
"human_map16/tile_error.h" "human_map16/tile_format.h" "human_map16/to_map16.cpp"  "insertables/initial_patch.h" "insertables/initial_patch.cpp" "dependency/policy.h" "builders/quick_builder.h"  "builders/quick_builder.cpp" "builders/must_rebuild_exception.h")

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
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

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v3.0.0
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
 
target_link_libraries(
        stardust
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

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ASAR_GEN_EXE OFF)
set(ASAR_GEN_EXE_TEST OFF)
set(ASAR_GEN_DLL_TEST OFF)
FetchContent_Declare(
	asar
	GIT_REPOSITORY https://github.com/RPGHacker/asar
	GIT_TAG v1.81
	SOURCE_SUBDIR src
)
FetchContent_MakeAvailable(asar)
set_target_properties(asar PROPERTIES CXX_STANDARD 14)

set(PIXI_BUILD_TESTS OFF)
FetchContent_Declare(
    pixi
    GIT_REPOSITORY https://github.com/JackTheSpades/SpriteToolSuperDelux
    GIT_TAG be9a3cff94a067d53cb1d8395fb4961da146a4fe
)
FetchContent_MakeAvailable(pixi)

target_link_libraries(stardust PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(stardust PUBLIC CLI11::CLI11)
target_link_libraries(stardust PUBLIC toml11::toml11)
target_link_libraries(stardust PUBLIC spdlog::spdlog)
target_link_libraries(stardust PRIVATE Boost::system Boost::filesystem Boost::thread Boost::program_options)
target_link_libraries(stardust PRIVATE fmt::fmt)
target_link_libraries(stardust PUBLIC pixi_api_static)
target_link_libraries(stardust PUBLIC platform_folders)

target_include_directories(stardust PUBLIC ${pixi_SOURCE_DIR}/src)
target_include_directories(stardust PUBLIC ${asar_SOURCE_DIR}/src)

add_custom_command(TARGET stardust POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:asar> $<TARGET_FILE_DIR:stardust>
)
