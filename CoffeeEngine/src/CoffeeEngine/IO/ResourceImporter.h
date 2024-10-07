#pragma once

#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Renderer/Texture.h"
namespace Coffee {

    class ResourceImporter
    {
    public:
        template<typename T>
        static Ref<Resource> Import(const std::filesystem::path& path);
    private:
        template<typename T>
        static const Ref<Resource>& LoadFromFile(const std::filesystem::path& path);

        static void SaveToCache(const std::filesystem::path& path, Ref<Resource> resource);
        static const Ref<Resource>& LoadFromCache(const std::filesystem::path& path);
    };

}