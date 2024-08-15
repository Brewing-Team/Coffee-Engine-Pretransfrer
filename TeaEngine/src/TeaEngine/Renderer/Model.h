#pragma once

#include "TeaEngine/Core/Base.h"
#include "TeaEngine/Renderer/Material.h"
#include "TeaEngine/Renderer/Mesh.h"
#include "TeaEngine/Renderer/Texture.h"
#include "TeaEngine/Scene/Scene.h"
#include <assimp/scene.h>
#include <glm/fwd.hpp>
#include <string>
#include <vector>
namespace Tea {

    class Model
    {
    public:
        Model() {};
        Model(const std::string& filePath);

        void LoadModel(const std::string& path);

        const std::vector<Ref<Mesh>>& GetMeshes() const { return m_Meshes; };
        void AddMesh(const Ref<Mesh> mesh) { m_Meshes.push_back(mesh); };

        std::string& GetName() { return m_Name; }

        const Model* GetParent() const { return m_Parent; }
        const std::vector<Ref<Model>> GetChildren() const { return m_Children; }
        
        const glm::mat4 GetTransform() const { return m_Transform; }
    private:
        Ref<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
        void processNode(aiNode* node, const aiScene* scene);
        Ref<Texture> LoadTexture(aiMaterial* material, aiTextureType type);
        MaterialTextures LoadMaterialTextures(aiMaterial* material);
    private:
        std::vector<Ref<Mesh>> m_Meshes;

        //Study to use raw pointers or smart pointers for the Hierarchy references
        Model* m_Parent;
        std::vector<Ref<Model>> m_Children;
        
        std::string m_FilePath;
        std::string m_Name;

        glm::mat4 m_Transform;
    };
}