#include "EditorLayer.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/FileDialog.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Core/Application.h"
#include "CoffeeEngine/Core/MouseCodes.h"
#include "CoffeeEngine/Events/KeyEvent.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/PrimitiveMesh.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/DebugRenderer.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Core/Input.h"
#include "Panels/SceneTreePanel.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "entt/entity/entity.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <cstdint>
#include <sys/types.h>
#include <tracy/Tracy.hpp>

namespace Coffee {

    static RendererStats s_RendererData;

    EditorLayer::EditorLayer() : Layer("Example")
    {

    }

    void EditorLayer::OnAttach()
    {
        ZoneScoped;

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;

        m_EditorCamera = EditorCamera(45.0f);

        m_ActiveScene->OnInit();

        m_SceneTreePanel.SetContext(m_ActiveScene);
        m_ContentBrowserPanel.SetContext(m_ActiveScene);

        //For now we are going to create a new project when the editor is attached
        Project::New();
    }

    void EditorLayer::OnUpdate(float dt)
    {
        ZoneScoped;

        switch (m_SceneState)
        {
            case SceneState::Edit:
                m_EditorCamera.OnUpdate();
                m_ActiveScene->OnUpdateEditor(m_EditorCamera, dt);
                OnOverlayRender();
            break;
            case SceneState::Play:
                m_ActiveScene->OnUpdateRuntime(dt);
            break;

        }
    }

    void EditorLayer::OnEvent(Coffee::Event& event)
    {
        ZoneScoped;

        m_EditorCamera.OnEvent(event);

        m_ActiveScene->OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(COFFEE_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(COFFEE_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if(event.IsRepeat() > 0)
            return false;

        switch (event.GetKeyCode())
        {
            case Coffee::Key::G:
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
            break;
            case Coffee::Key::R:
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
            break;
            case Coffee::Key::S:
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
            break;
            case Coffee::Key::F:
                if(m_SceneTreePanel.GetSelectedEntity())
                {
                    glm::mat4 worldTransform = m_SceneTreePanel.GetSelectedEntity().GetComponent<TransformComponent>().GetWorldTransform();

                    m_EditorCamera.SetFocusPoint(glm::vec3(worldTransform[3]));
                }
            break;
        }

        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
    {
        if (event.GetMouseButton() == Mouse::BUTTON_LEFT)
        {
            if (m_ViewportHovered && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
            {
                //TODO: Clean this up and wrap it in a function
                glm::vec2 mousePos = Input::GetMousePosition();
                mousePos.x -= m_ViewportBounds[0].x;
                mousePos.y -= m_ViewportBounds[0].y;
                glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
                mousePos.y = viewportSize.y - mousePos.y;
                int mouseX = (int)mousePos.x;
                int mouseY = (int)mousePos.y;

                if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
                {
                    const glm::vec4& pixelData = Renderer::GetEntityIDAtPixel(mouseX, mouseY);

                    /// Convert the vec3 back to uint32_t
                    uint32_t r = static_cast<uint32_t>(pixelData.r * 255.0f);
                    uint32_t g = static_cast<uint32_t>(pixelData.g * 255.0f);
                    uint32_t b = static_cast<uint32_t>(pixelData.b * 255.0f);

                    uint32_t entityID = (r << 0) | (g << 8) | (b << 16);

                    Entity hoveredEntity = entityID == -1 ? Entity() : Entity((entt::entity)entityID, m_ActiveScene.get());

                    m_SceneTreePanel.SetSelectedEntity(hoveredEntity);
                }
            }
        }
        return false;
    }

    void EditorLayer::OnDetach()
    {
        ZoneScoped;

        m_ActiveScene->OnExit();
    }

    void EditorLayer::OnImGuiRender()
    {
        ZoneScoped;

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        std::string mainMenuAction = "";
        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) { NewScene(); }
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) { OpenScene(); }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { SaveScene(); }
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) { SaveSceneAs(); }
                if (ImGui::MenuItem("Exit")) { Application::Get().Close(); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Project"))
            {
                if (ImGui::MenuItem("New Project...", "Ctrl+N")) { NewProject(); }
                if (ImGui::MenuItem("Open Project...", "Ctrl+O")) { OpenProject(); }
                if (ImGui::MenuItem("Save Project", "Ctrl+S")) { SaveProject(); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Editor"))
            {
                if(ImGui::BeginMenu("Color Theme"))
                {
                    if(ImGui::MenuItem("Coffee"))
                    {
                        Application::Get().GetImGuiLayer()->SetCoffeeColorStyle();
                    }
                    if(ImGui::MenuItem("Godot"))
                    {
                        Application::Get().GetImGuiLayer()->SetGodotColorStyle();
                    }
                    if(ImGui::MenuItem("Tea"))
                    {
                        Application::Get().GetImGuiLayer()->SetTeaColorStyle();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Windows"))
                {
                    if(ImGui::MenuItem("Scene Tree", nullptr, m_SceneTreePanel.IsVisible())) { m_SceneTreePanel.ToggleVisibility(); }
                    if(ImGui::MenuItem("Content Browser", nullptr, m_ContentBrowserPanel.IsVisible())) { m_ContentBrowserPanel.ToggleVisibility(); }
                    if(ImGui::MenuItem("Output", nullptr, m_OutputPanel.IsVisible())) { m_OutputPanel.ToggleVisibility(); }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("About"))
            {
                if(ImGui::MenuItem("About Coffee Engine"))
                {
                    mainMenuAction = "About Coffee Engine";
                }
                ImGui::EndMenu();
            }

            //Play and Stop buttons
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 50);

            switch (m_SceneState)
            {
                case SceneState::Edit:
                    if(ImGui::Button("Play"))
                    {
                        m_SceneState = SceneState::Play;
                    }
                break;
                case SceneState::Play:
                    if(ImGui::Button("Stop"))
                    {
                        m_SceneState = SceneState::Edit;
                    }
                break;
            }

            //set the fps counter in the right side of the menu bar
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 75);
            ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);

            ImGui::EndMainMenuBar();
        }

        // About Coffee Engine Popup

        if(mainMenuAction == "About Coffee Engine"){ ImGui::OpenPopup("About Coffee Engine"); }
        if(ImGui::BeginPopupModal("About Coffee Engine", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Coffee Engine is a 3D Game Engine developed by the Brewing Team.");
            ImGui::TextWrapped("This project is open source and can be found at:");
            ImGui::TextLinkOpenURL("https://github.com/Brewing-Team/Coffee-Engine");
            ImGui::EndPopup();
        }



        // Render the panels
        m_SceneTreePanel.OnImGuiRender();
        m_ContentBrowserPanel.OnImGuiRender();
        m_OutputPanel.OnImGuiRender();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ResizeViewport(viewportPanelSize.x, viewportPanelSize.y);

        uint32_t textureID = Renderer::GetRenderTexture()->GetID();
        ImGui::Image((void*)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, {0, 1}, {1, 0});

        //Guizmo
        Entity selectedEntity = m_SceneTreePanel.GetSelectedEntity();

        if(selectedEntity and m_GizmoType != -1 and m_SceneState == SceneState::Edit)
        {
            ImGuizmo::SetGizmoSizeClipSpace(0.2);

            // Customize ImGuizmo style to be more similar to Godot

            auto& style = ImGuizmo::GetStyle();

            //style.TranslationLineThickness = 3.0f;
            //style.TranslationLineArrowSize = 10.0f;
            //style.RotationLineThickness = 4.0f;
            //style.RotationOuterLineThickness = 4.0f;
            //style.ScaleLineThickness = 4.0f;
            //style.ScaleLineCircleSize = 6.0f;

            // Set colors
            style.Colors[ImGuizmo::DIRECTION_X] = ImVec4(0.918f, 0.196f, 0.310f, 1.0f);
            style.Colors[ImGuizmo::DIRECTION_Y] = ImVec4(0.153f, 0.525f, 0.918f, 1.0f);
            style.Colors[ImGuizmo::DIRECTION_Z] = ImVec4(0.502f, 0.800f, 0.051f, 1.0f);
            style.Colors[ImGuizmo::PLANE_X] = ImVec4(0.918f, 0.196f, 0.310f, 1.0f);
            style.Colors[ImGuizmo::PLANE_Y] = ImVec4(0.153f, 0.525f, 0.918f, 1.0f);
            style.Colors[ImGuizmo::PLANE_Z] = ImVec4(0.502f, 0.800f, 0.051f, 1.0f);
            style.Colors[ImGuizmo::SELECTION] = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

            const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            auto& transformComponent = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = transformComponent.GetWorldTransform();

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL,
                         glm::value_ptr(transform));

            if (ImGuizmo::IsUsing())
            {
              /*TODO: Revisit this bc this should work using the SetWorldTransform
                but for this in the SetWorldTransform we should update the local
                transform too and for this we need the transform of the parent.*/

                glm::mat4 localTransform = transform;

                auto& parentEntity = selectedEntity.GetComponent<HierarchyComponent>().m_Parent;
                if(parentEntity != entt::null)
                {
                    Entity e{parentEntity, m_ActiveScene.get()};
                    glm::mat4 parentGlobalTransform = e.GetComponent<TransformComponent>().GetWorldTransform();
                    glm::mat4 inverseParentGlobalTransform = glm::inverse(parentGlobalTransform);
                    localTransform = inverseParentGlobalTransform * transform;
                }

                // Update the local transform component
                transformComponent.SetLocalTransform(localTransform);
            }
        }
        else
        {
            m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        }

        //transparent overlay displaying fps draw calls etc
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | /*ImGuiWindowFlags_AlwaysAutoResize |*/ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 150, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 100));

        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

        ImGui::Begin("Renderer Stats", NULL, window_flags);
        ImGui::Text("Size: %.0f x %.0f (%0.1fMP)", m_ViewportSize.x, m_ViewportSize.y, m_ViewportSize.x * m_ViewportSize.y / 1000000.0f);
        ImGui::Text("Draw Calls: %d", Renderer::GetStats().DrawCalls);
        ImGui::Text("Vertex Count: %d", Renderer::GetStats().VertexCount);
        ImGui::Text("Index Count: %d", Renderer::GetStats().IndexCount);
        ImGui::End();

        ImGui::End();
        ImGui::PopStyleVar();

        //---------TESTING---------
        ImGui::Begin("Render Settings");

        ImGui::Checkbox("Post Processing", &Renderer::GetRenderSettings().PostProcessing);

        ImGui::DragFloat("Exposure", &Renderer::GetRenderSettings().Exposure, 0.001f, 100.0f);

        ImGui::End();

        //Debug Window for testing the ResourceRegistry
        ImGui::Begin("Resource Registry");

        auto& resources = ResourceRegistry::GetResourceRegistry();

        for(auto& resource : resources)
        {
            ImGui::Text(resource.first.c_str());
            ImGui::SameLine();
            ImGui::Text("Use Count: %ld", resource.second.use_count() - 1);
        }

        ImGui::End();
    }

    void EditorLayer::OnOverlayRender()
    {
        Renderer::BeginOverlay(m_EditorCamera);

        Entity selectedEntity = m_SceneTreePanel.GetSelectedEntity();
        static Entity lastSelectedEntity;

        if(selectedEntity)
        {
            auto& transformComponent = selectedEntity.GetComponent<TransformComponent>();
            if (selectedEntity.HasComponent<MeshComponent>()) {
                auto& meshComponent = selectedEntity.GetComponent<MeshComponent>();

                glm::mat4 transform = transformComponent.GetWorldTransform();

                if(meshComponent.drawAABB)
                {
                    const AABB& aabb = meshComponent.mesh->GetAABB(transform);
                    DebugRenderer::DrawBox(aabb, {0.27f, 0.52f, 0.53f, 1.0f});
                }

                // ----------------------------------

                OBB obb = meshComponent.mesh->GetOBB(transform);
                DebugRenderer::DrawBox(obb, {0.99f, 0.50f, 0.09f, 1.0f});


            } else if (selectedEntity != lastSelectedEntity) {
                // TODO generate defaults bounding boxes for when the entity does not have a mesh component
                lastSelectedEntity = selectedEntity;
                COFFEE_CORE_WARN("Not printing bounding box: Selected entity {0} does not have a MeshComponent.", selectedEntity.GetComponent<TagComponent>().Tag);
            }

        }

        auto view = m_ActiveScene->GetAllEntitiesWithComponents<LightComponent, TransformComponent>();

        for(auto entity : view)
        {
            auto& lightComponent = view.get<LightComponent>(entity);
            auto& transformComponent = view.get<TransformComponent>(entity);

            switch (lightComponent.type) {
                case LightComponent::Type::DirectionalLight:
                    //DebugRenderer::DrawArrow(transformComponent.GetWorldTransform()[3], lightComponent.Direction, lightComponent.Intensity);
                    DebugRenderer::DrawArrow(transformComponent.GetWorldTransform()[3], lightComponent.Direction, 1.5f);
                break;

                case LightComponent::Type::PointLight:
                    glm::vec3 worldPosition = transformComponent.GetWorldTransform()[3];
                    float radius = lightComponent.Range;
                    DebugRenderer::DrawSphere(worldPosition, radius);
                break;

                /* case LightComponent::Type::SpotLight:
                break;    */         
            }
        }

        DebugRenderer::DrawLine({-1000.0f, 0.0f, 0.0f}, {1000.0f, 0.0f, 0.0f}, {0.918f, 0.196f, 0.310f, 1.0f}, 2);
        DebugRenderer::DrawLine({0.0f, -1000.0f, 0.0f}, {0.0f, 1000.0f, 0.0f}, {0.502f, 0.800f, 0.051f, 1.0f}, 2);
        DebugRenderer::DrawLine({0.0f, 0.0f, -1000.0f}, {0.0f, 0.0f, 1000.0f}, {0.153f, 0.525f, 0.918f, 1.0f}, 2);

        static Ref<Mesh> gridPlaneDown = PrimitiveMesh::CreatePlane({1000.0f, 1000.0f});
        static Ref<Mesh> gridPlaneUp = PrimitiveMesh::CreatePlane({1000.0f, -1000.0f}); // FIXME this is a hack to avoid the grid not beeing rendered due to backface culling
        static Ref<Shader> gridShader = Shader::Create("assets/shaders/SimpleGridShader.vert", "assets/shaders/SimpleGridShader.frag");

        Renderer::Submit(gridShader, gridPlaneUp->GetVertexArray());
        Renderer::Submit(gridShader, gridPlaneDown->GetVertexArray());

        // TODO remove testing code
        // const glm::vec3& position, const glm::quat& rotation, glm::vec3& size, glm::vec4 color, const bool& isCentered, float lineWidth
        // DebugRenderer::DrawBox({0,0,0}, glm::quat({0,0,0}), {1,1,1}, {1,1,0,0}, false, 2.0f);

        Renderer::EndOverlay();
    }

    void EditorLayer::ResizeViewport(float width, float height)
    {
        if((m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f) &&
           (width != m_ViewportSize.x || height != m_ViewportSize.y))
        {
            m_EditorCamera.SetViewportSize(width, height);
            Renderer::OnResize((uint32_t)width, (uint32_t)height);
        }

        m_ViewportSize = { width, height };
    }

    void EditorLayer::NewProject()
    {
        Project::New();
    }

    void EditorLayer::OpenProject()
    {
        FileDialogArgs args;
        args.Filters = {{"Coffee Project", "TeaProject"}};
        const std::filesystem::path& path = FileDialog::OpenFile(args);

        if (!path.empty())
        {
            Project::Load(path);
            Application::Get().GetWindow().SetTitle(Project::GetActive()->GetProjectName() + " - Coffee Engine");
        }
        else
        {
            COFFEE_CORE_WARN("Open Project: No file selected");
        }
    }

    void EditorLayer::SaveProject()
    {
        const Ref<Project>& activeProject = Project::GetActive();

        if(activeProject->GetProjectDirectory().empty())
        {
            SaveProjectAs();
            return;
        }

        Project::SaveActive(activeProject->GetProjectDirectory());
    }

    void EditorLayer::SaveProjectAs()
    {
        FileDialogArgs args;
        args.Filters = {{"Coffee Project", "TeaProject"}};
        args.DefaultName = "Untitled.TeaProject";
        const std::filesystem::path& path = FileDialog::SaveFile(args);

        if (!path.empty())
        {
            Project::SaveActive(path);
            Application::Get().GetWindow().SetTitle(Project::GetActive()->GetProjectName() + " - Coffee Engine");
        }
        else
        {
            COFFEE_CORE_WARN("Save Project As: No file selected");
        }
    }

    void EditorLayer::NewScene()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnInit();

        m_SceneTreePanel = SceneTreePanel();

        m_SceneTreePanel.SetContext(m_ActiveScene);
        m_ContentBrowserPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OpenScene()
    {
        FileDialogArgs args;
        args.Filters = {{"Coffee Scene", "TeaScene"}};
        const std::filesystem::path& path = FileDialog::OpenFile(args);

        if (!path.empty())
        {
            m_ActiveScene = Scene::Load(path);
            m_ActiveScene->OnInit();

            m_SceneTreePanel = SceneTreePanel();

            m_SceneTreePanel.SetContext(m_ActiveScene);
            m_ContentBrowserPanel.SetContext(m_ActiveScene);
        }
        else
        {
            COFFEE_CORE_WARN("Open Scene: No file selected");
        }
    }
    void EditorLayer::SaveScene()
    {
        Scene::Save(Project::GetActive()->GetProjectDirectory() / "Untitled.TeaScene", m_ActiveScene);
    }
    void EditorLayer::SaveSceneAs() {}

}