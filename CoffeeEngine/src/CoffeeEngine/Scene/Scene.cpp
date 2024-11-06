#include "Scene.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Input.h"
#include "CoffeeEngine/Core/Stopwatch.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/PrimitiveMesh.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entity/snapshot.hpp"
#include <cstdint>
#include <string>
#include <tracy/Tracy.hpp>

#include <cereal/archives/json.hpp>
#include <fstream>

namespace Coffee {

    //TEMPORAL
    static Ref<Material> missingMaterial;

    Scene::Scene()
    {
        m_SceneTree = CreateScope<SceneTree>(this);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        ZoneScoped;

        Entity entity = { m_Registry.create(), this };
        entity.AddComponent<TransformComponent>();
        auto& nameTag = entity.AddComponent<TagComponent>();
        nameTag.Tag = name.empty() ? "Entity" : name;
        entity.AddComponent<HierarchyComponent>();
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        auto& hierarchyComponent = m_Registry.get<HierarchyComponent>(entity);
        auto curr = hierarchyComponent.m_First;

        while(curr != entt::null)
        {
            Entity e{curr, this};
            DestroyEntity(e);
            curr = m_Registry.get<HierarchyComponent>(curr).m_Next;
        }

        m_Registry.destroy((entt::entity)entity);
    }

    void Scene::OnInit()
    {
        ZoneScoped;

/*         Entity Root = CreateEntity("Root");

        for(int i = 0; i < 5; i++)
        {
            Entity child1 = CreateEntity("Child(" + std::to_string(i) + ")");
            child1.SetParent(Root);

            for(int i = 0; i < 5; i++)
            {
                Entity child2 = CreateEntity("Child(" + std::to_string(i) + ")");
                child2.SetParent(child1);
            }
        } */

        /* for(int i = 0; i < 4; i++)
        {
            Entity light = CreateEntity("Light");
            light.AddComponent<LightComponent>();
            light.GetComponent<TransformComponent>().Position = glm::vec3(rand () % 10, rand () % 10, rand () % 10);
        } */
/*         for(int i = 0; i < 4; i++)
                {
                    Entity light = CreateEntity("Light");
                    light.AddComponent<LightComponent>();

                    glm::vec3 position;
                    switch (i)
                    {
                        case 0:
                            position = glm::vec3(-5.0f, 5.0f, -5.0f); // Top-left
                            break;
                        case 1:
                            position = glm::vec3(5.0f, 5.0f, -5.0f); // Top-right
                            break;
                        case 2:
                            position = glm::vec3(-5.0f, 5.0f, 5.0f); // Bottom-left
                            break;
                        case 3:
                            position = glm::vec3(5.0f, 5.0f, 5.0f); // Bottom-right
                            break;
                    }

                    light.GetComponent<TransformComponent>().Position = position;
                } */

        /*Entity cube = CreateEntity("Cube test");
        cube.AddComponent<MeshComponent>(PrimitiveMesh::CreateCube());*/

        /*Entity sphere = CreateEntity("Sphere test");
        sphere.AddComponent<MeshComponent>(PrimitiveMesh::CreateSphere());*/

        Entity cylinder = CreateEntity("Cylinder test");
        cylinder.AddComponent<MeshComponent>(PrimitiveMesh::CreateCylinder());

        /*Entity torus = CreateEntity("Torus test");
        torus.AddComponent<MeshComponent>(PrimitiveMesh::CreateTorus());*/

        /*Entity capsule = CreateEntity("Capsule test");
        capsule.AddComponent<MeshComponent>(PrimitiveMesh::CreateCapsule());*/

        /*Entity cone = CreateEntity("Cone test");
        cone.AddComponent<MeshComponent>(PrimitiveMesh::CreateCone());*/

        Entity light = CreateEntity("Directional Light");
        light.AddComponent<LightComponent>().Color = {1.0f, 0.9f, 0.85f};
        light.GetComponent<TransformComponent>().Position = {0.0f, 0.8f, -2.1f};
        
        Entity camera = CreateEntity("Camera");
        camera.AddComponent<CameraComponent>();

        /* AddModelToTheSceneTree(this, CreateRef<Model>("assets/models/DamagedHelmet/DamagedHelmet.gltf")); */

        Ref<Shader> missingShader = Shader::Create("assets/shaders/MissingShader.vert", "assets/shaders/MissingShader.frag");
        missingMaterial = CreateRef<Material>(missingShader);
    }

    void Scene::OnUpdateEditor(EditorCamera& camera, float dt)
    {
        ZoneScoped;

        m_SceneTree->Update();

        Renderer::BeginScene(camera);

        // Get all entities with ModelComponent and TransformComponent
        auto view = m_Registry.view<MeshComponent, TransformComponent>();

        // Loop through each entity with the specified components
        for (auto& entity : view)
        {
            // Get the ModelComponent and TransformComponent for the current entity
            auto& meshComponent = view.get<MeshComponent>(entity);
            auto& transformComponent = view.get<TransformComponent>(entity);
            auto materialComponent = m_Registry.try_get<MaterialComponent>(entity);

            const Ref<Mesh>& mesh = meshComponent.GetMesh();
            const Ref<Material>& material = (materialComponent and materialComponent->material) ? materialComponent->material : missingMaterial;
            
            Renderer::Submit(material, mesh, transformComponent.GetWorldTransform(), (uint32_t)entity);
        }

        //Get all entities with LightComponent and TransformComponent
        auto lightView = m_Registry.view<LightComponent, TransformComponent>();

        //Loop through each entity with the specified components
        for(auto& entity : lightView)
        {
            auto& lightComponent = lightView.get<LightComponent>(entity);
            auto& transformComponent = lightView.get<TransformComponent>(entity);

            lightComponent.Position = transformComponent.GetWorldTransform()[3];
            lightComponent.Direction = glm::normalize(glm::vec3(-transformComponent.GetWorldTransform()[1]));

            //Renderer::Submit(lightComponent);
        }

        Renderer::EndScene();
    }

    void Scene::OnUpdateRuntime(float dt)
    {
        ZoneScoped;

        m_SceneTree->Update();

        Camera* camera = nullptr;
        glm::mat4 cameraTransform;
        auto cameraView = m_Registry.view<TransformComponent, CameraComponent>();
        for(auto entity : cameraView)
        {
            auto [transform, cameraComponent] = cameraView.get<TransformComponent, CameraComponent>(entity);
            
            //TODO: Multiple cameras support (for now, the last camera found will be used)
            camera = &cameraComponent.Camera;
            cameraTransform = transform.GetWorldTransform();
        }

        if(!camera)
        {
            COFFEE_ERROR("No camera entity found!");

            SceneCamera sceneCamera;
            camera = &sceneCamera;

            cameraTransform = glm::mat4(1.0f);
        }

        //TODO: Add this to a function bc it is repeated in OnUpdateEditor
        Renderer::BeginScene(*camera, cameraTransform);
        
        // Get all entities with ModelComponent and TransformComponent
        auto view = m_Registry.view<MeshComponent, TransformComponent>();

        // Loop through each entity with the specified components
        for (auto& entity : view)
        {
            // Get the ModelComponent and TransformComponent for the current entity
            auto& meshComponent = view.get<MeshComponent>(entity);
            auto& transformComponent = view.get<TransformComponent>(entity);
            auto materialComponent = m_Registry.try_get<MaterialComponent>(entity);

            Ref<Mesh> mesh = meshComponent.GetMesh();
            Ref<Material> material = (materialComponent and materialComponent->material) ? materialComponent->material : missingMaterial;
            
            Renderer::Submit(material, mesh, transformComponent.GetWorldTransform());
        }

        //Get all entities with LightComponent and TransformComponent
        auto lightView = m_Registry.view<LightComponent, TransformComponent>();

        //Loop through each entity with the specified components
        for(auto& entity : lightView)
        {
            auto& lightComponent = lightView.get<LightComponent>(entity);
            auto& transformComponent = lightView.get<TransformComponent>(entity);

            lightComponent.Position = transformComponent.GetWorldTransform()[3];
            lightComponent.Direction = glm::normalize(glm::vec3(-transformComponent.GetWorldTransform()[1]));

            Renderer::Submit(lightComponent);
        }

        Renderer::EndScene();
    }

    void Scene::OnEvent(Event& e)
    {
        ZoneScoped;
    }

    void Scene::OnExit()
    {
        ZoneScoped;
    }

    Ref<Scene> Scene::Load(const std::filesystem::path& path)
    {
        ZoneScoped;

        Ref<Scene> scene = CreateRef<Scene>();

        std::ifstream sceneFile(path);
        cereal::JSONInputArchive archive(sceneFile);

        entt::snapshot_loader{scene->m_Registry}
            .get<entt::entity>(archive)
            .get<TagComponent>(archive)
            .get<TransformComponent>(archive)
            .get<HierarchyComponent>(archive)
            //.get<CameraComponent>(archive)
            //.get<MeshComponent>(archive)
            //.get<MaterialComponent>(archive)
            .get<LightComponent>(archive);

        return scene;
    }

    void Scene::Save(const std::filesystem::path& path, Ref<Scene> scene)
    {
        ZoneScoped;

        std::ofstream sceneFile(path);
        cereal::JSONOutputArchive archive(sceneFile);

        //archive(*scene);

        //TEMPORAL
        entt::snapshot{scene->m_Registry}
            .get<entt::entity>(archive)
            .get<TagComponent>(archive)
            .get<TransformComponent>(archive)
            .get<HierarchyComponent>(archive)
            //.get<CameraComponent>(archive)
            //.get<MeshComponent>(archive)
            //.get<MaterialComponent>(archive)
            .get<LightComponent>(archive);
    }

    void AddModelToTheSceneTree(Scene* scene, Ref<Model> model)
    {
        static Entity parent;

        Entity modelEntity = scene->CreateEntity(model->GetName());

        if((entt::entity)parent != entt::null)modelEntity.SetParent(parent);
        modelEntity.GetComponent<TransformComponent>().SetLocalTransform(model->GetTransform());

        auto& meshes = model->GetMeshes();
        bool hasMultipleMeshes = meshes.size() > 1;

        for(auto& mesh : meshes)
        {
            Entity entity = hasMultipleMeshes ? scene->CreateEntity(mesh->GetName()) : modelEntity;

            entity.AddComponent<MeshComponent>(mesh);

            if(mesh->GetMaterial())
            {
                entity.AddComponent<MaterialComponent>(mesh->GetMaterial());
            }

            if(hasMultipleMeshes)
            {
                entity.SetParent(modelEntity);
            }
        }

        for(auto& c : model->GetChildren())
        {
            parent = modelEntity;
            AddModelToTheSceneTree(scene, c);
        }
    }

}
