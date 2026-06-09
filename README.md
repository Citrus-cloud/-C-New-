# Dungeon Survivors: D20 — C++ edition

Переписываем браузерную игру **Dungeon Survivors: D20** с JavaScript на C++ (движок на [raylib](https://www.raylib.com/)).
Жанр: roguelite / bullet-heaven в сеттинге D&D.

## Сборка

Нужны: компилятор C++ (MSVC / MinGW / g++) и CMake. Библиотека raylib подтягивается автоматически через CMake (FetchContent).

```bash
cmake -B build
cmake --build build
```

## Управление

- **WASD** / стрелки / левый стик геймпада — движение

## Структура

- `src/main.cpp` — точка входа и игровой цикл
- `src/player.h` / `src/player.cpp` — игрок (движение, коллизии)
- `src/tilemap.h` / `src/tilemap.cpp` — тайловая карта (пол / стены)
- `CMakeLists.txt` — конфигурация сборки

## Прогресс

- [x] Фаза 1 — каркас: пустое окно + игровой цикл
- [x] Фаза 2 — игрок и камера
- [x] Фаза 3 — мир и коллизии (тайловая карта + стены)
- [ ] Фаза 4 — враги
- [ ] Фаза 5 — бой (bullet heaven)
- [ ] Фаза 6 — прокачка и способности
