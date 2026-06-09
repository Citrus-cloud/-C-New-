#include "textures.h"

// Создаёт заглушку 1x1 белый пиксель ровно один раз.
// Используется, когда нужный файл текстуры не найден на диске.
void TextureManager::EnsureFallback()
{
    if (fallbackReady) return;
    Image img = GenImageColor(1, 1, WHITE);
    fallback = LoadTextureFromImage(img);
    UnloadImage(img);
    fallbackReady = true;
}

// Возвращает текстуру по пути, используя кэш.
Texture2D TextureManager::Get(const std::string& path)
{
    // Уже загружали этот путь — отдаём из кэша.
    auto it = cache.find(path);
    if (it != cache.end()) return it->second;

    EnsureFallback();

    // Файла нет — кэшируем заглушку, чтобы не проверять диск повторно.
    if (!FileExists(path.c_str()))
    {
        cache[path] = fallback;
        real[path] = false;
        return fallback;
    }

    Texture2D tex = LoadTexture(path.c_str());
    // Для пиксель-арта отключаем сглаживание — пиксели должны быть чёткими.
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    cache[path] = tex;
    real[path] = true;
    return tex;
}

// Проверяет, есть ли реальный файл по пути (не заглушка).
bool TextureManager::IsReal(const std::string& path)
{
    auto it = real.find(path);
    if (it != real.end()) return it->second;
    // Ещё не запрашивали — просто проверим наличие файла.
    return FileExists(path.c_str());
}

// Выгружает все реальные текстуры и заглушку из памяти.
void TextureManager::UnloadAll()
{
    for (auto& kv : cache)
    {
        // Заглушку пропускаем — она одна и выгружается отдельно ниже.
        auto r = real.find(kv.first);
        if (r != real.end() && r->second)
            UnloadTexture(kv.second);
    }
    cache.clear();
    real.clear();
    if (fallbackReady)
    {
        UnloadTexture(fallback);
        fallbackReady = false;
    }
}
