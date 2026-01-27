//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "brx_anari_pal_device.h"
#if defined(__GNUC__)
// GCC or CLANG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <DirectXPackedVector.h>
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
// MSVC or CLANG-CL
#include <DirectXPackedVector.h>
#else
#error Unknown Compiler
#endif
#include "../shaders/surface.bsli"
#include "../shaders/deforming_surface_resource_binding.bsli"
#include "../shaders/surface_resource_binding.bsli"
#include "../../Brioche-Shader-Language/include/brx_packed_vector.h"
#include "../../Brioche-Shader-Language/include/brx_octahedral_mapping.h"
#include <cstring>

brx_pal_storage_asset_buffer *brx_anari_pal_device::internal_create_asset_buffer(void const *const data_base, uint32_t const data_size)
{
    // TODO: merge submit and barrier

    brx_pal_staging_upload_buffer *buffer_staging_upload_buffer = NULL;
    {
        assert(NULL == buffer_staging_upload_buffer);
        buffer_staging_upload_buffer = this->m_device->create_staging_upload_buffer(data_size);

        std::memcpy(buffer_staging_upload_buffer->get_host_memory_range_base(), data_base, data_size);
    }

    brx_pal_storage_asset_buffer *const destination_asset_buffer = this->m_device->create_storage_asset_buffer(data_size);
    {
        brx_pal_upload_command_buffer *const upload_command_buffer = this->m_device->create_upload_command_buffer();

        brx_pal_graphics_command_buffer *const graphics_command_buffer = this->m_device->create_graphics_command_buffer();

        brx_pal_upload_queue *const upload_queue = this->m_device->create_upload_queue();

        brx_pal_graphics_queue *const graphics_queue = this->m_device->create_graphics_queue();

        brx_pal_fence *const fence = this->m_device->create_fence(true);

        this->m_device->reset_upload_command_buffer(upload_command_buffer);

        this->m_device->reset_graphics_command_buffer(graphics_command_buffer);

        upload_command_buffer->begin();

        graphics_command_buffer->begin();

        upload_command_buffer->asset_resource_load_dont_care(1U, &destination_asset_buffer, 0U, NULL);

        upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(destination_asset_buffer, 0U, buffer_staging_upload_buffer, 0U, data_size);

        upload_command_buffer->asset_resource_store(1U, &destination_asset_buffer, 0U, NULL);

        upload_command_buffer->release(1U, &destination_asset_buffer, 0U, NULL, 0U, NULL);

        graphics_command_buffer->acquire(1U, &destination_asset_buffer, 0U, NULL, 0U, NULL);

        upload_command_buffer->end();

        graphics_command_buffer->end();

        upload_queue->submit_and_signal(upload_command_buffer);

        this->m_device->reset_fence(fence);

        graphics_queue->wait_and_submit(upload_command_buffer, graphics_command_buffer, fence);

        // TODO: async load
        this->m_device->wait_for_fence(fence);

        this->m_device->destroy_fence(fence);

        this->m_device->destroy_upload_command_buffer(upload_command_buffer);

        this->m_device->destroy_graphics_command_buffer(graphics_command_buffer);

        this->m_device->destroy_upload_queue(upload_queue);

        this->m_device->destroy_graphics_queue(graphics_queue);
    }

    this->m_device->destroy_staging_upload_buffer(buffer_staging_upload_buffer);
    buffer_staging_upload_buffer = NULL;

    return destination_asset_buffer;
}

inline brx_pal_storage_intermediate_buffer *brx_anari_pal_device::create_deforming_surface_intermediate_vertex_position_buffer(uint32_t vertex_count)
{
    return this->m_device->create_storage_intermediate_buffer(sizeof(surface_vertex_position_buffer_element) * vertex_count);
}

inline brx_pal_storage_intermediate_buffer *brx_anari_pal_device::create_deforming_surface_intermediate_vertex_varying_buffer(uint32_t vertex_count)
{
    return this->m_device->create_storage_intermediate_buffer(sizeof(surface_vertex_varying_buffer_element) * vertex_count);
}

inline brx_pal_uniform_upload_buffer *brx_anari_pal_device::create_deforming_surface_group_update_set_uniform_buffer()
{
    return this->m_device->create_uniform_upload_buffer(this->helper_compute_uniform_buffer_size<deforming_surface_group_update_set_uniform_buffer_binding>());
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_deforming_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_deforming_surface_group_update_descriptor_set_layout, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(deforming_surface_group_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_deforming_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const vertex_blending_buffer, brx_pal_read_only_storage_buffer const *const *const morph_targets_vertex_position_buffers, brx_pal_read_only_storage_buffer const *const *const morph_targets_vertex_varying_buffers, brx_pal_storage_buffer const *const vertex_position_buffer_instance, brx_pal_storage_buffer const *const vertex_varying_buffer_instance)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_deforming_surface_update_descriptor_set_layout, 0U);

    assert(NULL != vertex_position_buffer);
    assert(NULL != vertex_varying_buffer);
    assert(NULL != vertex_blending_buffer);
    assert(NULL != vertex_position_buffer_instance);
    assert(NULL != vertex_varying_buffer_instance);

    brx_pal_read_only_storage_buffer const *surface_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
    surface_buffers[DEFORMING_SURFACE_INPUT_VERTEX_POSITION_BUFFER_INDEX] = vertex_position_buffer;
    surface_buffers[DEFORMING_SURFACE_INPUT_VERTEX_VARYING_BUFFER_INDEX] = vertex_varying_buffer;
    surface_buffers[DEFORMING_SURFACE_INPUT_VERTEX_BLENDING_BUFFER_INDEX] = vertex_blending_buffer;
    for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
    {
        surface_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_name_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_POSITION_BUFFER_INDEX] = (NULL != morph_targets_vertex_position_buffers[morph_target_name_index]) ? morph_targets_vertex_position_buffers[morph_target_name_index] : this->m_place_holder_asset_buffer->get_read_only_storage_buffer();
        surface_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_name_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_VARYING_BUFFER_INDEX] = (NULL != morph_targets_vertex_varying_buffers[morph_target_name_index]) ? morph_targets_vertex_varying_buffers[morph_target_name_index] : this->m_place_holder_asset_buffer->get_read_only_storage_buffer();
    }
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(surface_buffers) / sizeof(surface_buffers[0]), NULL, NULL, surface_buffers, NULL, NULL, NULL, NULL, NULL);

    brx_pal_storage_buffer const *surface_instance_buffers[DEFORMING_SURFACE_OUTPUT_BUFFER_COUNT];
    surface_instance_buffers[DEFORMING_SURFACE_OUTPUT_VERTEX_POSITION_BUFFER_INDEX] = vertex_position_buffer_instance;
    surface_instance_buffers[DEFORMING_SURFACE_OUTPUT_VERTEX_VARYING_BUFFER_INDEX] = vertex_varying_buffer_instance;
    this->m_device->write_descriptor_set(descriptor_set, 1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, sizeof(surface_instance_buffers) / sizeof(surface_instance_buffers[0]), NULL, NULL, NULL, surface_instance_buffers, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_uniform_upload_buffer *brx_anari_pal_device::create_surface_group_update_set_uniform_buffer()
{
    return this->m_device->create_uniform_upload_buffer(this->helper_compute_uniform_buffer_size<surface_group_update_set_uniform_buffer_binding>());
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_surface_group_update_descriptor_set_layout, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(surface_group_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const index_buffer, brx_pal_read_only_storage_buffer const *const auxiliary_buffer, brx_pal_sampled_image const *const emissive_image, brx_pal_sampled_image const *const normal_image, brx_pal_sampled_image const *const base_color_image, brx_pal_sampled_image const *const metallic_roughness_image)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_surface_update_descriptor_set_layout, 0U);

    assert(NULL != vertex_position_buffer);
    assert(NULL != vertex_varying_buffer);
    assert(NULL != index_buffer);
    assert(NULL != auxiliary_buffer);

    brx_pal_read_only_storage_buffer const *surface_buffers[FORWARD_SHADING_SURFACE_BUFFER_COUNT];
    surface_buffers[FORWARD_SHADING_SURFACE_VERTEX_POSITION_BUFFER_INDEX] = vertex_position_buffer;
    surface_buffers[FORWARD_SHADING_SURFACE_VERTEX_VARYING_BUFFER_INDEX] = vertex_varying_buffer;
    surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX] = index_buffer;
    surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX] = auxiliary_buffer;
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, FORWARD_SHADING_SURFACE_BUFFER_COUNT, NULL, NULL, surface_buffers, NULL, NULL, NULL, NULL, NULL);

    brx_pal_sampled_image const *surface_images[FORWARD_SHADING_SURFACE_TEXTURE_COUNT];
    surface_images[FORWARD_SHADING_SURFACE_EMISSIVE_TEXTURE_INDEX] = (NULL != emissive_image) ? emissive_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[FORWARD_SHADING_SURFACE_NORMAL_TEXTURE_INDEX] = (NULL != normal_image) ? normal_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[FORWARD_SHADING_SURFACE_BASE_COLOR_TEXTURE_INDEX] = (NULL != base_color_image) ? base_color_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[FORWARD_SHADING_SURFACE_METALLIC_ROUGHNESS_TEXTURE_INDEX] = (NULL != metallic_roughness_image) ? metallic_roughness_image : this->m_place_holder_asset_image->get_sampled_image();
    this->m_device->write_descriptor_set(descriptor_set, FORWARD_SHADING_SURFACE_BUFFER_COUNT, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, FORWARD_SHADING_SURFACE_TEXTURE_COUNT, NULL, NULL, NULL, NULL, surface_images, NULL, NULL, NULL);

    return descriptor_set;
}

brx_anari_surface_group *brx_anari_pal_device::new_surface_group(uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces)
{
    void *new_unwrapped_surface_group_base = mcrt_malloc(sizeof(brx_anari_pal_surface_group), alignof(brx_anari_pal_surface_group));
    assert(NULL != new_unwrapped_surface_group_base);

    brx_anari_pal_surface_group *new_unwrapped_surface_group = new (new_unwrapped_surface_group_base) brx_anari_pal_surface_group{};
    new_unwrapped_surface_group->init(this, surface_count, surfaces);
    return new_unwrapped_surface_group;
}

void brx_anari_pal_device::release_surface_group(brx_anari_pal_surface_group *const release_unwrapped_surface_group)
{
    if (0U == release_unwrapped_surface_group->internal_release())
    {
        brx_anari_pal_surface_group *const delete_unwrapped_surface_group = release_unwrapped_surface_group;

        auto const found_surface_group = this->m_world_surface_group_instances.find(static_cast<brx_anari_pal_surface_group const *>(delete_unwrapped_surface_group));
        if (this->m_world_surface_group_instances.end() != found_surface_group)
        {
            assert(found_surface_group->second.empty());
            this->m_world_surface_group_instances.erase(found_surface_group);
        }

        delete_unwrapped_surface_group->uninit(this);

        delete_unwrapped_surface_group->~brx_anari_pal_surface_group();
        mcrt_free(delete_unwrapped_surface_group);
    }
}

void brx_anari_pal_device::release_surface_group(brx_anari_surface_group *wrapped_surface_group)
{
    assert(NULL != wrapped_surface_group);
    brx_anari_pal_surface_group *const release_unwrapped_surface_group = static_cast<brx_anari_pal_surface_group *>(wrapped_surface_group);

    this->release_surface_group(release_unwrapped_surface_group);
}

brx_anari_surface_group_instance *brx_anari_pal_device::world_new_surface_group_instance(brx_anari_surface_group *surface_group)
{
    void *new_unwrapped_surface_group_instance_base = mcrt_malloc(sizeof(brx_anari_pal_surface_group_instance), alignof(brx_anari_pal_surface_group_instance));
    assert(NULL != new_unwrapped_surface_group_instance_base);

    brx_anari_pal_surface_group_instance *new_unwrapped_surface_group_instance = new (new_unwrapped_surface_group_instance_base) brx_anari_pal_surface_group_instance{};
    new_unwrapped_surface_group_instance->init(this, static_cast<brx_anari_pal_surface_group *>(surface_group));

    auto &surface_group_instances = this->m_world_surface_group_instances[surface_group];
    auto const found_surface_group_instance = surface_group_instances.find(new_unwrapped_surface_group_instance);
    assert(surface_group_instances.end() == found_surface_group_instance);
    surface_group_instances.emplace_hint(found_surface_group_instance, new_unwrapped_surface_group_instance);

    return new_unwrapped_surface_group_instance;
}

void brx_anari_pal_device::world_release_surface_group_instance(brx_anari_surface_group_instance *wrapped_surface_group_instance)
{
    brx_anari_pal_surface_group_instance *const delete_unwrapped_surface_group_instance = static_cast<brx_anari_pal_surface_group_instance *>(wrapped_surface_group_instance);

    auto &surface_group_instances = this->m_world_surface_group_instances[static_cast<brx_anari_surface_group const *>(delete_unwrapped_surface_group_instance->get_surface_group())];
    auto const found_surface_group_instance = surface_group_instances.find(delete_unwrapped_surface_group_instance);
    assert(surface_group_instances.end() != found_surface_group_instance);
    surface_group_instances.erase(found_surface_group_instance);

    delete_unwrapped_surface_group_instance->uninit(this);

    delete_unwrapped_surface_group_instance->~brx_anari_pal_surface_group_instance();
    mcrt_free(delete_unwrapped_surface_group_instance);
}

inline brx_anari_pal_surface::brx_anari_pal_surface() : m_vertex_count(0U), m_vertex_position_buffer(NULL), m_vertex_varying_buffer(NULL), m_vertex_blending_buffer(NULL), m_morph_targets_vertex_position_buffers{}, m_morph_targets_vertex_varying_buffers{}, m_index_count(0U), m_index_buffer(NULL), m_emissive_image(NULL), m_normal_image(NULL), m_base_color_image(NULL), m_metallic_roughness_image(NULL), m_auxiliary_buffer(NULL), m_surface_update_descriptor_set(NULL)
{
}

inline brx_anari_pal_surface::~brx_anari_pal_surface()
{
    assert(0U == this->m_vertex_count);
    assert(NULL == this->m_vertex_position_buffer);
    assert(NULL == this->m_vertex_varying_buffer);
    assert(NULL == this->m_vertex_blending_buffer);
    assert(0U == this->m_index_count);
    assert(NULL == this->m_index_buffer);
    assert(NULL == this->m_emissive_image);
    assert(NULL == this->m_normal_image);
    assert(NULL == this->m_base_color_image);
    assert(NULL == this->m_metallic_roughness_image);
    assert(NULL == this->m_auxiliary_buffer);
    assert(NULL == this->m_surface_update_descriptor_set);
}

inline void brx_anari_pal_surface::init(brx_anari_pal_device *device, BRX_ANARI_SURFACE const *surface)
{
    assert(0U == this->m_vertex_count);
    this->m_vertex_count = surface->m_vertex_count;

    // Vertex Position Buffer
    {
        mcrt_vector<surface_vertex_position_buffer_element> vertex_position_buffer_data(static_cast<size_t>(surface->m_vertex_count));
        for (uint32_t vertex_index = 0; vertex_index < surface->m_vertex_count; ++vertex_index)
        {
            vertex_position_buffer_data[vertex_index].m_position[0] = surface->m_vertex_positions[vertex_index].m_position[0];
            vertex_position_buffer_data[vertex_index].m_position[1] = surface->m_vertex_positions[vertex_index].m_position[1];
            vertex_position_buffer_data[vertex_index].m_position[2] = surface->m_vertex_positions[vertex_index].m_position[2];
        }

        assert(NULL == this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = device->internal_create_asset_buffer(vertex_position_buffer_data.data(), sizeof(surface_vertex_position_buffer_element) * surface->m_vertex_count);
    }

    // Vertex Varying Buffer
    {
        mcrt_vector<surface_vertex_varying_buffer_element> vertex_varying_buffer_data(static_cast<size_t>(surface->m_vertex_count));
        for (uint32_t vertex_index = 0; vertex_index < surface->m_vertex_count; ++vertex_index)
        {
            DirectX::XMFLOAT3 const raw_normal(surface->m_vertex_varyings[vertex_index].m_normal[0], surface->m_vertex_varyings[vertex_index].m_normal[1], surface->m_vertex_varyings[vertex_index].m_normal[2]);

            DirectX::XMFLOAT3 normalized_raw_normal;
            DirectX::XMStoreFloat3(&normalized_raw_normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_normal)));

            DirectX::XMFLOAT2 const mapped_normal = brx_octahedral_map(normalized_raw_normal);

            DirectX::PackedVector::XMSHORTN2 packed_normal;
            DirectX::PackedVector::XMStoreShortN2(&packed_normal, DirectX::XMLoadFloat2(&mapped_normal));

            vertex_varying_buffer_data[vertex_index].m_normal = packed_normal.v;

            DirectX::XMFLOAT3 const raw_tangent_xyz(surface->m_vertex_varyings[vertex_index].m_tangent[0], surface->m_vertex_varyings[vertex_index].m_tangent[1], surface->m_vertex_varyings[vertex_index].m_tangent[2]);
            float const raw_tangent_w = surface->m_vertex_varyings[vertex_index].m_tangent[3];

            DirectX::XMFLOAT3 normalized_raw_tangent_xyz;
            DirectX::XMStoreFloat3(&normalized_raw_tangent_xyz, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_tangent_xyz)));

            vertex_varying_buffer_data[vertex_index].m_tangent = brx_FLOAT3_to_R15G15B2_SNORM(brx_octahedral_map(normalized_raw_tangent_xyz), raw_tangent_w);

            DirectX::XMFLOAT2 const raw_texcoord(surface->m_vertex_varyings[vertex_index].m_texcoord[0], surface->m_vertex_varyings[vertex_index].m_texcoord[1]);

            DirectX::PackedVector::XMHALF2 packed_texcoord;
            DirectX::PackedVector::XMStoreHalf2(&packed_texcoord, DirectX::XMLoadFloat2(&raw_texcoord));

            vertex_varying_buffer_data[vertex_index].m_texcoord = packed_texcoord.v;
        }

        assert(NULL == this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = device->internal_create_asset_buffer(vertex_varying_buffer_data.data(), sizeof(surface_vertex_varying_buffer_element) * surface->m_vertex_count);
    }

    // Vertex Blending Buffer
    if (NULL != surface->m_vertex_blendings)
    {
        mcrt_vector<surface_vertex_blending_buffer_element> vertex_blending_buffer_data(static_cast<size_t>(surface->m_vertex_count));
        for (uint32_t vertex_index = 0; vertex_index < surface->m_vertex_count; ++vertex_index)
        {
            DirectX::PackedVector::XMUSHORT4 const packed_indices(
                static_cast<uint16_t>(surface->m_vertex_blendings[vertex_index].m_indices[0]),
                static_cast<uint16_t>(surface->m_vertex_blendings[vertex_index].m_indices[1]),
                static_cast<uint16_t>(surface->m_vertex_blendings[vertex_index].m_indices[2]),
                static_cast<uint16_t>(surface->m_vertex_blendings[vertex_index].m_indices[3]));

            (*reinterpret_cast<uint64_t *>(&vertex_blending_buffer_data[vertex_index].m_indices_xy)) = packed_indices.v;

            DirectX::XMFLOAT4 const raw_weights(
                surface->m_vertex_blendings[vertex_index].m_weights[0],
                surface->m_vertex_blendings[vertex_index].m_weights[1],
                surface->m_vertex_blendings[vertex_index].m_weights[2],
                surface->m_vertex_blendings[vertex_index].m_weights[3]);

            DirectX::PackedVector::XMUBYTEN4 packed_weights;
            DirectX::PackedVector::XMStoreUByteN4(&packed_weights, DirectX::XMLoadFloat4(&raw_weights));

            vertex_blending_buffer_data[vertex_index].m_weights = packed_weights.v;
        }

        assert(NULL == this->m_vertex_blending_buffer);
        this->m_vertex_blending_buffer = device->internal_create_asset_buffer(vertex_blending_buffer_data.data(), sizeof(surface_vertex_blending_buffer_element) * surface->m_vertex_count);
    }
    else
    {
        assert(NULL == this->m_vertex_blending_buffer);
    }

    // Morph Target
    for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
    {
        brx_anari_surface_vertex_position const *const morph_target_vertex_positions = surface->m_morph_targets_vertex_positions[morph_target_name_index];
        brx_anari_surface_vertex_varying const *const morph_target_vertex_varyings = surface->m_morph_targets_vertex_varyings[morph_target_name_index];
        assert((NULL != morph_target_vertex_positions) == (NULL != morph_target_vertex_varyings));

        // Vertex Position Buffer
        if (NULL != morph_target_vertex_positions)
        {
            mcrt_vector<surface_vertex_position_buffer_element> vertex_position_buffer_data(static_cast<size_t>(surface->m_vertex_count));
            for (uint32_t vertex_index = 0; vertex_index < surface->m_vertex_count; ++vertex_index)
            {
                vertex_position_buffer_data[vertex_index].m_position[0] = morph_target_vertex_positions[vertex_index].m_position[0];
                vertex_position_buffer_data[vertex_index].m_position[1] = morph_target_vertex_positions[vertex_index].m_position[1];
                vertex_position_buffer_data[vertex_index].m_position[2] = morph_target_vertex_positions[vertex_index].m_position[2];
            }

            assert(NULL == this->m_morph_targets_vertex_position_buffers[morph_target_name_index]);
            this->m_morph_targets_vertex_position_buffers[morph_target_name_index] = device->internal_create_asset_buffer(vertex_position_buffer_data.data(), sizeof(surface_vertex_position_buffer_element) * surface->m_vertex_count);
        }
        else
        {
            assert(NULL == this->m_morph_targets_vertex_position_buffers[morph_target_name_index]);
        }

        // Vertex Varying Buffer
        if (NULL != morph_target_vertex_varyings)
        {
            mcrt_vector<surface_vertex_varying_buffer_element> vertex_varying_buffer_data(static_cast<size_t>(surface->m_vertex_count));
            for (uint32_t vertex_index = 0; vertex_index < surface->m_vertex_count; ++vertex_index)
            {
                // We should map the absolute value instead of the offset
                DirectX::XMFLOAT3 const raw_normal(surface->m_vertex_varyings[vertex_index].m_normal[0] + morph_target_vertex_varyings[vertex_index].m_normal[0], surface->m_vertex_varyings[vertex_index].m_normal[1] + morph_target_vertex_varyings[vertex_index].m_normal[1], surface->m_vertex_varyings[vertex_index].m_normal[2] + morph_target_vertex_varyings[vertex_index].m_normal[2]);

                DirectX::XMFLOAT3 normalized_raw_normal;
                DirectX::XMStoreFloat3(&normalized_raw_normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_normal)));

                DirectX::XMFLOAT2 const mapped_normal = brx_octahedral_map(normalized_raw_normal);

                DirectX::PackedVector::XMSHORTN2 packed_normal;
                DirectX::PackedVector::XMStoreShortN2(&packed_normal, DirectX::XMLoadFloat2(&mapped_normal));

                vertex_varying_buffer_data[vertex_index].m_normal = packed_normal.v;

                // We should map the absolute value instead of the offset
                DirectX::XMFLOAT3 const raw_tangent_xyz(surface->m_vertex_varyings[vertex_index].m_tangent[0] + morph_target_vertex_varyings[vertex_index].m_tangent[0], surface->m_vertex_varyings[vertex_index].m_tangent[1] + morph_target_vertex_varyings[vertex_index].m_tangent[1], surface->m_vertex_varyings[vertex_index].m_tangent[2] + morph_target_vertex_varyings[vertex_index].m_tangent[2]);
                float const raw_tangent_w = surface->m_vertex_varyings[vertex_index].m_tangent[3] + morph_target_vertex_varyings[vertex_index].m_tangent[3];

                DirectX::XMFLOAT3 normalized_raw_tangent_xyz;
                DirectX::XMStoreFloat3(&normalized_raw_tangent_xyz, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_tangent_xyz)));

                vertex_varying_buffer_data[vertex_index].m_tangent = brx_FLOAT3_to_R15G15B2_SNORM(brx_octahedral_map(normalized_raw_tangent_xyz), raw_tangent_w);

                DirectX::XMFLOAT2 const raw_texcoord(morph_target_vertex_varyings[vertex_index].m_texcoord[0], morph_target_vertex_varyings[vertex_index].m_texcoord[1]);

                DirectX::PackedVector::XMUSHORTN2 packed_texcoord;
                DirectX::PackedVector::XMStoreUShortN2(&packed_texcoord, DirectX::XMLoadFloat2(&raw_texcoord));

                vertex_varying_buffer_data[vertex_index].m_texcoord = packed_texcoord.v;
            }

            assert(NULL == this->m_morph_targets_vertex_varying_buffers[morph_target_name_index]);
            this->m_morph_targets_vertex_varying_buffers[morph_target_name_index] = device->internal_create_asset_buffer(vertex_varying_buffer_data.data(), sizeof(surface_vertex_varying_buffer_element) * surface->m_vertex_count);
        }
        else
        {
            assert(NULL == this->m_morph_targets_vertex_varying_buffers[morph_target_name_index]);
        }
    }

    uint32_t surface_auxiliary_buffer_texture_flags = 0U;

    // TODO: we may merge the mesh sections if the only difference of the materials is whether "double sided" is enabled

    // Index Buffer
    {
        uint32_t raw_max_index = 0U;
        for (size_t index_index = 0; index_index < surface->m_index_count; ++index_index)
        {
            uint32_t const raw_index = surface->m_indices[index_index];

            raw_max_index = std::max(raw_max_index, raw_index);
        }

        if (!surface->m_is_double_sided)
        {
            assert(0U == this->m_index_count);
            this->m_index_count = surface->m_index_count;

            if (raw_max_index <= static_cast<uint32_t>(UINT16_MAX))
            {
                surface_auxiliary_buffer_texture_flags |= SURFACE_BUFFER_FLAG_UINT16_INDEX;

                mcrt_vector<uint16_t> uint16_indices(static_cast<size_t>(surface->m_index_count));
                for (uint32_t index_index = 0U; index_index < surface->m_index_count; ++index_index)
                {
                    uint16_indices[index_index] = static_cast<uint16_t>(surface->m_indices[index_index]);
                }

                assert(NULL == this->m_index_buffer);
                this->m_index_buffer = device->internal_create_asset_buffer(uint16_indices.data(), sizeof(uint16_t) * surface->m_index_count);
            }
            else
            {
                assert(NULL == this->m_index_buffer);
                this->m_index_buffer = device->internal_create_asset_buffer(surface->m_indices, sizeof(uint32_t) * surface->m_index_count);
            }
        }
        else
        {
            assert(0U == (surface->m_index_count % 3U));
            uint32_t const raw_face_count = surface->m_index_count / 3U;

            uint32_t const new_index_count = 2U * 3U * raw_face_count;
            assert((surface->m_index_count * 2U) == new_index_count);

            assert(0U == this->m_index_count);
            this->m_index_count = new_index_count;

            if (raw_max_index <= static_cast<uint32_t>(UINT16_MAX))
            {
                surface_auxiliary_buffer_texture_flags |= SURFACE_BUFFER_FLAG_UINT16_INDEX;

                mcrt_vector<uint16_t> new_uint16_indices(static_cast<size_t>(new_index_count));

                for (uint32_t raw_face_index = 0U; raw_face_index < raw_face_count; ++raw_face_index)
                {
                    uint32_t const index_index_front_v0 = 6U * raw_face_index;
                    uint32_t const index_index_front_v1 = 6U * raw_face_index + 1U;
                    uint32_t const index_index_front_v2 = 6U * raw_face_index + 2U;

                    uint32_t const index_index_back_v0 = 6U * raw_face_index + 3U;
                    uint32_t const index_index_back_v1 = 6U * raw_face_index + 4U;
                    uint32_t const index_index_back_v2 = 6U * raw_face_index + 5U;

                    uint32_t const raw_index_index_v0 = 3U * raw_face_index;
                    uint32_t const raw_index_index_v1 = 3U * raw_face_index + 1U;
                    uint32_t const raw_index_index_v2 = 3U * raw_face_index + 2U;

                    new_uint16_indices[index_index_front_v0] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v0]);
                    new_uint16_indices[index_index_front_v1] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v1]);
                    new_uint16_indices[index_index_front_v2] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v2]);

                    new_uint16_indices[index_index_back_v0] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v2]);
                    new_uint16_indices[index_index_back_v1] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v1]);
                    new_uint16_indices[index_index_back_v2] = static_cast<uint16_t>(surface->m_indices[raw_index_index_v0]);
                }

                assert(NULL == this->m_index_buffer);
                this->m_index_buffer = device->internal_create_asset_buffer(new_uint16_indices.data(), sizeof(uint16_t) * new_index_count);
            }
            else
            {
                mcrt_vector<uint16_t> new_indices(static_cast<size_t>(new_index_count));

                for (uint32_t raw_face_index = 0U; raw_face_index < raw_face_count; ++raw_face_index)
                {
                    uint32_t const index_index_front_v0 = 6U * raw_face_index;
                    uint32_t const index_index_front_v1 = 6U * raw_face_index + 1U;
                    uint32_t const index_index_front_v2 = 6U * raw_face_index + 2U;

                    uint32_t const index_index_back_v0 = 6U * raw_face_index + 3U;
                    uint32_t const index_index_back_v1 = 6U * raw_face_index + 4U;
                    uint32_t const index_index_back_v2 = 6U * raw_face_index + 5U;

                    uint32_t const raw_index_index_v0 = 3U * raw_face_index;
                    uint32_t const raw_index_index_v1 = 3U * raw_face_index + 1U;
                    uint32_t const raw_index_index_v2 = 3U * raw_face_index + 2U;

                    new_indices[index_index_front_v0] = surface->m_indices[raw_index_index_v0];
                    new_indices[index_index_front_v1] = surface->m_indices[raw_index_index_v1];
                    new_indices[index_index_front_v2] = surface->m_indices[raw_index_index_v2];

                    new_indices[index_index_back_v0] = surface->m_indices[raw_index_index_v2];
                    new_indices[index_index_back_v1] = surface->m_indices[raw_index_index_v1];
                    new_indices[index_index_back_v2] = surface->m_indices[raw_index_index_v0];
                }

                assert(NULL == this->m_index_buffer);
                this->m_index_buffer = device->internal_create_asset_buffer(new_indices.data(), sizeof(uint32_t) * new_index_count);
            }
        }
    }

    // Emissive Image
    {
        if (NULL != surface->m_emissive_image)
        {
            surface_auxiliary_buffer_texture_flags |= SURFACE_TEXTURE_FLAG_EMISSIVE;

            assert(NULL == this->m_emissive_image);
            static_cast<brx_anari_pal_image *>(surface->m_emissive_image)->retain();
            this->m_emissive_image = static_cast<brx_anari_pal_image *>(surface->m_emissive_image);
        }
        else
        {
            assert(NULL == this->m_emissive_image);
        }
    }

    // Normal Image
    {
        if (NULL != surface->m_normal_image)
        {
            surface_auxiliary_buffer_texture_flags |= SURFACE_TEXTURE_FLAG_NORMAL;

            assert(NULL == this->m_normal_image);
            static_cast<brx_anari_pal_image *>(surface->m_normal_image)->retain();
            this->m_normal_image = static_cast<brx_anari_pal_image *>(surface->m_normal_image);
        }
        else
        {
            assert(NULL == this->m_normal_image);
        }
    }

    // Base Color Image
    {
        if (NULL != surface->m_base_color_image)
        {
            surface_auxiliary_buffer_texture_flags |= SURFACE_TEXTURE_FLAG_BASE_COLOR;

            assert(NULL == this->m_base_color_image);
            static_cast<brx_anari_pal_image *>(surface->m_base_color_image)->retain();
            this->m_base_color_image = static_cast<brx_anari_pal_image *>(surface->m_base_color_image);
        }
        else
        {
            assert(NULL == this->m_base_color_image);
        }
    }

    // Metallic Roughness Image
    {
        if (NULL != surface->m_metallic_roughness_image)
        {
            surface_auxiliary_buffer_texture_flags |= SURFACE_TEXTURE_FLAG_METALLIC_ROUGHNESS;

            assert(NULL == this->m_metallic_roughness_image);
            static_cast<brx_anari_pal_image *>(surface->m_metallic_roughness_image)->retain();
            this->m_metallic_roughness_image = static_cast<brx_anari_pal_image *>(surface->m_metallic_roughness_image);
        }
        else
        {
            assert(NULL == this->m_metallic_roughness_image);
        }
    }

    // Auxiliary Buffer
    {
        surface_auxiliary_buffer surface_auxiliary_buffer_data;

        surface_auxiliary_buffer_data.m_buffer_texture_flags = surface_auxiliary_buffer_texture_flags;

        surface_auxiliary_buffer_data.m_emissive_factor_x = surface->m_emissive_factor.m_x;
        surface_auxiliary_buffer_data.m_emissive_factor_y = surface->m_emissive_factor.m_y;
        surface_auxiliary_buffer_data.m_emissive_factor_z = surface->m_emissive_factor.m_z;

        surface_auxiliary_buffer_data.m_base_color_factor_x = surface->m_base_color_factor.m_x;
        surface_auxiliary_buffer_data.m_base_color_factor_y = surface->m_base_color_factor.m_y;
        surface_auxiliary_buffer_data.m_base_color_factor_z = surface->m_base_color_factor.m_z;
        surface_auxiliary_buffer_data.m_base_color_factor_w = surface->m_base_color_factor.m_w;

        surface_auxiliary_buffer_data.m_normal_scale = surface->m_normal_scale;
        surface_auxiliary_buffer_data.m_metallic_factor = surface->m_metallic_factor;
        surface_auxiliary_buffer_data.m_roughness_factor = surface->m_roughness_factor;

        assert(NULL == this->m_auxiliary_buffer);
        this->m_auxiliary_buffer = device->internal_create_asset_buffer(&surface_auxiliary_buffer_data, sizeof(surface_auxiliary_buffer));
    }

    // Descriptor
    {
        bool skin = (NULL != surface->m_vertex_blendings);

        bool morph_target = false;
        for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
        {
            brx_anari_surface_vertex_position const *const morph_target_vertex_positions = surface->m_morph_targets_vertex_positions[morph_target_name_index];
            brx_anari_surface_vertex_varying const *const morph_target_vertex_varyings = surface->m_morph_targets_vertex_varyings[morph_target_name_index];
            if ((NULL != morph_target_vertex_positions) && (NULL != morph_target_vertex_varyings))
            {
                morph_target = true;
                break;
            }
            else
            {
                assert(NULL == morph_target_vertex_positions);
                assert(NULL == morph_target_vertex_varyings);
            }
        }

        if (skin || morph_target)
        {
            assert(NULL == this->m_surface_update_descriptor_set);
        }
        else
        {
            assert(NULL == this->m_surface_update_descriptor_set);
            assert(NULL != this->m_vertex_position_buffer);
            assert(NULL != this->m_vertex_varying_buffer);
            assert(NULL != this->m_index_buffer);
            assert(NULL != this->m_auxiliary_buffer);
            this->m_surface_update_descriptor_set = device->create_surface_update_descriptor_set(this->m_vertex_position_buffer->get_read_only_storage_buffer(), this->m_vertex_varying_buffer->get_read_only_storage_buffer(), this->m_index_buffer->get_read_only_storage_buffer(), this->m_auxiliary_buffer->get_read_only_storage_buffer(), (NULL != this->m_emissive_image) ? this->m_emissive_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_normal_image) ? this->m_normal_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_base_color_image) ? this->m_base_color_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_metallic_roughness_image) ? this->m_metallic_roughness_image->get_image()->get_sampled_image() : NULL);
        }
    }
}

inline void brx_anari_pal_surface::uninit(brx_anari_pal_device *device)
{
    // Descriptor
    {
        if (NULL != this->m_vertex_blending_buffer)
        {
            assert(NULL == this->m_surface_update_descriptor_set);
        }
        else
        {
            assert(NULL != this->m_surface_update_descriptor_set);
            device->helper_destroy_descriptor_set(this->m_surface_update_descriptor_set);
            this->m_surface_update_descriptor_set = NULL;
        }
    }

    // Auxiliary Buffer
    {
        assert(NULL != this->m_auxiliary_buffer);
        device->helper_destroy_asset_buffer(this->m_auxiliary_buffer);
        this->m_auxiliary_buffer = NULL;
    }

    // Emissive Image
    if (NULL != this->m_emissive_image)
    {
        device->release_image(this->m_emissive_image);
        this->m_emissive_image = NULL;
    }

    // Normal Image
    if (NULL != this->m_normal_image)
    {
        device->release_image(this->m_normal_image);
        this->m_normal_image = NULL;
    }

    // Base Color Image
    if (NULL != this->m_base_color_image)
    {
        device->release_image(this->m_base_color_image);
        this->m_base_color_image = NULL;
    }

    // Metallic Roughness
    if (NULL != this->m_metallic_roughness_image)
    {
        device->release_image(this->m_metallic_roughness_image);
        this->m_metallic_roughness_image = NULL;
    }

    // Index Buffer
    {
        assert(NULL != this->m_index_buffer);
        device->helper_destroy_asset_buffer(this->m_index_buffer);
        this->m_index_buffer = NULL;
    }

    assert(0U != this->m_index_count);
    this->m_index_count = 0U;

    // Morph Target
    for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
    {
        assert((NULL != this->m_morph_targets_vertex_position_buffers[morph_target_name_index]) == (NULL != this->m_morph_targets_vertex_varying_buffers[morph_target_name_index]));

        // Vertex Position Buffer
        if (NULL != this->m_morph_targets_vertex_position_buffers[morph_target_name_index])
        {
            device->helper_destroy_asset_buffer(this->m_morph_targets_vertex_position_buffers[morph_target_name_index]);
            this->m_morph_targets_vertex_position_buffers[morph_target_name_index] = NULL;
        }

        // Vertex Varying Buffer
        if (NULL != this->m_morph_targets_vertex_varying_buffers[morph_target_name_index])
        {
            device->helper_destroy_asset_buffer(this->m_morph_targets_vertex_varying_buffers[morph_target_name_index]);
            this->m_morph_targets_vertex_varying_buffers[morph_target_name_index] = NULL;
        }
    }

    // Vertex Position Buffer
    {
        assert(NULL != this->m_vertex_position_buffer);
        device->helper_destroy_asset_buffer(this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = NULL;
    }

    // Vertex Varying Buffer
    {
        assert(NULL != this->m_vertex_varying_buffer);
        device->helper_destroy_asset_buffer(this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = NULL;
    }

    // Vertex Blending Buffer
    {
        if (NULL != this->m_vertex_blending_buffer)
        {
            device->helper_destroy_asset_buffer(this->m_vertex_blending_buffer);
            this->m_vertex_blending_buffer = NULL;
        }
    }

    assert(0U != this->m_vertex_count);
    this->m_vertex_count = 0U;
}

uint32_t brx_anari_pal_surface::get_vertex_count() const
{
    return this->m_vertex_count;
}

inline brx_pal_storage_asset_buffer const *brx_anari_pal_surface::get_vertex_position_buffer() const
{
    return this->m_vertex_position_buffer;
}

inline brx_pal_storage_asset_buffer const *brx_anari_pal_surface::get_vertex_varying_buffer() const
{
    return this->m_vertex_varying_buffer;
}

inline brx_pal_storage_asset_buffer const *brx_anari_pal_surface::get_vertex_blending_buffer() const
{
    return this->m_vertex_blending_buffer;
}

brx_pal_storage_asset_buffer const *const *brx_anari_pal_surface::get_morph_targets_vertex_position_buffers() const
{
    return this->m_morph_targets_vertex_position_buffers;
}

brx_pal_storage_asset_buffer const *const *brx_anari_pal_surface::get_morph_targets_vertex_varying_buffers() const
{
    return this->m_morph_targets_vertex_varying_buffers;
}

uint32_t brx_anari_pal_surface::get_index_count() const
{
    return this->m_index_count;
}

inline brx_pal_storage_asset_buffer const *brx_anari_pal_surface::get_index_buffer() const
{
    return this->m_index_buffer;
}

inline brx_pal_storage_asset_buffer const *brx_anari_pal_surface::get_auxiliary_buffer() const
{
    return this->m_auxiliary_buffer;
}

inline brx_anari_pal_image const *brx_anari_pal_surface::get_emissive_image() const
{
    return this->m_emissive_image;
}

inline brx_anari_pal_image const *brx_anari_pal_surface::get_normal_image() const
{
    return this->m_normal_image;
}

inline brx_anari_pal_image const *brx_anari_pal_surface::get_base_color_image() const
{
    return this->m_base_color_image;
}

inline brx_anari_pal_image const *brx_anari_pal_surface::get_metallic_roughness_image() const
{
    return this->m_metallic_roughness_image;
}

bool brx_anari_pal_surface::get_deforming() const
{
    bool skin = (NULL != this->m_vertex_blending_buffer);

    bool morph_target = false;
    for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
    {
        brx_pal_storage_asset_buffer const *const morph_target_vertex_position_buffer = this->m_morph_targets_vertex_position_buffers[morph_target_name_index];
        brx_pal_storage_asset_buffer const *const morph_target_vertex_varying_buffer = this->m_morph_targets_vertex_varying_buffers[morph_target_name_index];
        if ((NULL != morph_target_vertex_position_buffer) && (NULL != morph_target_vertex_varying_buffer))
        {
            morph_target = true;
            break;
        }
        else
        {
            assert(NULL == morph_target_vertex_position_buffer);
            assert(NULL == morph_target_vertex_varying_buffer);
        }
    }

    if (skin || morph_target)
    {
        assert(NULL == this->m_surface_update_descriptor_set);
        return true;
    }
    else
    {

        assert(NULL != this->m_surface_update_descriptor_set);
        return false;
    }
}

brx_pal_descriptor_set const *brx_anari_pal_surface::get_surface_update_descriptor_set() const
{
    assert(!this->get_deforming());
    return this->m_surface_update_descriptor_set;
}

inline brx_anari_pal_surface_group::brx_anari_pal_surface_group() : m_ref_count(0U), m_surfaces{}
{
}

inline brx_anari_pal_surface_group::~brx_anari_pal_surface_group()
{
    assert(0U == this->m_ref_count);
    assert(this->m_surfaces.empty());
}

inline void brx_anari_pal_surface_group::init(brx_anari_pal_device *device, uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces)
{
    assert(0U == this->m_ref_count);
    this->m_ref_count = 1U;

    assert(this->m_surfaces.empty());
    this->m_surfaces.resize(surface_count);
    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
    {
        this->m_surfaces[surface_index].init(device, surfaces + surface_index);
    }
}

inline void brx_anari_pal_surface_group::uninit(brx_anari_pal_device *device)
{
    assert(0U == this->m_ref_count);

    assert(!this->m_surfaces.empty());
    uint32_t const surface_count = this->m_surfaces.size();
    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
    {
        this->m_surfaces[surface_index].uninit(device);
    }
    this->m_surfaces.clear();
}

inline void brx_anari_pal_surface_group::retain()
{
    assert(this->m_ref_count > 0U);
    assert(this->m_ref_count < static_cast<uint32_t>(UINT32_MAX));
    ++this->m_ref_count;
}

uint32_t brx_anari_pal_surface_group::internal_release()
{
    assert(this->m_ref_count > 0U);
    --this->m_ref_count;
    return this->m_ref_count;
}

uint32_t brx_anari_pal_surface_group::get_surface_count() const
{
    return static_cast<uint32_t>(this->m_surfaces.size());
}

brx_anari_pal_surface const *brx_anari_pal_surface_group::get_surfaces() const
{
    return this->m_surfaces.data();
}

inline brx_anari_pal_surface_instance::brx_anari_pal_surface_instance() : m_vertex_position_buffer(NULL), m_vertex_varying_buffer(NULL), m_deforming_surface_update_descriptor_set(NULL), m_surface_update_descriptor_set(NULL)
{
}

inline brx_anari_pal_surface_instance::~brx_anari_pal_surface_instance()
{
    assert(NULL == this->m_vertex_position_buffer);
    assert(NULL == this->m_vertex_varying_buffer);
    assert(NULL == this->m_deforming_surface_update_descriptor_set);
    assert(NULL == this->m_surface_update_descriptor_set);
}

inline void brx_anari_pal_surface_instance::init(brx_anari_pal_device *device, brx_anari_pal_surface const *surface)
{
    if (surface->get_deforming())
    {
        uint32_t const vertex_count = surface->get_vertex_count();

        assert(NULL == this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = device->create_deforming_surface_intermediate_vertex_position_buffer(vertex_count);

        assert(NULL == this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = device->create_deforming_surface_intermediate_vertex_varying_buffer(vertex_count);

        assert(NULL == this->m_deforming_surface_update_descriptor_set);
        assert(NULL != surface->get_vertex_position_buffer());
        assert(NULL != surface->get_vertex_varying_buffer());
        assert(NULL != surface->get_vertex_blending_buffer());
        assert(NULL != this->m_vertex_position_buffer);
        assert(NULL != this->m_vertex_varying_buffer);

        brx_pal_read_only_storage_buffer const *morph_targets_vertex_position_buffers[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
        brx_pal_read_only_storage_buffer const *morph_targets_vertex_varying_buffers[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
        for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
        {
            brx_pal_storage_asset_buffer const *const morph_target_vertex_position_buffer = surface->get_morph_targets_vertex_position_buffers()[morph_target_name_index];
            brx_pal_storage_asset_buffer const *const morph_target_vertex_varying_buffer = surface->get_morph_targets_vertex_varying_buffers()[morph_target_name_index];
            assert((NULL != morph_target_vertex_position_buffer) == (NULL != morph_target_vertex_varying_buffer));
            morph_targets_vertex_position_buffers[morph_target_name_index] = (NULL != morph_target_vertex_position_buffer) ? morph_target_vertex_position_buffer->get_read_only_storage_buffer() : NULL;
            morph_targets_vertex_varying_buffers[morph_target_name_index] = (NULL != morph_target_vertex_varying_buffer) ? morph_target_vertex_varying_buffer->get_read_only_storage_buffer() : NULL;
        }

        this->m_deforming_surface_update_descriptor_set = device->create_deforming_surface_update_descriptor_set(surface->get_vertex_position_buffer()->get_read_only_storage_buffer(), surface->get_vertex_varying_buffer()->get_read_only_storage_buffer(), surface->get_vertex_blending_buffer()->get_read_only_storage_buffer(), morph_targets_vertex_position_buffers, morph_targets_vertex_varying_buffers, this->m_vertex_position_buffer->get_storage_buffer(), this->m_vertex_varying_buffer->get_storage_buffer());

        assert(NULL == this->m_surface_update_descriptor_set);
        assert(NULL != this->m_vertex_position_buffer);
        assert(NULL != this->m_vertex_varying_buffer);
        assert(NULL != surface->get_index_buffer());
        assert(NULL != surface->get_auxiliary_buffer());
        this->m_surface_update_descriptor_set = device->create_surface_update_descriptor_set(this->m_vertex_position_buffer->get_read_only_storage_buffer(), this->m_vertex_varying_buffer->get_read_only_storage_buffer(), surface->get_index_buffer()->get_read_only_storage_buffer(), surface->get_auxiliary_buffer()->get_read_only_storage_buffer(), (NULL != surface->get_emissive_image()) ? surface->get_emissive_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_normal_image()) ? surface->get_normal_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_base_color_image()) ? surface->get_base_color_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_metallic_roughness_image()) ? surface->get_metallic_roughness_image()->get_image()->get_sampled_image() : NULL);
    }
    else
    {
        assert(NULL == this->m_vertex_position_buffer);
        assert(NULL == this->m_vertex_varying_buffer);
        assert(NULL == this->m_deforming_surface_update_descriptor_set);
        assert(NULL == this->m_surface_update_descriptor_set);
    }
}

inline void brx_anari_pal_surface_instance::uninit(brx_anari_pal_device *device, brx_anari_pal_surface const *surface)
{
    if (surface->get_deforming())
    {
        assert(NULL != this->m_deforming_surface_update_descriptor_set);
        device->helper_destroy_descriptor_set(this->m_deforming_surface_update_descriptor_set);
        this->m_deforming_surface_update_descriptor_set = NULL;

        assert(NULL != this->m_surface_update_descriptor_set);
        device->helper_destroy_descriptor_set(this->m_surface_update_descriptor_set);
        this->m_surface_update_descriptor_set = NULL;

        assert(NULL != this->m_vertex_position_buffer);
        device->helper_destroy_intermediate_buffer(this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = NULL;

        assert(NULL != this->m_vertex_varying_buffer);
        device->helper_destroy_intermediate_buffer(this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = NULL;
    }
    else
    {
        assert(NULL == this->m_vertex_position_buffer);
        assert(NULL == this->m_vertex_varying_buffer);
        assert(NULL == this->m_deforming_surface_update_descriptor_set);
        assert(NULL == this->m_surface_update_descriptor_set);
    }
}

brx_pal_storage_intermediate_buffer const *brx_anari_pal_surface_instance::get_vertex_position_buffer() const
{
    return this->m_vertex_position_buffer;
}

brx_pal_storage_intermediate_buffer const *brx_anari_pal_surface_instance::get_vertex_varying_buffer() const
{
    return this->m_vertex_varying_buffer;
}

brx_pal_descriptor_set const *brx_anari_pal_surface_instance::get_deforming_surface_update_descriptor_set() const
{
    assert(this->get_deforming());
    return this->m_deforming_surface_update_descriptor_set;
}

brx_pal_descriptor_set const *brx_anari_pal_surface_instance::get_surface_update_descriptor_set() const
{
    assert(this->get_deforming());
    return this->m_surface_update_descriptor_set;
}

#ifndef NDEBUG
bool brx_anari_pal_surface_instance::get_deforming() const
{
    if ((NULL != this->m_vertex_position_buffer) && (NULL != this->m_vertex_varying_buffer) && (NULL != this->m_deforming_surface_update_descriptor_set) && (NULL != this->m_surface_update_descriptor_set))
    {
        return true;
    }
    else if ((NULL == this->m_vertex_position_buffer) && (NULL == this->m_vertex_varying_buffer) && (NULL == this->m_deforming_surface_update_descriptor_set) && (NULL == this->m_surface_update_descriptor_set))
    {
        return false;
    }
    else
    {
        assert(false);
        return false;
    }
}
#endif

inline brx_anari_pal_surface_group_instance::brx_anari_pal_surface_group_instance() : m_surface_group(NULL), m_surfaces{}, m_deforming_surface_group_update_set_uniform_buffer(NULL), m_deforming_surface_group_update_descriptor_set(NULL), m_surface_group_update_set_uniform_buffer(NULL), m_surface_group_update_descriptor_set(NULL)
{
}

inline brx_anari_pal_surface_group_instance::~brx_anari_pal_surface_group_instance()
{
    assert(NULL == this->m_surface_group);
    assert(this->m_surfaces.empty());
    assert(NULL == this->m_deforming_surface_group_update_set_uniform_buffer);
    assert(NULL == this->m_deforming_surface_group_update_descriptor_set);
    assert(NULL == this->m_surface_group_update_set_uniform_buffer);
    assert(NULL == this->m_surface_group_update_descriptor_set);
}

inline void brx_anari_pal_surface_group_instance::init(brx_anari_pal_device *device, brx_anari_pal_surface_group *surface_group)
{
    assert(NULL == this->m_surface_group);
    surface_group->retain();
    this->m_surface_group = surface_group;

    bool deforming = false;
    uint32_t const surface_count = surface_group->get_surface_count();
    assert(this->m_surfaces.empty());
    this->m_surfaces.resize(surface_count);
    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
    {
        brx_anari_pal_surface const *const surface = surface_group->get_surfaces() + surface_index;
        if ((!deforming) && surface->get_deforming())
        {
            deforming = true;
        }
        this->m_surfaces[surface_index].init(device, surface);
    }

    if (deforming)
    {
        assert(NULL == this->m_deforming_surface_group_update_set_uniform_buffer);
        this->m_deforming_surface_group_update_set_uniform_buffer = device->create_deforming_surface_group_update_set_uniform_buffer();

        assert(NULL == this->m_deforming_surface_group_update_descriptor_set);
        this->m_deforming_surface_group_update_descriptor_set = device->create_deforming_surface_group_update_descriptor_set(this->m_deforming_surface_group_update_set_uniform_buffer);
    }
    else
    {
        assert(NULL == this->m_deforming_surface_group_update_set_uniform_buffer);
        assert(NULL == this->m_deforming_surface_group_update_descriptor_set);
    }

    assert(NULL == this->m_surface_group_update_set_uniform_buffer);
    this->m_surface_group_update_set_uniform_buffer = device->create_surface_group_update_set_uniform_buffer();

    assert(NULL == this->m_surface_group_update_descriptor_set);
    this->m_surface_group_update_descriptor_set = device->create_surface_group_update_descriptor_set(this->m_surface_group_update_set_uniform_buffer);
}

inline void brx_anari_pal_surface_group_instance::uninit(brx_anari_pal_device *device)
{
    bool deforming = false;
    uint32_t const surface_count = this->m_surface_group->get_surface_count();
    assert(this->m_surfaces.size() == surface_count);
    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
    {
        brx_anari_pal_surface const *const surface = this->m_surface_group->get_surfaces() + surface_index;
        if ((!deforming) && surface->get_deforming())
        {
            deforming = true;
        }
        this->m_surfaces[surface_index].uninit(device, surface);
    }
    this->m_surfaces.clear();

    if (deforming)
    {
        assert(NULL != this->m_deforming_surface_group_update_descriptor_set);
        device->helper_destroy_descriptor_set(this->m_deforming_surface_group_update_descriptor_set);
        this->m_deforming_surface_group_update_descriptor_set = NULL;

        assert(NULL != this->m_deforming_surface_group_update_set_uniform_buffer);
        device->helper_destroy_upload_buffer(this->m_deforming_surface_group_update_set_uniform_buffer);
        this->m_deforming_surface_group_update_set_uniform_buffer = NULL;
    }
    else
    {
        assert(NULL == this->m_deforming_surface_group_update_descriptor_set);
        assert(NULL == this->m_deforming_surface_group_update_set_uniform_buffer);
    }

    assert(NULL != this->m_surface_group_update_descriptor_set);
    device->helper_destroy_descriptor_set(this->m_surface_group_update_descriptor_set);
    this->m_surface_group_update_descriptor_set = NULL;

    assert(NULL != this->m_surface_group_update_set_uniform_buffer);
    device->helper_destroy_upload_buffer(this->m_surface_group_update_set_uniform_buffer);
    this->m_surface_group_update_set_uniform_buffer = NULL;

    assert(NULL != this->m_surface_group);
    device->release_surface_group(this->m_surface_group);
    this->m_surface_group = NULL;
}

brx_anari_pal_surface_group const *brx_anari_pal_surface_group_instance::get_surface_group() const
{
    return this->m_surface_group;
}

brx_pal_uniform_upload_buffer const *brx_anari_pal_surface_group_instance::get_deforming_surface_group_update_set_uniform_buffer() const
{
    return this->m_deforming_surface_group_update_set_uniform_buffer;
}

brx_pal_descriptor_set const *brx_anari_pal_surface_group_instance::get_deforming_surface_group_update_descriptor_set() const
{
    return this->m_deforming_surface_group_update_descriptor_set;
}

brx_pal_uniform_upload_buffer const *brx_anari_pal_surface_group_instance::get_surface_group_update_set_uniform_buffer() const
{
    return this->m_surface_group_update_set_uniform_buffer;
}

brx_pal_descriptor_set const *brx_anari_pal_surface_group_instance::get_surface_group_update_descriptor_set() const
{
    return this->m_surface_group_update_descriptor_set;
}

inline uint32_t brx_anari_pal_surface_group_instance::get_surface_count() const
{
    return this->m_surfaces.size();
}

brx_anari_pal_surface_instance const *brx_anari_pal_surface_group_instance::get_surfaces() const
{
    return this->m_surfaces.data();
}

float brx_anari_pal_surface_group_instance::get_morph_target_weight(BRX_ANARI_MORPH_TARGET_NAME morph_target_name) const
{
    uint32_t const morph_target_name_index = morph_target_name;
    return this->m_morph_target_weights[morph_target_name_index];
}

uint32_t brx_anari_pal_surface_group_instance::get_skin_transform_count() const
{
    return this->m_skin_transforms.size();
}

brx_anari_rigid_transform const *brx_anari_pal_surface_group_instance::get_skin_transforms() const
{
    return this->m_skin_transforms.data();
}

brx_anari_rigid_transform brx_anari_pal_surface_group_instance::get_model_transform() const
{
    return this->m_model_transform;
}

void brx_anari_pal_surface_group_instance::set_morph_target_weight(BRX_ANARI_MORPH_TARGET_NAME morph_target_name, float morph_target_weight)
{
    this->m_morph_target_weights[morph_target_name] = morph_target_weight;
}

void brx_anari_pal_surface_group_instance::set_skin_transforms(uint32_t skin_transform_count, brx_anari_rigid_transform const *skin_transforms)
{
    this->m_skin_transforms.resize(skin_transform_count);
    std::memcpy(this->m_skin_transforms.data(), skin_transforms, sizeof(brx_anari_rigid_transform) * skin_transform_count);
}

void brx_anari_pal_surface_group_instance::set_model_transform(brx_anari_rigid_transform model_transform)
{
    this->m_model_transform = model_transform;
}
