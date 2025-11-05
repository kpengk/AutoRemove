cmake_minimum_required (VERSION 3.16)

project(AutoRemove)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

#find_package(nlohmann_json REQUIRED)

file(GLOB SRC_FILES src/*.h src/*.hpp src/*.cpp)
add_executable (AutoRemove ${SRC_FILES})

target_link_libraries(AutoRemove 
    PRIVATE 
        nlohmann_json::nlohmann_json
)
