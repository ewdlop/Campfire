#include "RayTracingLayer.h"

#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanRenderer.h"
#include "Vulkan/VulkanTexture.h"
#include "Vulkan/VulkanUtil.h"
#include "Vulkan/VulkanMesh.h"
#include "Vulkan/VulkanInitializers.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Timer.h"
#include "Core/ResourceManager.h" // ASSETS dir
#include "Core/FileSystem.h"
#include "Core/Log.h"

#include "ImGui/VulkanImGuiLayer.h"

#include "Renderer/FrameGraph.h"
#include "Renderer/GlobalInfo.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Component.h"
#include "Scene/CameraController.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>

SharedPtr<cf::Pipeline> CreateRaytracingComputePipeline();

static SharedPtr<Camera> editorCamera;
static CameraController cameraController;
static double frameTime = 0;
static float metricTimer = 0.0;
static global::RenderContext globalInfoCompute;
static global::RenderContext globalInfoGraphics;
static global::RayTraceScene rayTraceScene;
static FrameGraph frameGraph;
static SharedPtr<VulkanVertexBuffer> postProcessVbo;
static SharedPtr<VulkanIndexBuffer> postProcessIbo;
static SharedPtr<cf::Pipeline> graphicsPipeline;
static SharedPtr<cf::Pipeline> computePipeline;
static SharedPtr<cf::Pipeline> postProcessPipeline;
static std::vector<vk::UniqueDescriptorSet> computeResolveDescriptorSet;
static std::vector<vk::UniqueCommandBuffer> computeCmdBuffers;
static vk::UniqueFence computeFence;
static vk::DescriptorImageInfo descriptorImageInfo;
static SharedPtr<VulkanTexture2D> blankTexture;
static SharedPtr<VulkanTexture2D> albedoMap;
static SharedPtr<VulkanTexture2D> metallicMap;

// params.x = frameNumber
// params.y = isWhitted
// params.z = selectedScene
// params.w = isCulmulateSamples
static glm::ivec4 params = glm::ivec4(0);
static bool isWhitted = false;
static bool isCulmulateSamples = true;
static int  selectedScene = 0;
static uint32_t frameNumber;

//========================================================
RayTracingLayer::RayTracingLayer()
    : Layer("RayTracingLayer")
{
    mDevice = VulkanContext::Get()->GetDevice()->GetVulkanDevice();
}

void RayTracingLayer::OnAttach()
{
    scene = CreateSharedPtr<Scene>();

    const int maxLights = 1;
    for (int i = 0; i < maxLights; ++i)
    {
        auto light = scene->CreateEntity("light");
        light.GetComponent<TransformComponent>().position =
            glm::vec3(
                0.0f,
                7.0f,
                0.0f
            );
        light.GetComponent<TransformComponent>().scale =
            glm::vec3(
                2.0f,
                0.0f,
                0.0f
            );
        light.AddComponent<LightComponent>();
        light.GetComponent<LightComponent>().intensity = 5.0f;
    }

    editorCamera = CreateSharedPtr<Camera>(1600, 900, 0.1f, 1000.0f);
    editorCamera->nearPlane = 0.001f;
    editorCamera->farPlane = 10000.0f;
    cameraController.normalSpeed = 5;
    cameraController.SetActiveCamera(
        editorCamera,
        glm::vec3(0.0f, 0.0f, 30.0f), // position
        glm::vec3(0.0f, 0.0f, 0.0f) // euler angles
    );

    //auto environment = scene->CreateEntity("environment");
    //environment.AddComponent<VulkanMeshComponent>(
    //    //ASSETS + "/Models/Sponza/gltf/Sponza.gltf"
    //    ASSETS + "/Models/helmet/scene.gltf"
    //);
    //environment.GetComponent<TransformComponent>().scale = glm::vec3(0.3f);

    // Post-process
    //// TODO replace with just one triangle for projection quad
    float vertices[] =
    {
        -1.0f,  1.0f, 0.0f,     0, 1,   0, 0, 0,
        -1.0f, -1.0f, 0.0f,     0, 0,   0, 0, 0,
         1.0f, -1.0f, 0.0f,     1, 0,   0, 0, 0,
         1.0f,  1.0f, 0.0f,     1, 1,   0, 0, 0,
    };

    uint32_t indices[] =
    {
        0, 1, 2,
        2, 3, 0,
    };

    postProcessVbo = CreateSharedPtr<VulkanVertexBuffer>(vertices, sizeof(vertices));
    postProcessIbo = CreateSharedPtr<VulkanIndexBuffer>(indices, sizeof(indices) / sizeof(uint32_t));

    frameGraph.Prepare();

    VulkanContext::Get()->GetSwapChain()->CreateFramebuffers(frameGraph.GetRenderPass("opaque"));

    //graphicsPipeline = frameGraph.GetPipeline("models");
    postProcessPipeline = frameGraph.GetPipeline("postprocess");
    computePipeline = frameGraph.GetPipeline("raytrace");

    //globalInfoGraphics.Init(graphicsPipeline);
    globalInfoCompute.Init(computePipeline);
    // Create descriptor sets using layout from set 2 from compute pipeline
    rayTraceScene.Init(computePipeline);

    // Compute resolve image descriptor set
    auto descriptorInfo = vk::initializers::DescriptorSetAllocateInfo(
        VulkanContext::Get()->GetDescriptorPool(),
        1,
        &computePipeline->mDescriptorSetLayouts.at(1).get()
    );
    computeResolveDescriptorSet = mDevice.allocateDescriptorSetsUnique(descriptorInfo);
    auto swapChain = VulkanContext::Get()->GetSwapChain();
    ResizeTexture(swapChain->GetWidth(), swapChain->GetHeight());

    // Wall texture
    {
        albedoMap = CreateSharedPtr<VulkanTexture2D>(
            ASSETS + "/Textures/pbr/rusted_iron/albedo.png"
        );
        // Submit texture data to descriptorSet
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = albedoMap->GetImageView();
        imageInfo.sampler = albedoMap->GetSampler();

        vk::WriteDescriptorSet writeInfo{};
        writeInfo.dstSet = computeResolveDescriptorSet.at(0).get();
        writeInfo.dstBinding = 1;
        writeInfo.dstArrayElement = 0;
        writeInfo.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeInfo.descriptorCount = 1;
        writeInfo.pImageInfo = &imageInfo;
        mDevice.updateDescriptorSets(1, &writeInfo, 0, nullptr);
    }

    {
        metallicMap = CreateSharedPtr<VulkanTexture2D>(
            ASSETS + "/Textures/pbr/rusted_iron/metallic.png"
        );

        // Submit texture data to descriptorSet
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = metallicMap->GetImageView();
        imageInfo.sampler = metallicMap->GetSampler();

        vk::WriteDescriptorSet writeInfo{};
        writeInfo.dstSet = computeResolveDescriptorSet.at(0).get();
        writeInfo.dstBinding = 2;
        writeInfo.dstArrayElement = 0;
        writeInfo.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeInfo.descriptorCount = 1;
        writeInfo.pImageInfo = &imageInfo;
        mDevice.updateDescriptorSets(1, &writeInfo, 0, nullptr);
    }

    vk::CommandBufferAllocateInfo cmdBufInfo {};
    cmdBufInfo.commandPool = VulkanContext::Get()->GetCommandPool(QueueFamilyType::COMPUTE);
    cmdBufInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufInfo.commandBufferCount = 1;
    computeCmdBuffers = mDevice.allocateCommandBuffersUnique(cmdBufInfo);
}

void RayTracingLayer::ResizeTexture(uint32_t width, uint32_t height)
{
    blankTexture = CreateSharedPtr<VulkanTexture2D>(width, height);

    descriptorImageInfo.sampler = blankTexture->GetSampler();
    descriptorImageInfo.imageView = blankTexture->GetImageView();
    descriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;

    // Write for compute resolve set
    {
        vk::WriteDescriptorSet writeInfo{};
        writeInfo.dstSet = computeResolveDescriptorSet.at(0).get();
        writeInfo.dstBinding = 0;
        writeInfo.dstArrayElement = 0;
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = vk::DescriptorType::eStorageImage;
        writeInfo.pImageInfo = &descriptorImageInfo;

        mDevice.updateDescriptorSets(1, &writeInfo, 0, nullptr);
    }

    // Write for post process pipeline read
    {
        auto postProcPipeline = frameGraph.GetPipeline("postprocess");
        vk::WriteDescriptorSet writeInfo{};
        // TODO wrap descriptorSet into some API
        // so it can be more easily understood that we're accessing set n
        writeInfo.dstSet = postProcPipeline->mDescriptorSets[0].get();
        writeInfo.dstBinding = 0;
        writeInfo.dstArrayElement = 0;
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeInfo.pImageInfo = &descriptorImageInfo;

        mDevice.updateDescriptorSets(1, &writeInfo, 0, nullptr);
    }
}

void RayTracingLayer::OnDetach()
{
}

void RayTracingLayer::OnUpdate(float dt)
{
    ProcessUserInput();

    metricTimer -= dt;

    timer.Reset();

    // Current frame in flight
    auto frameIdx = VulkanContext::Get()->GetSwapChain()->GetCurrentImageIndex();

    // Update camera
    cameraController.OnUpdate(dt);

    VulkanImGuiLayer* vkImguiLayer = Application::Get().imguiLayer;

    globalInfoCompute.Update(editorCamera, scene, frameIdx);
    //globalInfoGraphics.Update(editorCamera, scene, frameIdx);

    // TODO this should be called from the application loop
    vkImguiLayer->Begin();
    OnImGuiRender();
    vkImguiLayer->End();

    computePipeline = frameGraph.GetPipeline("raytrace");
    auto computeCmdBuf = computeCmdBuffers.at(0).get();

    std::vector<vk::DescriptorSet> computeDescriptorSets = {
        globalInfoCompute.mDescriptorSets.at(0).get(), // Set0 - camera,lights
        computeResolveDescriptorSet.at(0).get(),       // Set1 - resolve image
        rayTraceScene.mDescriptorSets.at(0).get(),     // Set2 - scene data
    };

    vk::CommandBufferBeginInfo beginInfo {};
    computeCmdBuf.begin(beginInfo);

        computeCmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            computePipeline->mPipelineLayout.get(),
            0, // first set
            static_cast<uint32_t>(computeDescriptorSets.size()), // descriptor set count
            computeDescriptorSets.data(),
            0, // dynamicOffsetCount
            nullptr // pDyanmicOffsets
        );

        params.x = frameNumber;
        computeCmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline->mPipeline.get());
        // Send rendering parameters via push constant
        computeCmdBuf.pushConstants(
            computePipeline->mPipelineLayout.get(),
            vk::ShaderStageFlagBits::eCompute,
            0, sizeof(glm::vec4),
            &params);
        // Increment frame number
        frameNumber++;

        computeCmdBuf.dispatch(blankTexture->GetWidth() / 16, blankTexture->GetHeight() / 16, 1);
    computeCmdBuf.end();

    // Submit compute command
    vk::SubmitInfo computeSubmitInfo{};
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &computeCmdBuffers.at(0).get();

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    computeFence = mDevice.createFenceUnique(fenceInfo);

    mDevice.waitForFences(computeFence.get(), VK_TRUE, UINT64_MAX);
    mDevice.resetFences(computeFence.get());
    auto computeQueue = VulkanContext::Get()->GetDevice()->GetQueue(QueueFamilyType::COMPUTE);
    computeQueue.submit(computeSubmitInfo, computeFence.get());

    auto swapChainSize = VulkanContext::Get()->GetSwapChain()->GetImages().size();
    for (size_t frame = 0; frame < swapChainSize; ++frame)
    {
        auto commandBuffer = VulkanRenderer::BeginScene(frame, frameGraph.GetRenderPass("opaque"));
        {
            // TODO read from descriptor with compute image
            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                postProcessPipeline->mPipelineLayout.get(),
                0, // set
                1, // size
                &postProcessPipeline->mDescriptorSets[0].get(),
                0,
                nullptr
            );

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, postProcessPipeline->mPipeline.get());

            /*
            std::vector<vk::DescriptorSet> descriptorSets{
                globalInfoGraphics.mDescriptorSets.at(0).get(),
            };

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                graphicsPipeline->mPipelineLayout.get(),
                0,
                static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(),
                0, nullptr
            );
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline->mPipeline.get());
            */

            mPushConstBlock.model = glm::mat4(1.0f);
            commandBuffer.pushConstants(
                postProcessPipeline->mPipelineLayout.get(),
                vk::ShaderStageFlagBits::eVertex,
                0, sizeof(VulkanGraphicsPipeline::TransformPushConstBlock),
                &mPushConstBlock);


            // TODO render off screen
            /*
            auto group = scene->registry.group<VulkanMeshComponent>(entt::get<TransformComponent, TagComponent>);
            for (auto entity : group)
            {
                auto [transformComponent, meshComponent, tagComponent] = group.get<TransformComponent, VulkanMeshComponent, TagComponent>(entity);

                mPushConstBlock.model = transformComponent;
                commandBuffer.pushConstants(
                    graphicsPipeline->mPipelineLayout.get(),
                    vk::ShaderStageFlagBits::eVertex,
                    0, sizeof(VulkanGraphicsPipeline::TransformPushConstBlock),
                    &mPushConstBlock);

                // Draw mesh
                meshComponent.mesh->Draw(commandBuffer, frame);
            }
            */

            // Post process quad
            VulkanRenderer::DrawIndexed(
                commandBuffer,
                postProcessVbo->GetBuffer(),
                postProcessIbo->GetBuffer(),
                postProcessIbo->GetCount()
            );

            vkImguiLayer->mImGuiImpl->DrawFrame(commandBuffer);
        }
        VulkanRenderer::EndScene(commandBuffer);
    }


    if (metricTimer <= 0.0)
    {
        frameTime = timer.GetTime();
        metricTimer = 1.0f;
    }
}

void RayTracingLayer::ProcessUserInput()
{
    if (Input::GetMod(MOD_KEY_CONTROL) && Input::GetKeyDown(KEY_R))
    {
        ReconstructPipelines();
        //LOG_INFO("Finish reconstructing pipelines");
        CORE_INFO("Finish reconstructing pipelines");
    }

    if (Input::GetMouseButton(MOUSE_BUTTON_RIGHT))
    {
        // Reset frame number per user movement
        frameNumber = 0;
    }

    // Reset camera position
    if (Input::GetKeyDown(KEY_F))
    {
        cameraController.ResetActiveCamera(
            editorCamera,
            glm::vec3(0.0f, 0.0f, 30.0f), // position
            glm::vec3(0.0f, 0.0f, 0.0f)   // euler angles
        );
        frameNumber = 0;
    }
}

void RayTracingLayer::OnImGuiRender()
{
    static bool showHierarchy = true;
    static bool showInspector = true;
    mHierarchyWidget.ShowHierarchy(scene, editorCamera, &showHierarchy);
    auto & selectedEntity = mHierarchyWidget.GetSelectedEntity();
    if (selectedEntity)
    {
        mInspectorWidget.ShowInspector(selectedEntity, &showInspector);

        //if (/*allowViewportCameraEvents &&*/ !Input::GetMouseButtonDown(MOUSE_BUTTON_RIGHT))
        //{
        //    if (Input::GetKeyDown(KEY_W))
        //        mTransformWidget.operation = ImGuizmo::TRANSLATE;
        //    if (Input::GetKeyDown(KEY_E))
        //        mTransformWidget.operation = ImGuizmo::ROTATE;
        //    if (Input::GetKeyDown(KEY_R))
        //        mTransformWidget.operation = ImGuizmo::SCALE;
        //}
        //mTransformWidget.EditTransform(selectedEntity, *editorCamera);
    }

    ImGui::Begin("Metrics");
    {
        auto gpu = VulkanContext::Get()->GetDevice()->GetVulkanPhysicalDevice();
        auto props = gpu.getProperties();
        ImGui::Text(props.deviceName);
        std::string timerText(std::to_string(frameTime) + " ms/frame");
        ImGui::Text(timerText.c_str());
        ImGui::Separator();
    }
    ImGui::End();

    ImGui::Begin("Controls");
    {
        ImGui::Separator();

        ImGui::Text("Rendering Params");

        { // Auto actions when toggling whitted
            bool lastValue = isWhitted;
            ImGui::Checkbox("Direct Illumination", &isWhitted);
            params.y = static_cast<float>(isWhitted);

            if (lastValue != isWhitted)
            {
                // Auto switch off culmulate samples when we switch to whitted
                if (lastValue == false)
                    isCulmulateSamples = false;
                // switch on culmulate samples when switching to path tracer
                else {
                    isCulmulateSamples = true;
                }
                frameNumber = 0;
            }
        }

        { // Refresh frame when we toggle isCulmulateSamples
            bool lastValue = isCulmulateSamples;
            ImGui::Checkbox("Accumulate Samples", &isCulmulateSamples);
            params.w = static_cast<float>(isCulmulateSamples);
            if (lastValue != isCulmulateSamples)
                frameNumber = 0;
        }

        { // Refresh frame when we select new scene
            uint32_t lastValue = selectedScene;
            ImGui::RadioButton("Scene A", &selectedScene, 0); ImGui::SameLine();
            ImGui::RadioButton("Scene B", &selectedScene, 1); ImGui::SameLine();
            ImGui::RadioButton("MyTest", &selectedScene, 2); ImGui::SameLine();
            params.z = selectedScene;
            if (lastValue != selectedScene)
                frameNumber = 0;
        }

        ImGui::Separator();

        ImGui::Text("Camera");
        //ImGui::DragFloat3("Controller Position", (float*)&cameraController.position);
        //ImGui::DragFloat3("Camera Position", (float*)&editorCamera->pos);
        ImGui::DragFloat("Speed", &cameraController.normalSpeed);
        ImGui::DragFloat("Near", &editorCamera->nearPlane);
        ImGui::DragFloat("Far", &editorCamera->farPlane);

        ImGui::Separator();

        ImGui::Text("Inverse View Matrix");
        glm::mat4 invView = glm::inverse(editorCamera->GetViewMatrix());
        ImGui::DragFloat4("", (float*)&invView[0], 0.01f);
        ImGui::DragFloat4("", (float*)&invView[1], 0.01f);
        ImGui::DragFloat4("", (float*)&invView[2], 0.01f);
        ImGui::DragFloat4("", (float*)&invView[3], 0.01f);

        ImGui::Separator();
    }
    ImGui::End();
}

void RayTracingLayer::OnEvent(Event& event)
{
    cameraController.OnEvent(event);
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RayTracingLayer::OnWindowResize));
}

void RayTracingLayer::ReconstructPipelines()
{
    auto ctx = VulkanContext::Get();
    ctx->GetDevice()->GetVulkanDevice().waitIdle();

    // TODO reconstruct async
    frameGraph.ReconstructFrameGraph();
}

bool RayTracingLayer::OnWindowResize(WindowResizeEvent& e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0) {
        return true;
    }

    editorCamera->width = e.GetWidth();
    editorCamera->height = e.GetHeight();
    editorCamera->SetProjection();

    VulkanContext::Get()->RecreateSwapChain(frameGraph.GetRenderPass("opaque"));
    ResizeTexture(e.GetWidth(), e.GetHeight());
    frameNumber = 0;

    return false;
}

SharedPtr<cf::Pipeline> CreateRaytracingComputePipeline()
{
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindings(3);

    { // Environment descriptors
        // Camera
        auto camera = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eCompute,
            0);

        // Lights
        auto lights = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eCompute,
            1);

        // Set 0
        descriptorSetLayoutBindings[0] = {
            camera,
            lights,
        };
    }

    { // Compute Resolve
        auto computeResolve = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            0);

        auto albedoMap = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            1);

       auto metallicMap = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            2);

        // Set 1
        descriptorSetLayoutBindings[1] = {
            computeResolve,
            albedoMap,
            metallicMap,
        };
    }

    { // Scene data
        // Spheres
        auto spheres = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eCompute,
            0);

        // Quads
        auto quads = vk::initializers::DescriptorSetLayoutBinding(
            vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eCompute,
            1);

        // Set 2
        descriptorSetLayoutBindings[2] = {
            spheres,
            quads
        };
    }

    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSets = { descriptorSetLayoutBindings };

    auto shader = CreateSharedPtr<VulkanShader>(SHADERS + "/raytrace.comp");
    vk::PipelineShaderStageCreateInfo shaderInfo {};
    shaderInfo.stage  = vk::ShaderStageFlagBits::eCompute;
    shaderInfo.module = shader->GetShaderModule();
    shaderInfo.pName  = "main";

    return CreateSharedPtr<cf::Pipeline>(descriptorSets, shaderInfo);
}
