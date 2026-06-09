# Dungeon Survivors: D20 — C++ edition

Переписываем браузерную игру **Dungeon Survivors: D20** с JavaScript на C++ (движок на [raylib](https://www.raylib.com/)).
Жанр: roguelite / bullet-heaven в сеттинге D&D.

## Сборка

Нужны: компилятор C++ (MSVC / MinGW / g++) и CMake. Библиотека raylib подтягивается автоматически через CMake (FetchContent).

```bash
cmake -B build
cmake --build build
```

## Структура

- `src/` — исходный код
- `assets/` — спрайты и звуки (позже)
- `CMakeLists.txt` — конфигурация сборки

## Прогресс

- [x] Фаза 1 — каркас: пустое окно + игровой цикл
- [ ] Фаза 2 — игрок и камера
- [ ] Фаза 3 — мир и коллизии
- [ ] Фаза 4 — враги
