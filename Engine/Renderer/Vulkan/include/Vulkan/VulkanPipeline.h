#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/glm.hpp>
//#include "Vulkan/VulkanInitializers.h"
//#include "Core/Log.h"

struct DescriptorSet
{
    vk::UniqueDescriptorSet descriptorSet;
};

enum class PipelineType
{
    eGraphics = 0,
    eCompute = 1,
    eRaytracing = 2,
    Count
};

namespace cf
{

enum class VertexComponent
{
    Position = 0,
    UV,
    Normal,
    Color,
    Tangent,
};

class Pipeline
{
public:
    Pipeline(
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptorSetLayoutBindings
      , const vk::PipelineShaderStageCreateInfo& shaderStage
    );

    Pipeline(
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> & descriptorSetLayoutBindings
      , const std::vector<vk::PipelineShaderStageCreateInfo> & shaderStages
      , PipelineType type
      , const std::vector<cf::VertexComponent>& components
      , vk::RenderPass renderPass
    );
    std::vector<vk::UniqueDescriptorSet> mDescriptorSets;
    std::vector<vk::UniqueDescriptorSetLayout> mDescriptorSetLayouts;
    vk::UniquePipelineLayout mPipelineLayout;
    vk::UniquePipeline mPipeline;

private:
    vk::UniquePipeline CreateGraphicsPipeline(
        const std::vector<vk::PipelineShaderStageCreateInfo> & shaderStages
      , const std::vector<cf::VertexComponent>& components
      , vk::RenderPass renderPass
    );

    vk::UniquePipeline CreateComputePipeline(
        const std::vector<vk::PipelineShaderStageCreateInfo> & shaderStages
    );

    vk::Device mDevice;
    PipelineType type;
};


//=============================================================================
// Graphics pipeline related
struct TransformPushConstBlock
{
    glm::mat4 model;
};

vk::PipelineVertexInputStateCreateInfo* CreatePipelineVertexInputState(const std::vector<VertexComponent>& components);

} // namespace cf
