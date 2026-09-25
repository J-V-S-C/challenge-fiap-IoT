cmake_minimum_required(VERSION 3.15)
project(clinical_simulator_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

find_package(nlohmann_json 3.11.3 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(clinical_simulator
    src/main.cpp
    src/simulation_api.cpp
    src/simulation_api.h
    src/simulation_store.cpp
    src/simulation_store.h
)
target_include_directories(clinical_simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
)
target_link_libraries(clinical_simulator PRIVATE nlohmann_json::nlohmann_json)
