# CMakeList.txt : CMake project for stardust, include source and define
# project specific logic here.
#

# Add source to this project's executable.

add_executable (stardust "stardust.cpp" "stardust.h" "insertable.h" 
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
"configuration/configuration_level.h" "configuration/configuration.cpp" "configuration/tool_configuration.h" "configuration/emulator_configuration.h")

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG v2.3.1
)
FetchContent_MakeAvailable(cli11)

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

target_include_directories(stardust PUBLIC ${pixi_SOURCE_DIR}/src)
target_include_directories(stardust PUBLIC ${asar_SOURCE_DIR}/src)

add_custom_command(TARGET stardust POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:asar> $<TARGET_FILE_DIR:stardust>
)
