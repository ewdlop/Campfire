#include <imgui.h>
#include <imgui_stdlib.h>
#include <type_traits>
#include <fstream>
#include <iomanip>
#include <lua.hpp>

#include "Scene/Component.h"
#include "Scene/Entity.h"
#include "Widgets/InspectorWidget.h"
#include "Core/FileSystem.h"
#include "Renderer/Material.h"
#include "Scene/SceneManager.h"
#include "Command/CommandManager.h"
//#include "Scripting/LuaManager.h"

void InspectorWidget::ShowInspector(Entity& entity, bool* isOpen)
{
    ImGui::Begin("Inspector", isOpen);

    if (entity) { ShowEntity(entity); }

    ImGui::End();
}

void InspectorWidget::ShowEntity(Entity& entity)
{
    int isActiveID = 0;
    ImGui::PushID(isActiveID++);
    //ImGui::Checkbox("", &entity.GetComponent<ActiveComponent>().isActive);
    ImGui::PopID();
    ImGui::SameLine();

    if (ImGui::Button("Save Prefab"))
    {
        std::string path = FileSystem::SaveFile("Prefab Files(*.prefab)\0");
        if (!path.empty())
        {
            json eJson = SceneManager::SerializeEntity(entity);
            std::ofstream out(path);
            out << std::setw(2) << eJson << std::endl;
            out.close();
        }
    }

    // TODO change tag to be a name
    // and tag later is an identifier for filtering objects
    // e.g. player, enemy tags
    if (entity.HasComponent<TagComponent>())
    {
        auto& tagComp = entity.GetComponent<TagComponent>();
        static std::string origTag;
        ImGui::InputText("Name", &tagComp.tag);
        //if (ImGui::IsItemActivated())
        //{
        //    origTag = tagComp.tag;
        //}
        //if (ImGui::IsItemDeactivatedAfterEdit())
        //{
        //    //TODO : handle control z conflicts
        //    //CommandManager::Execute(std::make_unique<ImGuiStringCommand>(tagComp.tag, origTag, tagComp.tag));
        //}
        ImGui::Separator();
    }

    // Transform
    if (entity.HasComponent<TransformComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Transform"))
        {
            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }
            auto& transform = entity.GetComponent<TransformComponent>();
            static glm::vec3 oldPos;
            static glm::vec3 oldEuler;
            static glm::vec3 oldScale;

            ImGui::DragFloat3("Position", (float*)&transform.position, 0.01f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldPos = transform.position;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                //cannot bind static variables...
                //cannot pass these value by reference neither....
                glm::vec3 tempOldPos = oldPos;
                glm::vec3 tempNewPos = transform.position;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewPos]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().position = tempNewPos;},
                    [&entity, tempOldPos]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().position = tempOldPos; }));
            }

            ImGui::DragFloat3("Rotation", (float*)&transform.euler, 0.01f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldEuler = transform.euler;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                glm::vec3 tempOldRotation = oldEuler;
                glm::vec3 tempNewRotation = transform.euler;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewRotation]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().euler = tempNewRotation;},
                    [&entity, tempOldRotation]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().euler = tempOldRotation;}));
            }
            ImGui::DragFloat3("Scale", (float*)&transform.scale, 0.01f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldScale = transform.scale;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                glm::vec3 tempOldScale = oldScale;
                glm::vec3 tempNewScale = transform.scale;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewScale]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().scale = tempNewScale;},
                    [&entity, tempOldScale]() {if (entity.IsValid())entity.GetComponent<TransformComponent>().scale = tempOldScale;}));
            }

            // NOTE: Popup should be put after the inspector options
            // Since it's possible the user can remove the component
            // hence causing a nullptr exception when viewing component fields
            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<TransformComponent>(entity);
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Camera
    if (entity.HasComponent<CameraComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Camera"))
        {
            auto& comp = entity.GetComponent<CameraComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            SharedPtr<Camera> camera = comp.camera;

            const char* clearFlags[] = { "Skybox", "Solid Color", "Depth Only", "Don't Clear" };
            auto previousClearFlag = static_cast<int>(camera->clearFlag);
            auto currClearFlag = static_cast<int>(camera->clearFlag);
            if(ImGui::Combo("Clear Flags", &currClearFlag, clearFlags, IM_ARRAYSIZE(clearFlags)))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity, currClearFlag]() {
                        if (entity.IsValid() && entity.HasComponent<CameraComponent>())
                        {
                            entity.GetComponent<CameraComponent>().camera->clearFlag = static_cast<ClearFlag>(currClearFlag);
                        }
                    },
                    [&entity, previousClearFlag]() {
                        if (entity.IsValid() && entity.HasComponent<CameraComponent>())
                        {
                            entity.GetComponent<CameraComponent>().camera->clearFlag = static_cast<ClearFlag>(previousClearFlag);
                        }
                    }));
            }
            glm::vec4 oldColor = camera->backgroundColor;
            ImGui::ColorEdit4("Background", (float*)&camera->backgroundColor);
            bool prevState = camera->isPerspective;
            ImGui::Checkbox("Is Perspective", &camera->isPerspective);

            const char* displays[] = { "Display 1", "Display 2", "Display 3", "Display 4", "Display 5" };
            ImGui::Combo("Target Display", (int*)&camera->targetDisplay, displays, IM_ARRAYSIZE(displays));

            if (prevState != camera->isPerspective)
                camera->SetProjection();

            if (camera->isPerspective)
                ImGui::DragFloat("Vertical FOV", &camera->vFov, 0.1f);
            else
                ImGui::DragFloat("Size", &camera->size, 0.1f);

            ImGui::DragFloat("Near Plane", &camera->nearPlane, 0.1f);
            ImGui::DragFloat("Far Plane", &camera->farPlane, 0.1f);
            ImGui::DragFloat("Depth", &camera->depth, 0.1f);

            ImGui::Text("Viewport Rect");
            ImGui::DragFloat("x", &camera->x, 0.1f);
            ImGui::DragFloat("y", &camera->y, 0.1f);
            ImGui::DragFloat("w", &camera->width, 0.1f);
            ImGui::DragFloat("h", &camera->height, 0.1f);

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<CameraComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Text
    if (entity.HasComponent<TextComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Text"))
        {
            auto& comp = entity.GetComponent<TextComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            SharedPtr<Text> textRef = entity.GetComponent<TextComponent>();

            ImGui::Checkbox("isUI", &textRef->isUI);

            // NDC positions for UI
            if (textRef->isUI)
            {
                ImGui::DragFloat2("Pos", (float*)&textRef->uiPos, 0.01f);
            }

            ImGui::ColorEdit4("Color", (float*)&textRef->color);
            if ( ImGui::Button( textRef->GetFont()->GetPath().c_str() ) )
            {
                // TODO
            }

            int min = 0; int max = 255;
            ImGui::DragScalar("Font Size", ImGuiDataType_U32, &textRef->fontSize, 0.03f, &min, &max);
            ImGui::NewLine();
            ImGui::InputText("Display Text", &textRef->text);

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<TextComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Mesh
    if (entity.HasComponent<MeshComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Mesh"))
        {
            auto& comp = entity.GetComponent<MeshComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            auto& meshComp = entity.GetComponent<MeshComponent>();
            auto& mesh = meshComp.mesh;
            if (ImGui::Button("Load Mesh"))
            {
                std::string newPath = FileSystem::OpenFile();
                if (!newPath.empty())
                {
                    mesh = Mesh::Create(newPath);
                }
            }

            ImGui::SameLine();

            if (mesh)
            {
                mesh->OnImGuiRender();
            }
            else
            {
                ImGui::Text("Empty Mesh");
            }

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<MeshComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Sprite
    if (entity.HasComponent<SpriteComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Sprite"))
        {
            auto& comp = entity.GetComponent<SpriteComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            ImGui::Text(comp.sprite->GetName().c_str());
            if (ImGui::ImageButton((ImTextureID)comp.sprite->GetRenderID(), ImVec2(128, 128), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
            {
                std::string path = FileSystem::OpenFile();
                if (!path.empty())
                {
                    comp.sprite = Texture2D::Create(path);
                }
            }
            ImGui::ColorEdit4("Color", (float*)&comp.color);

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<SpriteComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Rigidbody
    if (entity.HasComponent<RigidbodyComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Rigidbody"))
        {
            auto& comp = entity.GetComponent<RigidbodyComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            auto& rigidbody = entity.GetComponent<RigidbodyComponent>().rigidbody;

            const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
            int previousType = static_cast<int>(rigidbody->type);
            int currType = static_cast<int>(rigidbody->type);
            if(ImGui::Combo("Type", &currType, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&rigidbody, currType]() {rigidbody->type = static_cast<Rigidbody::BodyType>(currType); },
                    [&rigidbody, previousType]() {rigidbody->type = static_cast<Rigidbody::BodyType>(previousType); }));
            }
            //ImGui::SameLine(); HelpMarker("Static for non-movable objects. Kinematic for objects that player will move. Dynamic for objects that are moved by the engine.\n");

            static float oldMass;
            static float oldDrag;
            static float oldAngularDrag;
            static bool oldUseGravity = false;
            ImGui::DragFloat("Mass", &rigidbody->mass, 0.1f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldMass = rigidbody->mass;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                float tempOldMass = oldMass;
                float tempNewMass = rigidbody->mass;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewMass]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->mass = tempNewMass;
                    }
                },
                    [&entity, tempOldMass]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->mass = tempOldMass;
                    }
                }));
            }
            ImGui::DragFloat("Drag", &rigidbody->drag, 0.1f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldDrag = rigidbody->drag;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                float tempOldDrag = oldDrag;
                float tempNewDrag = rigidbody->drag;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewDrag]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->drag = tempNewDrag;
                    }
                },
                    [&entity, tempOldDrag]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->mass = tempOldDrag;
                    }
                }));
            }
            ImGui::DragFloat("Angular Drag", &rigidbody->angularDrag, 0.1f);
            if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(0))
            {
                oldAngularDrag = rigidbody->angularDrag;
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                float tempOldAngularDrag = oldAngularDrag;
                float tempNewAngularDrag = rigidbody->angularDrag;
                CommandManager::Execute(std::make_unique<SkippedExecuteActionCommand>(
                    [&entity, tempNewAngularDrag]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->angularDrag = tempNewAngularDrag;
                    }
                },
                    [&entity, tempOldAngularDrag]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->angularDrag = tempOldAngularDrag;
                    }
                }));
            }
            if (ImGui::Checkbox("Use Gravity", &rigidbody->useGravity))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleGravity();
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleGravity();
                    }
                }));
            }
            ImGui::PushID(0);
            ImGui::Text("Freeze Position");
            ImGui::SameLine();
            if (ImGui::Checkbox("X", &rigidbody->freezePosition[0]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(0);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(0);
                    }
                }));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Y", &rigidbody->freezePosition[1]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(1);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(1);
                    }
                }));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Z", &rigidbody->freezePosition[2]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(2);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(2);
                    }
                }));
            }
            ImGui::PopID();

            ImGui::PushID(1);
            ImGui::Text("Freeze Rotation");
            ImGui::SameLine();
            if (ImGui::Checkbox("X", &rigidbody->freezeRotation[0]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezeRotation(0);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(0);
                    }
                }));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Y", &rigidbody->freezeRotation[1]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezeRotation(1);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(1);
                    }
                }));
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Z", &rigidbody->freezeRotation[2]))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        //show we load the saved or toggle?
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezeRotation(2);
                    }
                },
                    [&entity]() {
                    if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())
                    {
                        entity.GetComponent<RigidbodyComponent>().rigidbody->ToggleFreezePositon(2);
                    }
                }));
            }
            ImGui::PopID();

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<RigidbodyComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Colliders
    if (entity.HasComponent<Colliders>())
    {
        auto& colliders = entity.GetComponent<Colliders>().list;
        for (size_t i = 0; i < colliders.size(); ++i)
        {
            auto& collider = colliders[i];

            ImGui::PushID(i);
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            // Collider
            //if (ImGui::TreeNode(colliderComponent.GetShapeTypeString().c_str()))
            if (ImGui::TreeNode("Collider Shape"))
            {
                ImGui::PushID(isActiveID++);
                ImGui::Checkbox("", &collider->isActive);
                ImGui::PopID();
                ImGui::SameLine();

                if (ImGui::Button("..."))
                {
                    ImGui::OpenPopup("ComponentListOptionsPopup");
                }

                ImGui::Checkbox("isTrigger", &collider->isTrigger);

                const char* colliderTypes[] = { "Box", "Sphere", "Capsule", "Box2D" };
                int currType = static_cast<int>(collider->type);
                ImGui::Combo("Type", &currType, colliderTypes, IM_ARRAYSIZE(colliderTypes));
                // Reinit the collider shape if it changes
                if (currType != static_cast<int>(collider->type))
                {
                    collider = Collider::Create(static_cast<Collider::Shape>(currType));
                }

                ImGui::DragFloat3("Center", (float*)&collider->center, 0.1f);

                // For showing specific shape parameters
                collider->ShowData();

                if (ImGui::BeginPopup("ComponentListOptionsPopup"))
                {
                    ShowComponentListOptionsMenu<Colliders>(entity, i);
                    ImGui::EndPopup();
                }

                ImGui::TreePop();
            }
            ImGui::PopID();

            ImGui::Separator();
        }
    }

    // Light
    if (entity.HasComponent<LightComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Light"))
        {
            auto& comp = entity.GetComponent<LightComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            const char* lightTypes[] = { "Directional", "Point", "Spot", "Area" };
            auto currType = static_cast<int>(comp.type);
            ImGui::Combo("Type", (int*)&currType, lightTypes, IM_ARRAYSIZE(lightTypes));
            comp.type = static_cast<LightComponent::LightType>(currType);

            ImGui::ColorEdit4("Light Color", (float*)&comp.color);

            // TODO reorganize this with range, type, etc
            //ImGui::DragFloat("Range", &light.linear, 0.01f);

            ImGui::DragFloat("Intensity", &comp.intensity, 0.1f, 0.0f, 1000.0f);

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<LightComponent>(entity);
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Particle System
    if (entity.HasComponent<ParticleSystemComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Particle System"))
        {
            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            auto& psPtr = entity.GetComponent<ParticleSystemComponent>().ps;
            psPtr->OnImGuiRender();

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<ParticleSystemComponent>(entity);
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }


    // Audio
    if (entity.HasComponent<AudioComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Audio Source"))
        {
            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            auto& audioSource = entity.GetComponent<AudioComponent>().audioSource;
            audioSource->OnImGuiRender();

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<AudioComponent>(entity);
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    // Native Script
    if (entity.HasComponent<NativeScriptComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Native Script"))
        {
            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            // TODO

            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<NativeScriptComponent>(entity);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
    }


    // Script
    if (entity.HasComponent<ScriptComponent>())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Script"))
        {
            auto& comp = entity.GetComponent<ScriptComponent>();
            ImGui::PushID(isActiveID++);
            ImGui::Checkbox("", &comp.isActive);
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                ImGui::OpenPopup("ComponentOptionsPopup");
            }

            //ShowJsonObject(LuaManager::GetLuaGlobal());
            
            auto& sc = entity.GetComponent<ScriptComponent>();

            std::string filename = sc.filepath.empty() ? "Blank" : sc.filepath;
            if (ImGui::Button(filename.c_str()))
            {
                std::string path = FileSystem::OpenFile("*.lua");
                if (!path.empty())
                {
                    CommandManager::Execute(std::make_unique<ActionCommand>(
                        [&entity,path]() {if(entity.IsValid() && entity.HasComponent<ScriptComponent>())entity.GetComponent<ScriptComponent>().filepath = path; },
                        [&entity,filename]() {if (entity.IsValid() && entity.HasComponent<ScriptComponent>())entity.GetComponent<ScriptComponent>().filepath = filename; }));
                }
            }

            lua_State* L = luaL_newstate();
            if ((luaL_loadfile(L, sc.filepath.c_str()) | lua_pcall(L, 0, LUA_MULTRET, 0)) == LUA_OK)//check syntax error
            {
                if (lua_getglobal(L, "Update") && lua_isfunction(L,-1))
                {
                    ImGui::Checkbox("Run Update", &(sc.runUpdate));
                }

                if (lua_getglobal(L, "OnTriggerEnter") && lua_isfunction(L, -1))
                {
                    ImGui::Checkbox("Run OnTriggerEnter", &(sc.runOnTriggerEnter));
                }

                if (lua_getglobal(L, "OnTriggerStay") && lua_isfunction(L, -1))
                {
                    ImGui::Checkbox("Run OnTriggerStay", &(sc.runOnTriggerStay));
                }

                if (lua_getglobal(L, "OnTriggerExit") && lua_isfunction(L,-1))
                {
                    ImGui::Checkbox("Run OnTriggerExit", &(sc.runOnTriggerExit));
                }
            }
            lua_close(L);
            
            if (ImGui::BeginPopup("ComponentOptionsPopup"))
            {
                ShowComponentOptionsMenu<ScriptComponent>(entity);
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }


    ImGui::NewLine();
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("ComponentMenuPopup");
    }
    ImGui::PopStyleColor(1);
    if (ImGui::BeginPopup("ComponentMenuPopup"))
    {
        ShowComponentMenu(entity);
        ImGui::EndPopup();
    }
}

void InspectorWidget::ShowJsonObject(const nlohmann::json& json)
{
    for (auto& [key, value] : json.items())
    {
        if (value.is_number_integer())
        {
            int nubmer = value;
            ImGui::Text("%s: %i", key.c_str(), nubmer);
        }
        else if (value.is_number_float())
        {
            float nubmer = value.get<float>();
            ImGui::Text("%s: %f", key.c_str(), nubmer);
        }
        else if (value.is_string())
        {
            std::string text = value;
            ImGui::Text("%s: %s", key.c_str(), text.c_str());
        }
        else if (value.is_boolean())
        {
            if (value.get<bool>())
            {
                ImGui::Text("%s: true", key.c_str());
            }
            else
            {
                ImGui::Text("%s: false", key.c_str());
            }
        }
        else if (value.is_array())
        {
            if (ImGui::TreeNode(key.c_str()))
            {
                ShowJsonObject(value);
                ImGui::TreePop();
            }
        }
        else if (value.is_object())
        {
            if (ImGui::TreeNode(key.c_str()))
            {
                ShowJsonObject(value);
                ImGui::TreePop();
            }
        }
    }
}

void InspectorWidget::ShowComponentMenu(Entity& entity)
{
    if (!entity.HasComponent<AudioComponent>())
    {
        if (ImGui::MenuItem("Audio"))
        {
            CommandManager::Execute(std::make_unique<ActionCommand>(
                [&entity]() {if (entity.IsValid())entity.AddComponent<AudioComponent>(); },
                [&entity]() {if (entity.IsValid() && entity.HasComponent<AudioComponent>())entity.RemoveComponent<AudioComponent>(); }));
        }
    }

    if (ImGui::BeginMenu("Effects"))
    {
        if (!entity.HasComponent<ParticleSystemComponent>())
        {
            if (ImGui::MenuItem("Particle System"))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {if (entity.IsValid())entity.AddComponent<ParticleSystemComponent>(); },
                    [&entity]() {if (entity.IsValid() && entity.HasComponent<ParticleSystemComponent>())entity.RemoveComponent<ParticleSystemComponent>(); }));
            }
        }
        ImGui::EndMenu();
    }

    // The object should either be a sprite or a mesh
    if (!entity.HasComponent<MeshComponent>() && !entity.HasComponent<SpriteComponent>())
    {
        if (ImGui::MenuItem("Mesh"))
        {
            CommandManager::Execute(std::make_unique<ActionCommand>(
                [&entity]() {if (entity.IsValid())entity.AddComponent<MeshComponent>(); },
                [&entity]() {if (entity.IsValid() && entity.HasComponent<MeshComponent>())entity.RemoveComponent<MeshComponent>(); }));
        }
        else if (ImGui::MenuItem("Sprite"))
        {
            CommandManager::Execute(std::make_unique<ActionCommand>(
                [&entity]() {if (entity.IsValid())entity.AddComponent<SpriteComponent>(); },
                [&entity]() {if (entity.IsValid() && entity.HasComponent<SpriteComponent>())entity.RemoveComponent<SpriteComponent>(); }));
        }
    }

    if (ImGui::BeginMenu("Physics"))
    {
        if (!entity.HasComponent<RigidbodyComponent>())
        {
            if (ImGui::MenuItem("Rigidbody"))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {if (entity.IsValid())entity.AddComponent<RigidbodyComponent>(); },
                    [&entity]() {if (entity.IsValid() && entity.HasComponent<RigidbodyComponent>())entity.RemoveComponent<RigidbodyComponent>(); }));
            }
        }

        //if (!entity.HasComponent<TriggerComponent>())
        //{
        //    if (ImGui::MenuItem("Trigger"))
        //    {
        //        entity.AddComponent<TriggerComponent>();
        //    }
        //}

        if (ImGui::BeginMenu("Colliders"))
        {
            if (!entity.HasComponent<Colliders>())
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {if (entity.IsValid())entity.AddComponent<Colliders>(); },
                    [&entity]() {if (entity.IsValid() && entity.HasComponent<Colliders>())entity.RemoveComponent<Colliders>(); }));
            }

            // TODO 2D collider shapes
            auto& cols = entity.GetComponent<Colliders>().list;

            if (ImGui::MenuItem("Box Collider"))
            {
                cols.emplace_back(Collider::Create(Collider::Shape::BOX));
            }
            if (ImGui::MenuItem("Sphere Collider"))
            {
                cols.emplace_back(Collider::Create(Collider::Shape::SPHERE));
            }
            if (ImGui::MenuItem("Capsule Collider"))
            {
                // TODO
                //auto& comp = entity.GetComponent<Colliders>();
                //comp.emplace_back(Collider::Create(Collider::Shape::SPHERE));
            }
            if (ImGui::MenuItem("Box2D"))
            {
                cols.emplace_back(Collider::Create(Collider::Shape::BOX_2D));
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if (!entity.HasComponent<LightComponent>())
    {
        if (ImGui::BeginMenu("Light"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                entity.AddComponent<LightComponent>(LightComponent::LightType::DIRECTIONAL);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                entity.AddComponent<LightComponent>(LightComponent::LightType::POINT);
            }
            // TODO
            if (ImGui::MenuItem("Spot Light"))
            {
                entity.AddComponent<LightComponent>(LightComponent::LightType::SPOT);
            }
            // TODO
            if (ImGui::MenuItem("Area Light"))
            {
                entity.AddComponent<LightComponent>(LightComponent::LightType::AREA);
            }

            ImGui::EndMenu();
        }

        // TODO move this for the other lights
        //if (ImGui::MenuItem("Light"))
        //{
        //    CommandManager::Execute(std::make_unique<ActionCommand>(
        //        [&entity]() {if(entity.IsValid())entity.AddComponent<LightComponent>(); },
        //        [&entity]() {if(entity.IsValid() && entity.HasComponent<LightComponent>()) entity.RemoveComponent<LightComponent>(); }));
        //}
    }

    if (!entity.HasComponent<ScriptComponent>())
    {
        if (ImGui::BeginMenu("Scripts"))
        {
            if (ImGui::BeginMenu("Available"))
            {
                // listing all available scripts in Assets dir
                auto scriptPaths = FileSystem::GetAllFiles(ASSETS.c_str(), ".lua");
                for (auto const& scriptPath : scriptPaths)
                {
                    if (ImGui::MenuItem(scriptPath.filename().string().c_str()))
                    {
                        entity.AddComponent<ScriptComponent>();
                        entity.GetComponent<ScriptComponent>().filepath = scriptPath.string();
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("New Script"))
            {
                CommandManager::Execute(std::make_unique<ActionCommand>(
                    [&entity]() {if (entity.IsValid())entity.AddComponent<ScriptComponent>(); },
                    [&entity]() {if (entity.IsValid() && entity.HasComponent<ScriptComponent>())entity.RemoveComponent<ScriptComponent>(); }));
            }

            ImGui::EndMenu();
        }
    }

    if (!entity.HasComponent<CameraComponent>())
    {
        if (ImGui::MenuItem("Camera"))
        {
            CommandManager::Execute(std::make_unique<ActionCommand>(
                [&entity]() {if (entity.IsValid())entity.AddComponent<CameraComponent>(); },
                [&entity]() {if (entity.IsValid())entity.RemoveComponent<CameraComponent>(); }));
        }
    }
}

template <typename T>
void InspectorWidget::ShowComponentOptionsMenu(Entity& entity)
{
    if (ImGui::MenuItem("Reset"))
    {
        entity.GetComponent<T>().Reset();
    }

    ImGui::Separator();

    // All objects in scene should have a transform component
    // so prevent removal of just the transform component
    if (!std::is_same<T, TransformComponent>::value)
    {
        if (ImGui::MenuItem("Remove Component"))
        {
            //TODO: add the setting restoration capability
            //CommandManager::Execute(std::make_unique<ActionCommand>(
            //    [&entity]() {entity.RemoveComponent<T>(); },
            //    [&entity]() {entity.AddComponent<T>();}));
        }
    }
    if (ImGui::MenuItem("Copy Component"))
    {
    }

    ImGui::Separator();

    // Component specific options
}

template <typename T>
void InspectorWidget::ShowComponentListOptionsMenu(Entity& entity, size_t index)
{
    auto& list = entity.GetComponent<T>().list;
    if (ImGui::MenuItem("Reset"))
    {
        list.at(index)->Reset();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Remove Component"))
    {
        list.erase(list.begin() + index);
    }

    ImGui::Separator();
}
