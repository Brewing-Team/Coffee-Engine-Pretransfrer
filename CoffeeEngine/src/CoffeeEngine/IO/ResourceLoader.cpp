#include "ResourceLoader.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/IO/Resource.h"

#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/IO/ResourceImporter.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace Coffee {

    ResourceImporter ResourceLoader::s_Importer = ResourceImporter();

    void ResourceLoader::LoadFile(const std::filesystem::path& path)
    {
        ResourceType type = GetResourceTypeFromExtension(path);
        switch (type)
        {
            case ResourceType::Texture:
            {
                LoadTexture(path);
                break;
            }
            case ResourceType::Model:
            {
                LoadModel(path, false);
                break;
            }
            case ResourceType::Shader:
            {
                LoadShader(path);
                break;
            }
            case ResourceType::Unknown:
            {
                COFFEE_CORE_ERROR("ResourceLoader::LoadResources: Unsupported file extension {0}", path.extension().string());
                break;
            }
        }
    }

    void ResourceLoader::LoadDirectory(const std::filesystem::path& directory)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if(GetResourceTypeFromExtension(entry.path()) == ResourceType::Unknown and entry.path().extension() != ".import")
            {
                continue;
            }

            std::filesystem::path resourcePath = entry.path();

            if (resourcePath.extension() == ".import")
            {
                resourcePath = GetPathFromImportFile(resourcePath);
                COFFEE_CORE_INFO("ResourceLoader::LoadDirectory: Loading resource from import file {0}", resourcePath.string());
            }
            else
            {
                std::filesystem::path importFilePath = resourcePath;
                importFilePath.replace_extension(".import");
                if(!std::filesystem::exists(importFilePath))
                {
                    COFFEE_CORE_INFO("ResourceLoader::LoadDirectory: Generating import file for {0}", resourcePath.string());
                    GenerateImportFile(resourcePath);
                }
            }

            LoadFile(resourcePath);
        }
    }

    Ref<Texture> ResourceLoader::LoadTexture(const std::filesystem::path& path, bool srgb, bool cache)
    {
        if(GetResourceTypeFromExtension(path) != ResourceType::Texture)
        {
            COFFEE_CORE_ERROR("ResourceLoader::Load<Texture>: Resource is not a texture!");
            return nullptr;
        }

        UUID uuid = GetUUIDFromImportFile(path);

        if(ResourceRegistry::Exists(uuid))
        {
            return ResourceRegistry::Get<Texture>(uuid);
        }

        const Ref<Texture>& texture = s_Importer.ImportTexture(path, srgb, cache);

        ResourceRegistry::Add(uuid, texture);
        return texture;
    }

    Ref<Model> ResourceLoader::LoadModel(const std::filesystem::path& path, bool cache)
    {
        if(GetResourceTypeFromExtension(path) != ResourceType::Model)
        {
            COFFEE_CORE_ERROR("ResourceLoader::Load<Model>: Resource is not a model!");
            return nullptr;
        }

        UUID uuid = GetUUIDFromImportFile(path);

        if(ResourceRegistry::Exists(uuid))
        {
            return ResourceRegistry::Get<Model>(uuid);
        }

        const Ref<Model>& model = CreateRef<Model>(path);

        ResourceRegistry::Add(uuid, model);
        return model;
    }

    Ref<Shader> ResourceLoader::LoadShader(const std::filesystem::path& shaderPath)
    {
        if(GetResourceTypeFromExtension(shaderPath) != ResourceType::Shader)
        {
            COFFEE_CORE_ERROR("ResourceLoader::Load<Shader>: Resource is not a shader!");
            return nullptr;
        }

        UUID uuid = GetUUIDFromImportFile(shaderPath);

        if(ResourceRegistry::Exists(uuid))
        {
            return ResourceRegistry::Get<Shader>(uuid);
        }

        const Ref<Shader>& shader = CreateRef<Shader>(shaderPath);

        ResourceRegistry::Add(uuid, shader);

        return shader;

        //TODO: Add support for Resource Registry, Resource Importer and UUIDs

        //OLD CODE
        /*
            std::filesystem::path filePath(vertexPath);
            std::string fileName = filePath.stem().string();

            if(ResourceRegistry::Exists(fileName))
            {
                return ResourceRegistry::Get<Shader>(fileName);
            }
            else
            {
                Ref<Shader> shader = CreateRef<Shader>(vertexPath, fragmentPath);
                ResourceRegistry::Add(fileName, shader);
                return shader;
            }
        */
    }

    /*Ref<Shader> ResourceLoader::LoadShader(const std::string& shaderSource)
    {
        if(ResourceRegistry::Exists())

        const Ref<Shader>& shader = CreateRef<Shader>(shaderSource);

        ResourceRegistry::Add(uuid, shader);

        return shader;
    }*/

    ResourceType ResourceLoader::GetResourceTypeFromExtension(const std::filesystem::path& path)
    {
        auto extension = path.extension();

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
        {
            return ResourceType::Texture;
        }
        else if(extension == ".glb" || extension == ".gltf" || extension == ".fbx" || extension == ".obj")
        {
            return ResourceType::Model;
        }
        else if(extension == ".glsl")
        {
            return ResourceType::Shader;
        }

        return ResourceType::Unknown;
    }

    void ResourceLoader::GenerateImportFile(const std::filesystem::path& path)
    {
        std::filesystem::path importFilePath = path;
        importFilePath.replace_extension(".import");

        if(!std::filesystem::exists(importFilePath))
        {
            ImportData importData;
            importData.uuid = UUID();
            importData.originalPath = path;

            std::ofstream importFile(importFilePath);
            cereal::JSONOutputArchive archive(importFile);
            archive(CEREAL_NVP(importData));
        }
    }

    ResourceLoader::ImportData ResourceLoader::GetImportData(const std::filesystem::path& path)
    {
        ImportData importData;

        std::filesystem::path importFilePath = path;
        importFilePath.replace_extension(".import");

        /* TODO: add the path serialization relative to the project directory.
         * This means that when we load the path from the .import file we need to append the project directory
         * 
        */

        if(std::filesystem::exists(importFilePath))
        {
            std::ifstream importFile(importFilePath);
            cereal::JSONInputArchive archive(importFile);
            archive(CEREAL_NVP(importData));
        }
        else
        {
            COFFEE_CORE_ERROR("ResourceLoader::GetImportData: .import file does not exist for {0}", path.string());
        }

        return importData;
    }

    UUID ResourceLoader::GetUUIDFromImportFile(const std::filesystem::path& path)
    {
        ImportData importData = GetImportData(path);
        return importData.uuid;
    }

    std::filesystem::path ResourceLoader::GetPathFromImportFile(const std::filesystem::path& path)
    {
        ImportData importData = GetImportData(path);
        return importData.originalPath;
    }
}