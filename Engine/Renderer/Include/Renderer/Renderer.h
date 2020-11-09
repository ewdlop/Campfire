#ifndef RENDERER_H
#define RENDERER_H

#include "Renderer/RenderCommand.h"
#include "Renderer/RendererAPI.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

#include "Scene/Camera.h"

class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera& camera);
    static void EndScene();

    static void OnWindowResize(uint32_t width, uint32_t height);

public:
    // TODO allow material instance to override
    static void SubmitMesh(const SharedPtr<Mesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f), SharedPtr<Material> overrideMaterial = nullptr);
    static void SubmitMesh(const SharedPtr<Mesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f), SharedPtr<TextureCube> texCube = nullptr, SharedPtr<Material> overrideMaterial = nullptr);

public:
    static void Draw(const SharedPtr<Shader>& shader, const SharedPtr<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

    static void DrawLines(const SharedPtr<Shader>& shader, const SharedPtr<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

    static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }


private:
    static glm::mat4 viewProjMatrix;
    static SharedPtr<Shader> shader;
};

#endif // RENDERER_H