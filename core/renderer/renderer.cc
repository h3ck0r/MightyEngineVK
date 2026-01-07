#include "renderer.h"

#include <iostream>
#include <memory>

#include "../utils/image.h"
#include "../utils/object_utils.h"
#include "utils/accel.h"
#include "utils/buffer.h"

namespace engine {
Renderer::Renderer() {
    command_buffer_ = std::make_unique<engine_init::CommandBuffer>(context_.device(),
        context_.command_pool_handle(),
        context_.swapchain_handle().swapchain_images().size());

    output_image_ = std::make_unique<engine::Image>(context_,
        vk::Extent2D{WIDTH, HEIGHT},
        vk::Format::eB8G8R8A8Unorm,
        vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eTransferDst);

    // Load mesh
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Face> faces;
    LoadFromFile(vertices, indices, faces);

    vertex_buffer_ = std::make_unique<Buffer>(context_,
        Buffer::Type::AccelInput,
        sizeof(Vertex) * vertices.size(),
        vertices.data());
    index_buffer_ = std::make_unique<Buffer>(context_,
        Buffer::Type::AccelInput,
        sizeof(uint32_t) * indices.size(),
        indices.data());
    face_buffer_ = std::make_unique<Buffer>(context_,
        Buffer::Type::AccelInput,
        sizeof(Face) * faces.size(),
        faces.data());

    // Create bottom level accel struct
    vk::AccelerationStructureGeometryTrianglesDataKHR triangle_data_;
    triangle_data_.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangle_data_.setVertexData(vertex_buffer_->device_address());
    triangle_data_.setVertexStride(sizeof(Vertex));
    triangle_data_.setMaxVertex(static_cast<uint32_t>(vertices.size()));
    triangle_data_.setIndexType(vk::IndexType::eUint32);
    triangle_data_.setIndexData(index_buffer_->device_address());

    vk::AccelerationStructureGeometryKHR triangle_geometry_;
    triangle_geometry_.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    triangle_geometry_.setGeometry({triangle_data_});
    triangle_geometry_.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    uint32_t primitive_count_ = static_cast<uint32_t>(indices.size() / 3);

    bottom_accel_ = std::make_unique<Accel>(context_,
        triangle_geometry_,
        primitive_count_,
        vk::AccelerationStructureTypeKHR::eBottomLevel);

    // Create top level accel struct
    vk::TransformMatrixKHR transform_matrix = std::array{
        std::array{1.0f, 0.0f, 0.0f, 0.0f},
        std::array{0.0f, 1.0f, 0.0f, 0.0f},
        std::array{0.0f, 0.0f, 1.0f, 0.0f},
    };

    vk::AccelerationStructureInstanceKHR accel_instance;
    accel_instance.setTransform(transform_matrix);
    accel_instance.setMask(0xFF);
    accel_instance.setAccelerationStructureReference(
        bottom_accel_->buffer().device_address());
    accel_instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

    Buffer instances_buffer{context_,
        Buffer::Type::AccelInput,
        sizeof(vk::AccelerationStructureInstanceKHR),
        &accel_instance};

    vk::AccelerationStructureGeometryInstancesDataKHR instances_data;
    instances_data.setArrayOfPointers(false);
    instances_data.setData(instances_buffer.device_address());

    vk::AccelerationStructureGeometryKHR instance_geometry;
    instance_geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    instance_geometry.setGeometry({instances_data});
    instance_geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    top_accel_ = std::make_unique<Accel>(context_,
        instance_geometry,
        1,
        vk::AccelerationStructureTypeKHR::eTopLevel);

    // Load shaders
    const std::vector<char> raygen_code = ReadFile("./shaders/raygen.rgen.spv");
    const std::vector<char> miss_code = ReadFile("./shaders/miss.rmiss.spv");
    const std::vector<char> chit_code = ReadFile("./shaders/closesthit.rchit.spv");

    std::vector<vk::UniqueShaderModule> shader_modules(3);
    shader_modules[0] = context_.device().createShaderModuleUnique(
        {{}, raygen_code.size(), reinterpret_cast<const uint32_t*>(raygen_code.data())});
    shader_modules[1] = context_.device().createShaderModuleUnique(
        {{}, miss_code.size(), reinterpret_cast<const uint32_t*>(miss_code.data())});
    shader_modules[2] = context_.device().createShaderModuleUnique(
        {{}, chit_code.size(), reinterpret_cast<const uint32_t*>(chit_code.data())});

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages(3);
    shader_stages[0] = {{},
        vk::ShaderStageFlagBits::eRaygenKHR,
        *shader_modules[0],
        "main"};
    shader_stages[1] = {{},
        vk::ShaderStageFlagBits::eMissKHR,
        *shader_modules[1],
        "main"};
    shader_stages[2] = {{},
        vk::ShaderStageFlagBits::eClosestHitKHR,
        *shader_modules[2],
        "main"};

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups(3);
    shader_groups[0] = {vk::RayTracingShaderGroupTypeKHR::eGeneral,
        0,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR};
    shader_groups[1] = {vk::RayTracingShaderGroupTypeKHR::eGeneral,
        1,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR};
    shader_groups[2] = {vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
        VK_SHADER_UNUSED_KHR,
        2,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR};

    // create ray tracing pipeline
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        {0,
            vk::DescriptorType::eAccelerationStructureKHR,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR},  // Binding = 0 : TLAS
        {1,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR},  // Binding = 1 : Storage
                                                   // image
        {2,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eClosestHitKHR},  // Binding = 2 :
                                                       // Vertices
        {3,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eClosestHitKHR},  // Binding = 3 :
                                                       // Indices
        {4,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eClosestHitKHR},  // Binding = 4 :
                                                       // Faces
    };

    // Create desc set layout
    vk::DescriptorSetLayoutCreateInfo desc_set_layout_info;
    desc_set_layout_info.setBindings(bindings);
    vk::UniqueDescriptorSetLayout desc_set_layout =
        context_.device().createDescriptorSetLayoutUnique(desc_set_layout_info);

    // Create pipeline layout
    vk::PushConstantRange push_range;
    push_range.setOffset(0);
    push_range.setSize(sizeof(int));
    push_range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setSetLayouts(*desc_set_layout);
    pipeline_layout_info.setPushConstantRanges(push_range);
    pipeline_layout_ = context_.device().createPipelineLayoutUnique(pipeline_layout_info);

    // Create pipeline
    vk::RayTracingPipelineCreateInfoKHR rt_pipeline_info;
    rt_pipeline_info.setStages(shader_stages);
    rt_pipeline_info.setGroups(shader_groups);
    rt_pipeline_info.setMaxPipelineRayRecursionDepth(4);
    rt_pipeline_info.setLayout(*pipeline_layout_);

    auto result = context_.device().createRayTracingPipelineKHRUnique(nullptr,
        nullptr,
        rt_pipeline_info);
    std::cout << "Finished initializing ray traycing pipeline.\n";
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create ray tracing pipeline.");
    }

    pipeline_ = std::move(result.value);

    // Get ray tracing properties
    auto properties = context_.physical_device_handle()
                          .physical_device()
                          .getProperties2<vk::PhysicalDeviceProperties2,
                              vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    auto rt_properties =
        properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    // Calculate shader binding table (SBT) size
    uint32_t handle_size = rt_properties.shaderGroupHandleSize;
    uint32_t handle_size_aligned = rt_properties.shaderGroupHandleAlignment;
    uint32_t group_count = static_cast<uint32_t>(shader_groups.size());
    uint32_t sbt_size = group_count * handle_size_aligned;

    // Get shader group handles
    std::vector<uint8_t> handle_storage(sbt_size);
    if (context_.device().getRayTracingShaderGroupHandlesKHR(*pipeline_,
            0,
            group_count,
            sbt_size,
            handle_storage.data())
        != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create SBT
    raygen_SBT_ = std::make_unique<Buffer>(context_,
        Buffer::Type::ShaderBindingTable,
        handle_size,
        handle_storage.data() + 0 * handle_size_aligned);
    miss_SBT_ = std::make_unique<Buffer>(context_,
        Buffer::Type::ShaderBindingTable,
        handle_size,
        handle_storage.data() + 1 * handle_size_aligned);
    hit_SBT_ = std::make_unique<Buffer>(context_,
        Buffer::Type::ShaderBindingTable,
        handle_size,
        handle_storage.data() + 2 * handle_size_aligned);

    uint32_t stride = rt_properties.shaderGroupHandleAlignment;
    uint32_t size = rt_properties.shaderGroupHandleAlignment;

    raygen_region_ =
        vk::StridedDeviceAddressRegionKHR{raygen_SBT_->device_address(), stride, size};
    miss_region_ =
        vk::StridedDeviceAddressRegionKHR{miss_SBT_->device_address(), stride, size};
    hit_region_ =
        vk::StridedDeviceAddressRegionKHR{hit_SBT_->device_address(), stride, size};

    // Create desc set
    desc_set_ = context_.AllocateDescSet(*desc_set_layout,
        context_.descriptor_pool_handle().descriptor_pool(),
        context_.device());
    std::vector<vk::WriteDescriptorSet> writes(bindings.size());
    for (int i = 0; i < bindings.size(); i++) {
        writes[i].setDstSet(*desc_set_);
        writes[i].setDescriptorType(bindings[i].descriptorType);
        writes[i].setDescriptorCount(bindings[i].descriptorCount);
        writes[i].setDstBinding(bindings[i].binding);
    }
    writes[0].setPNext(&top_accel_->desc_accel_info());
    writes[1].setImageInfo(output_image_->desc_image_info());
    writes[2].setBufferInfo(vertex_buffer_->desc_buffer_info());
    writes[3].setBufferInfo(index_buffer_->desc_buffer_info());
    writes[4].setBufferInfo(face_buffer_->desc_buffer_info());
    context_.device().updateDescriptorSets(writes, nullptr);

    std::cout << "Finished initializing renderer.\n";
}

void Renderer::MainLoop() {
    std::cout << "Started engine main loop.\n";
    // Main loop
    uint32_t image_index = 0;
    int frame = 0;
    vk::UniqueSemaphore image_acquired_semaphore =
        context_.device().createSemaphoreUnique(vk::SemaphoreCreateInfo());
    while (!glfwWindowShouldClose(context_.window_handle().window())) {
        glfwPollEvents();

        // Acquire next image
        image_index = context_.device()
                          .acquireNextImageKHR(context_.swapchain_handle().swapchain(),
                              UINT64_MAX,
                              *image_acquired_semaphore)
                          .value;

        // Record commands
        vk::CommandBuffer command_buffer = command_buffer_->get_buffer(image_index);
        command_buffer.begin(vk::CommandBufferBeginInfo());
        command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline_);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
            *pipeline_layout_,
            0,
            *desc_set_,
            nullptr);
        command_buffer.pushConstants(*pipeline_layout_,
            vk::ShaderStageFlagBits::eRaygenKHR,
            0,
            sizeof(int),
            &frame);
        command_buffer.traceRaysKHR(raygen_region_,
            miss_region_,
            hit_region_,
            {},
            WIDTH,
            HEIGHT,
            1);

        vk::Image src_image = output_image_->image();
        vk::Image dst_image =
            context_.swapchain_handle().get_swapchain_image(image_index);
        Image::SetImageLayout(command_buffer,
            src_image,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferSrcOptimal);
        Image::SetImageLayout(command_buffer,
            dst_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal);
        Image::CopyImage(command_buffer, src_image, dst_image);
        Image::SetImageLayout(command_buffer,
            src_image,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eGeneral);
        Image::SetImageLayout(command_buffer,
            dst_image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR);

        command_buffer.end();

        // Submit
        context_.queue().submit(vk::SubmitInfo().setCommandBuffers(command_buffer));

        // Present image
        vk::PresentInfoKHR presentInfo;
        presentInfo.setSwapchains(context_.swapchain_handle().swapchain());
        presentInfo.setImageIndices(image_index);
        presentInfo.setWaitSemaphores(*image_acquired_semaphore);
        auto result = context_.queue().presentKHR(presentInfo);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present.");
        }
        context_.queue().waitIdle();
        frame++;
    }

    context_.device().waitIdle();
}
}  // namespace engine