#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"

#include <iostream>
#include <array>

#include <glm/gtc/matrix_transform.hpp>

// TODO add tiling factor
glm::mat4 Renderer2D::viewMatrix;
SharedPtr<Shader> Renderer2D::shader;

struct QuadVertexData
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 uvCoords;
    float textureID;
};

struct BatchData
{
    static const uint32_t maxQuads = 10000;
    static const uint32_t maxVertices = maxQuads * 4;
    static const uint32_t maxIndices = maxQuads * 6;
    static const uint32_t maxTextureSlots = 32;

    uint32_t indexCount = 0;
    uint32_t quadCount = 0;

    QuadVertexData* quadVertexBufferBase = nullptr;
    QuadVertexData* quadVertexBufferPtr = nullptr;

    SharedPtr<VertexArray> vertexArray;
    SharedPtr<VertexBuffer> vertexBuffer;
    SharedPtr<Texture2D> whiteTexture;

    uint32_t textureSlotIndex = 1; // reserve 0 for default white texture
    std::array<SharedPtr<Texture2D>, maxTextureSlots> textureSlots;
};

static BatchData batch;

void Renderer2D::Init()
{
    std::vector<uint32_t> indices;
    for (uint32_t count = 0; count < batch.maxQuads; ++count)
    {
        indices.insert(indices.end(),
            {
                0 + (4 * count), 1 + (4 * count), 2 + (4 * count),
                2 + (4 * count), 3 + (4 * count), 0 + (4 * count)
            }
        );
    }

    batch.vertexArray = VertexArray::Create();

    batch.quadVertexBufferBase = new QuadVertexData[batch.maxVertices];
    batch.quadVertexBufferPtr  = batch.quadVertexBufferBase;

    batch.vertexBuffer = VertexBuffer::Create(batch.maxVertices * sizeof(QuadVertexData));

    batch.whiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    batch.whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

    int32_t samplers[batch.maxTextureSlots];
    for (uint32_t i = 0; i < batch.maxTextureSlots; ++i)
    {
        samplers[i] = i;
    }

    shader = ShaderManager::Create("quadDefault", "../Campfire/Shaders/quad.vert", "../Campfire/Shaders/quad.frag");
    shader->Bind();
    shader->SetUniformBlock("Camera", 0);
    shader->SetIntArray("uTextures", samplers, batch.maxTextureSlots);

    // Default-white texture at slot 0
    batch.textureSlots[0] = batch.whiteTexture;

    BufferLayout layout =
    {
        { ShaderDataType::FLOAT3, "aPos" },
        { ShaderDataType::FLOAT4, "aColor" },
        { ShaderDataType::FLOAT2, "aUV" },
        { ShaderDataType::FLOAT , "aTexIndex" }
    };
    batch.vertexBuffer->SetLayout(layout);

    SharedPtr<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
    batch.vertexArray->AddVertexBuffer(batch.vertexBuffer);
    batch.vertexArray->SetIndexBuffer(indexBuffer);

    batch.vertexBuffer->Unbind();
    batch.vertexArray->Unbind();
}

void Renderer2D::Shutdown()
{
}

void Renderer2D::DrawBillboard(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color)
{
    DrawBillboard(position, scale, batch.whiteTexture, color);
}

void Renderer2D::DrawBillboard(const glm::vec3& position, const glm::vec3& scale, const SharedPtr<Texture2D>& texture, const glm::vec4& color)
{
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);

    glm::vec3 cameraRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    glm::vec3 cameraUp    = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

    glm::vec2 uv[4] =
    {
        glm::vec2(0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f)
    };

    // Default quad vertices
    std::array<glm::vec4, 4> pos =
    {
        glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
        glm::vec4(-1.0f,-1.0f, 0.0f, 1.0f),
        glm::vec4( 1.0f,-1.0f, 0.0f, 1.0f),
        glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f),
    };

    for (int i = 0; i < pos.size(); ++i)
    {
        pos[i] = glm::vec4(position + (cameraRight * pos[i].x) + (cameraUp * pos[i].y), 1.0f);
        pos[i] = transform * pos[i];
    }

    // Check if texture is present in list
    uint32_t textureIndex = 0;
    for (uint32_t i = 1; i < batch.maxTextureSlots; ++i)
    {
        if (batch.textureSlots[i].get() == texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // This is a new texture
    if (textureIndex == 0)
    {
        textureIndex = (float)batch.textureSlotIndex;
        batch.textureSlots[batch.textureSlotIndex] = texture;
        batch.textureSlotIndex++;
    }

    // Process new data
    for (int i = 0; i < 4; ++i)
    {
        batch.quadVertexBufferPtr->position = glm::vec3(pos[i].x, pos[i].y, pos[i].z);
        batch.quadVertexBufferPtr->color = color;
        batch.quadVertexBufferPtr->uvCoords = uv[i];
        batch.quadVertexBufferPtr->textureID = textureIndex;
        batch.quadVertexBufferPtr++;
    }

    batch.quadCount++;
    batch.indexCount += 6;
}

void Renderer2D::DrawPostProcessQuad(const SharedPtr<Shader>& shader, uint32_t colorAttachmentID)
{
    // TODO need to draw this last and not part of batch
    // or make sure its the last object within the batch
    shader->Bind();
    glBindTextureUnit(0, colorAttachmentID);

    glDisable(GL_DEPTH_TEST);
    //Renderer::Draw(shader, quadVertexArray, glm::mat4(1.0f));
    glEnable(GL_DEPTH_TEST);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const SharedPtr<Texture2D>& texture, const SharedPtr<Shader>& shader)
{
    shader->Bind();
    texture->Bind();
    //Renderer::Draw(shader, quadVertexArray, transform);
}


void Renderer2D::DrawQuad(const glm::mat4& transform, const SharedPtr<Texture2D>& texture, const glm::vec4& tintColor)
{
    shader->Bind();
    shader->SetFloat4("tintColor", tintColor);

    texture->Bind();

    //Renderer::Draw(shader, quadVertexArray, transform);
}

//=========================================
//--------------Batching-------------------
//=========================================
void Renderer2D::SubmitQuad(const glm::mat4& transform, const glm::vec4& color)
{
    SubmitQuad(transform, batch.whiteTexture, color);
}

void Renderer2D::SubmitQuad(const glm::mat4& transform, const SharedPtr<Texture2D>& texture, const glm::vec4& color)
{
    glm::vec2 uv[4] =
    {
        glm::vec2(0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f)
    };

    // Default quad vertices
    std::array<glm::vec4, 4> pos =
    {
        transform * glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
        transform * glm::vec4(-1.0f,-1.0f, 0.0f, 1.0f),
        transform * glm::vec4( 1.0f,-1.0f, 0.0f, 1.0f),
        transform * glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f),
    };

    // Check if texture is present in list
    uint32_t textureIndex = 0;
    for (uint32_t i = 1; i < batch.maxTextureSlots; ++i)
    {
        if (batch.textureSlots[i].get() == texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // This is a new texture
    if (textureIndex == 0)
    {
        textureIndex = (float)batch.textureSlotIndex;
        batch.textureSlots[batch.textureSlotIndex] = texture;
        batch.textureSlotIndex++;
    }

    // Process new data
    for (int i = 0; i < 4; ++i)
    {
        batch.quadVertexBufferPtr->position = glm::vec3(pos[i].x, pos[i].y, pos[i].z);
        batch.quadVertexBufferPtr->color = color;
        batch.quadVertexBufferPtr->uvCoords = uv[i];
        batch.quadVertexBufferPtr->textureID = textureIndex;
        batch.quadVertexBufferPtr++;
    }

    batch.quadCount++;
    batch.indexCount += 6;
}

void Renderer2D::DrawBatch()
{
    if (batch.indexCount == 0) { return; } // No quads submitted to draw

    // Bind all submitted textures
    for (uint32_t i = 0; i < batch.textureSlotIndex; ++i)
    {
        batch.textureSlots[i]->Bind(i);
    }

    batch.vertexArray->Bind();
    glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, (void*)0);
    batch.vertexArray->Unbind();

}


void Renderer2D::BeginScene(Camera& camera)
{
    // view matrix for orienting billboards
    viewMatrix = camera.GetViewMatrix();
    //int32_t samplers[batch.maxTextureSlots];
    //for (uint32_t i = 0; i < batch.maxTextureSlots; ++i)
    //{
    //    samplers[i] = i;
    //}
    shader->Bind();
    shader->SetMat4("model", glm::mat4(1.0f));
    //shader->SetIntArray("uTextures", samplers, batch.maxTextureSlots);

    batch.indexCount = 0;
    batch.quadCount = 0;
    //batch.textureSlotIndex = 1;

    batch.quadVertexBufferPtr = batch.quadVertexBufferBase; // reset

}

void Renderer2D::EndScene()
{
    // Set new data
    uint32_t dataSize = (uint32_t)((uint8_t*)batch.quadVertexBufferPtr - (uint8_t*)batch.quadVertexBufferBase);
    batch.vertexBuffer->SetData(batch.quadVertexBufferBase, dataSize);

    Renderer2D::DrawBatch();
}

