# CMakeList.txt : CMake project for stardust, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (stardust "stardust.cpp" "stardust.h" "insertable.h" "stardust_exception.h" "insertion_exception.h" "insertables/graphics.h" "insertables/graphics.cpp" "insertables/exgraphics.h" "insertables/exgraphics.cpp")

set_property(TARGET stardust PROPERTY CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG c2ea58c7f9bb2a1da2d3d7f5b462121ac6a07f16  # v2.3.1 commit tag
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
        GIT_TAG dcfe39a783a94e8d52c885e5883a6fbb21529019  # v3.7.1 commit tag
)
FetchContent_MakeAvailable(toml11)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d  # v3.11.2 commit tag
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG ad0e89cbfb4d0c1ce4d097e134eb7be67baebb36  # v1.11.0 commit tag
)
FetchContent_MakeAvailable(spdlog)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.79.0 REQUIRED
    COMPONENTS
        system
        filesystem
        thread
)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(stardust PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(stardust PUBLIC CLI11::CLI11)
target_link_libraries(stardust PUBLIC toml11::toml11)
target_link_libraries(stardust PUBLIC spdlog::spdlog)
target_link_libraries(stardust PRIVATE Boost::system Boost::filesystem Boost::thread)
target_link_libraries(stardust PRIVATE fmt::fmt)

# TODO: Add tests and install targets if needed.
