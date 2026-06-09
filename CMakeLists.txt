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
    src/loot.cpp
    src/audio.cpp
    src/hud.cpp
    src/savegame.cpp
    src/textures.cpp
    src/animation.cpp
    src/effects.cpp
)

target_link_libraries(${PROJECT_NAME} raylib)

# Копируем папку assets рядом с исполняемым файлом,
# чтобы игра находила шрифты/звук/спрайты после сборки.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)
