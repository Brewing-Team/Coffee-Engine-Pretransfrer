#include "Material.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    Ref<Texture> Material::s_MissingTexture;

    Material::Material()
    {
        ZoneScoped;

        s_MissingTexture = Texture::Load("assets/textures/UVMap-Grid.jpg");

        m_MaterialTextures.albedo = s_MissingTexture;
        m_MaterialTextureFlags.hasAlbedo = true;

        m_Shader = Coffee::Shader::Create("assets/shaders/StandardShader.vert", "assets/shaders/StandardShader.frag");

        m_Shader->Bind();
        m_MaterialTextures.albedo->Bind(0);
        m_Shader->setInt("material.albedoMap", 0);
        m_Shader->Unbind();
    }

    Material::Material(Ref<Shader> shader) : m_Shader(shader) {}

    Material::Material(MaterialTextures& materialTextures)
    {
        ZoneScoped;

        m_MaterialTextures.albedo = materialTextures.albedo;
        m_MaterialTextures.normal = materialTextures.normal;
        m_MaterialTextures.metallic = materialTextures.metallic;
        m_MaterialTextures.roughness = materialTextures.roughness;
        m_MaterialTextures.ao = materialTextures.ao;
        m_MaterialTextures.emissive = materialTextures.emissive;

        m_MaterialTextureFlags.hasAlbedo = (m_MaterialTextures.albedo != nullptr);
        m_MaterialTextureFlags.hasNormal = (m_MaterialTextures.normal != nullptr);
        m_MaterialTextureFlags.hasMetallic = (m_MaterialTextures.metallic != nullptr);
        m_MaterialTextureFlags.hasRoughness = (m_MaterialTextures.roughness != nullptr);
        m_MaterialTextureFlags.hasAO = (m_MaterialTextures.ao != nullptr);
        m_MaterialTextureFlags.hasEmissive = (m_MaterialTextures.emissive != nullptr);

        if(m_MaterialTextureFlags.hasMetallic)m_MaterialProperties.metallic = 1.0f;
        if(m_MaterialTextureFlags.hasEmissive)m_MaterialProperties.emissive = glm::vec3(1.0f);

        m_Shader = Coffee::Shader::Create("assets/shaders/StandardShader.vert", "assets/shaders/StandardShader.frag");

        m_Shader->Bind();
        m_Shader->setInt("material.albedoMap", 0);
        m_Shader->setInt("material.normalMap", 1);
        m_Shader->setInt("material.metallicMap", 2);
        m_Shader->setInt("material.roughnessMap", 3);
        m_Shader->setInt("material.aoMap", 4);
        m_Shader->setInt("material.emissiveMap", 5);
        m_Shader->Unbind();
    }

    void Material::Use()
    {
        ZoneScoped;

/*         // Update Texture Flags
        m_MaterialTextureFlags.hasAlbedo = (m_MaterialTextures.albedo != nullptr);
        m_MaterialTextureFlags.hasNormal = (m_MaterialTextures.normal != nullptr);
        m_MaterialTextureFlags.hasMetallic = (m_MaterialTextures.metallic != nullptr);
        m_MaterialTextureFlags.hasRoughness = (m_MaterialTextures.roughness != nullptr);
        m_MaterialTextureFlags.hasAO = (m_MaterialTextures.ao != nullptr);
        m_MaterialTextureFlags.hasEmissive = (m_MaterialTextures.emissive != nullptr); */

        m_Shader->Bind();

        // Bind Textures
        if(m_MaterialTextureFlags.hasAlbedo)m_MaterialTextures.albedo->Bind(0);
        if(m_MaterialTextureFlags.hasNormal)m_MaterialTextures.normal->Bind(1);
        if(m_MaterialTextureFlags.hasMetallic)m_MaterialTextures.metallic->Bind(2);
        if(m_MaterialTextureFlags.hasRoughness)m_MaterialTextures.roughness->Bind(3);
        if(m_MaterialTextureFlags.hasAO)m_MaterialTextures.ao->Bind(4);
        if(m_MaterialTextureFlags.hasEmissive)m_MaterialTextures.emissive->Bind(5);

/*         // Set Material Properties
        m_Shader->setVec4("material.color", m_MaterialProperties.color);
        m_Shader->setFloat("material.metallic", m_MaterialProperties.metallic);
        m_Shader->setFloat("material.roughness", m_MaterialProperties.roughness);
        m_Shader->setFloat("material.ao", m_MaterialProperties.ao);
        m_Shader->setVec3("material.emissive", m_MaterialProperties.emissive);

        // Set Material Texture Flags
        m_Shader->setInt("material.hasAlbedo", m_MaterialTextureFlags.hasAlbedo);
        m_Shader->setInt("material.hasNormal", m_MaterialTextureFlags.hasNormal);
        m_Shader->setInt("material.hasMetallic", m_MaterialTextureFlags.hasMetallic);
        m_Shader->setInt("material.hasRoughness", m_MaterialTextureFlags.hasRoughness);
        m_Shader->setInt("material.hasAO", m_MaterialTextureFlags.hasAO);
        m_Shader->setInt("material.hasEmissive", m_MaterialTextureFlags.hasEmissive); */
    }
}
