cmake_minimum_required(VERSION 3.15)
project(DungeonSurvivorsCPP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматически скачиваем библиотеку raylib через FetchContent
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/player.cpp
    src/tilemap.cpp
    src/enemy.cpp
    src/spawner.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
