#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "Core/Application.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiLayer.h"

#include "Core/Log.h"
#include "Core/Time.h"
#include "Core/Random.h"

#include "Renderer/Renderer.h"
#include "Physics/PhysicsManager.h"
#include "Command/CommandManager.h"

#include "Audio/AudioSystem.h"
//#include "JobSystem/JobSystem.h"
//#include "Scripting/LuaManager.h"
#include "Core/ProcessorInfo.h"
#include "Core/RCCpp.h"
#include "ImGui/VulkanImGuiLayer.h"

Application* Application::instance = nullptr;

Application::Application(const ApplicationProps& props)
{
    Log::Init();
    LogWidget::Init();
    Time::Init();
    Random::Init();
    //JobSystem::Init();
    AudioSystem::Init();
    ProcessorInfo::Display();
    RCCpp::Init();

    instance = this;
    window = Window::Create({props.name, props.width, props.height});
    window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

    //LuaManager::Init();
    //LuaManager::SetEventCallback(std::bind(&Application::OnLuaEvent, this, std::placeholders::_1));

    //Renderer::Init();
    // FIXME physics manager uses a debug shader so for now it needs to be initialized after renderer
    //PhysicsManager::Init();
    //CommandManager::Init();

    // TODO should be part of the overlay thats handled by each application instead of in the core engine
    // Imgui overlay
    imguiLayer = new VulkanImGuiLayer();
    PushOverlay(imguiLayer);
}

Application::~Application()
{
    // TODO move to shutdown
    PhysicsManager::Shutdown();
    Renderer::Shutdown();
    //LuaManager::Shutdown();
    RCCpp::Shutdown();
    Shutdown();
}

void Application::Run()
{
    OnInit();

    while (isRunning)
    {
        //Timer timer("FrameTime");
        Time::Update();
        RCCpp::Update(Time::deltaTime);

        for (Layer* layer : layerStack)
        {
            layer->OnUpdate(static_cast<float>(Time::deltaTime));
        }

        //if (enableImgui)
        //{
        //    imguiLayer->Begin();
        //    for (Layer* layer : layerStack)
        //    {
        //        layer->OnImGuiRender();
        //    }
        //    imguiLayer->End();
        //}

        window->OnUpdate();
    }
}

void Application::Close()
{
    isRunning = false;
}

void Application::PushLayer(Layer* layer)
{
    layerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* overlay)
{
    layerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));
    dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

    for (auto revIt = layerStack.rbegin(); revIt != layerStack.rend(); ++revIt)
    {
        (*revIt)->OnEvent(e);
        // If event is handled by this layer then don't propogate event down layerstack
        if (e.handled) { break; }
    }
}

//void Application::OnLuaEvent(LuaEvent& e)
//{
//    for (auto revIt = layerStack.rbegin(); revIt != layerStack.rend(); ++revIt)
//    {
//        (*revIt)->OnLuaEvent(e);
//        if (e.handled) { break; }
//    }
//}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
//#ifdef OPENGL
    // For OpenGL's case since glfw handles minimization cases
    //if (e.GetWidth() == 0 || e.GetHeight() == 0)
    //{
    //    // Don't propagate event further
    //    // to avoid resizing framebuffer to 0 dimensions
    //    return true;
    //}
//#endif
    // Check for minimization case
    int width = e.GetWidth(), height = e.GetHeight();
    auto nativeWindow = GetWindow().GetNativeWindow();
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(nativeWindow), &width, &height);
        glfwWaitEvents();
    }

    CORE_INFO("Resize to {0} : {1}", e.GetWidth(), e.GetHeight());
    Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
    return false;
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    isRunning = false;
    return false;
}
