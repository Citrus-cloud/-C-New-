cmake_minimum_required(VERSION 3.15)
project(DungeonSurvivors CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/player.cpp
    src/tilemap.cpp
    src/enemy.cpp
    src/spawner.cpp
    src/projectile.cpp
    src/combat.cpp
    src/pickups.cpp
    src/upgrades.cpp
    src/pathfinding.cpp
    src/traps.cpp
)

target_link_libraries(${PROJECT_NAME} raylib)
