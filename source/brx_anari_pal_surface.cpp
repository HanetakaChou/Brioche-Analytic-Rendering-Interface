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
#include "../shaders/surface.sli"
#include "../shaders/deforming_resource_binding.sli"
#include "../shaders/forward_shading_resource_binding.sli"
#if defined(__GNUC__)
// GCC or CLANG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
// MSVC or CLANG-CL
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#else
#error Unknown Compiler
#endif
#include "../../Environment-Lighting/include/brx_octahedral_mapping.h"
#include "../../Packed-Vector/include/brx_packed_vector.h"

inline brx_pal_uniform_upload_buffer *brx_anari_pal_device::create_upload_buffer(uint32_t const size)
{
    assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);
    brx_pal_uniform_upload_buffer *const buffer = this->m_device->create_uniform_upload_buffer(internal_align_up(size, this->m_uniform_upload_buffer_offset_alignment) * INTERNAL_FRAME_THROTTLING_COUNT);
    return buffer;
}

inline void brx_anari_pal_device::destroy_upload_buffer(brx_pal_uniform_upload_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_uniform_upload_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

inline brx_pal_storage_intermediate_buffer *brx_anari_pal_device::create_intermediate_buffer(uint32_t const size)
{
    return this->m_device->create_storage_intermediate_buffer(size);
}

inline void brx_anari_pal_device::destroy_intermediate_buffer(brx_pal_storage_intermediate_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_storage_intermediate_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

inline brx_pal_storage_asset_buffer *brx_anari_pal_device::internal_create_asset_buffer(void const *const data_base, uint32_t const data_size)
{
    // TODO: batch

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

        upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(destination_asset_buffer, 0U, buffer_staging_upload_buffer, 0U, data_size);

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

inline void brx_anari_pal_device::internal_destroy_asset_buffer(brx_pal_storage_asset_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_storage_asset_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_deforming_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_deforming_descriptor_set_layout_per_surface_group_update, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(deforming_per_surface_group_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_deforming_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const vertex_blending_buffer, brx_pal_storage_buffer const *const vertex_position_buffer_instance, brx_pal_storage_buffer const *const vertex_varying_buffer_instance)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_deforming_descriptor_set_layout_per_surface_update, 0U);

    assert(NULL != vertex_position_buffer);
    assert(NULL != vertex_varying_buffer);
    assert(NULL != vertex_blending_buffer);
    assert(NULL != vertex_position_buffer_instance);
    assert(NULL != vertex_varying_buffer_instance);

    brx_pal_read_only_storage_buffer const *surface_buffers[Deforming_Surface_Buffer_Count];
    surface_buffers[Deforming_Surface_Buffer_Vertex_Position_Index] = vertex_position_buffer;
    surface_buffers[Deforming_Surface_Buffer_Vertex_Varying_Index] = vertex_varying_buffer;
    surface_buffers[Deforming_Surface_Buffer_Vertex_Blending_Index] = vertex_blending_buffer;
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, Deforming_Surface_Buffer_Count, NULL, NULL, surface_buffers, NULL, NULL, NULL, NULL, NULL);

    brx_pal_storage_buffer const *surface_instance_buffers[Deforming_Surface_Buffer_Instance_Count];
    surface_instance_buffers[Deforming_Surface_Buffer_Instance_Vertex_Position_Index] = vertex_position_buffer_instance;
    surface_instance_buffers[Deforming_Surface_Buffer_Instance_Vertex_Varying_Index] = vertex_varying_buffer_instance;
    this->m_device->write_descriptor_set(descriptor_set, 1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, Deforming_Surface_Buffer_Instance_Count, NULL, NULL, NULL, surface_instance_buffers, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_forward_shading_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_forward_shading_descriptor_set_layout_per_surface_group_update, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(forward_shading_per_surface_group_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

inline brx_pal_descriptor_set *brx_anari_pal_device::create_forward_shading_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const index_buffer, brx_pal_read_only_storage_buffer const *const auxiliary_buffer, brx_pal_sampled_image const *const emissive_image, brx_pal_sampled_image const *const normal_image, brx_pal_sampled_image const *const base_color_image, brx_pal_sampled_image const *const metallic_roughness_image)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_forward_shading_descriptor_set_layout_per_surface_update, 0U);

    assert(NULL != vertex_position_buffer);
    assert(NULL != vertex_varying_buffer);
    assert(NULL != index_buffer);
    assert(NULL != auxiliary_buffer);

    brx_pal_read_only_storage_buffer const *surface_buffers[Forward_Shading_Surface_Buffer_Count];
    surface_buffers[Forward_Shading_Surface_Buffer_Vertex_Position_Index] = vertex_position_buffer;
    surface_buffers[Forward_Shading_Surface_Buffer_Vertex_Varying_Index] = vertex_varying_buffer;
    surface_buffers[Forward_Shading_Surface_Buffer_Index_Index] = index_buffer;
    surface_buffers[Forward_Shading_Surface_Buffer_Auxiliary_Index] = auxiliary_buffer;
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, Forward_Shading_Surface_Buffer_Count, NULL, NULL, surface_buffers, NULL, NULL, NULL, NULL, NULL);

    brx_pal_sampled_image const *surface_images[Forward_Shading_Surface_Texture_Count];
    surface_images[Forward_Shading_Surface_Texture_Emissive_Index] = (NULL != emissive_image) ? emissive_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[Forward_Shading_Surface_Texture_Normal_Index] = (NULL != normal_image) ? normal_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[Forward_Shading_Surface_Texture_Base_Color_Index] = (NULL != base_color_image) ? base_color_image : this->m_place_holder_asset_image->get_sampled_image();
    surface_images[Forward_Shading_Surface_Texture_Metallic_Roughness_Index] = (NULL != metallic_roughness_image) ? metallic_roughness_image : this->m_place_holder_asset_image->get_sampled_image();
    this->m_device->write_descriptor_set(descriptor_set, Forward_Shading_Surface_Buffer_Count, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, Forward_Shading_Surface_Texture_Count, NULL, NULL, NULL, NULL, surface_images, NULL, NULL, NULL);

    return descriptor_set;
}

inline void brx_anari_pal_device::destroy_descriptor_set(brx_pal_descriptor_set *descriptor_set)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != descriptor_set);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_descriptor_sets[previous_frame_throttling_index].push_back(descriptor_set);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
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
    return new_unwrapped_surface_group_instance;
}

void brx_anari_pal_device::world_release_surface_group_instance(brx_anari_surface_group_instance *wrapped_surface_group_instance)
{
    brx_anari_pal_surface_group_instance *const delete_unwrapped_surface_group_instance = static_cast<brx_anari_pal_surface_group_instance *>(wrapped_surface_group_instance);

    delete_unwrapped_surface_group_instance->uninit(this);

    delete_unwrapped_surface_group_instance->~brx_anari_pal_surface_group_instance();
    mcrt_free(delete_unwrapped_surface_group_instance);
}

inline brx_anari_pal_surface::brx_anari_pal_surface() : m_vertex_count(0U), m_vertex_position_buffer(NULL), m_vertex_varying_buffer(NULL), m_vertex_blending_buffer(NULL), m_index_count(0U), m_index_buffer(NULL), m_emissive_image(NULL), m_normal_image(NULL), m_base_color_image(NULL), m_metallic_roughness_image(NULL), m_auxiliary_buffer(NULL), m_forward_shading_descriptor_set_per_surface_update(NULL)
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
    assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
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

            DirectX::XMFLOAT2 const mapped_normal = brx_octahedral_map(raw_normal);

            DirectX::PackedVector::XMSHORTN2 packed_normal;
            DirectX::PackedVector::XMStoreShortN2(&packed_normal, DirectX::XMLoadFloat2(&mapped_normal));

            vertex_varying_buffer_data[vertex_index].m_normal = packed_normal.v;

            DirectX::XMFLOAT3 const raw_tangent_xyz(surface->m_vertex_varyings[vertex_index].m_tangent[0], surface->m_vertex_varyings[vertex_index].m_tangent[1], surface->m_vertex_varyings[vertex_index].m_tangent[2]);
            float const raw_tangent_w = surface->m_vertex_varyings[vertex_index].m_tangent[3];

            vertex_varying_buffer_data[vertex_index].m_tangent = brx_FLOAT3_to_R15G15B2_SNORM(brx_octahedral_map(raw_tangent_xyz), raw_tangent_w);

            DirectX::XMFLOAT2 const raw_texcoord(surface->m_vertex_varyings[vertex_index].m_texcoord[0], surface->m_vertex_varyings[vertex_index].m_texcoord[1]);

            DirectX::PackedVector::XMUSHORTN2 packed_texcoord;
            DirectX::PackedVector::XMStoreUShortN2(&packed_texcoord, DirectX::XMLoadFloat2(&raw_texcoord));

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

    uint32_t surface_auxiliary_buffer_texture_flags = 0U;

    assert(0U == this->m_index_count);
    this->m_index_count = surface->m_index_count;

    // Index Buffer
    {
        uint32_t raw_max_index = 0U;
        for (size_t index_index = 0; index_index < surface->m_index_count; ++index_index)
        {
            uint32_t const raw_index = surface->m_indices[index_index];

            raw_max_index = std::max(raw_max_index, raw_index);
        }

        if (raw_max_index <= static_cast<uint32_t>(UINT16_MAX))
        {
            surface_auxiliary_buffer_texture_flags |= Surface_Buffer_Flag_UInt16_Index;

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

    // Emissive Image
    {
        if (NULL != surface->m_emissive_image)
        {
            surface_auxiliary_buffer_texture_flags |= Surface_Texture_Flag_Emissive;

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
            surface_auxiliary_buffer_texture_flags |= Surface_Texture_Flag_Normal;

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
            surface_auxiliary_buffer_texture_flags |= Surface_Texture_Flag_Base_Color;

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
            surface_auxiliary_buffer_texture_flags |= Surface_Texture_Flag_Metallic_Roughness;

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

        surface_auxiliary_buffer_data.m_normal_scale = surface->m_normal_scale;

        surface_auxiliary_buffer_data.m_base_color_factor_x = surface->m_base_color_factor.m_x;
        surface_auxiliary_buffer_data.m_base_color_factor_y = surface->m_base_color_factor.m_y;
        surface_auxiliary_buffer_data.m_base_color_factor_z = surface->m_base_color_factor.m_z;
        // TODO
        // surface_auxiliary_buffer_data.m_base_color_factor_w = surface->m_base_color_factor.m_w;

        surface_auxiliary_buffer_data.m_metallic_factor = surface->m_metallic_factor;
        surface_auxiliary_buffer_data.m_roughness_factor = surface->m_roughness_factor;

        assert(NULL == this->m_auxiliary_buffer);
        this->m_auxiliary_buffer = device->internal_create_asset_buffer(&surface_auxiliary_buffer_data, sizeof(surface_auxiliary_buffer));
    }

    // Descriptor
    {
        if (NULL != surface->m_vertex_blendings)
        {
            assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
        }
        else
        {
            assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
            assert(NULL != this->m_vertex_position_buffer);
            assert(NULL != this->m_vertex_varying_buffer);
            assert(NULL != this->m_index_buffer);
            assert(NULL != this->m_auxiliary_buffer);
            this->m_forward_shading_descriptor_set_per_surface_update = device->create_forward_shading_per_surface_update_descriptor_set(this->m_vertex_position_buffer->get_read_only_storage_buffer(), this->m_vertex_varying_buffer->get_read_only_storage_buffer(), this->m_index_buffer->get_read_only_storage_buffer(), this->m_auxiliary_buffer->get_read_only_storage_buffer(), (NULL != this->m_emissive_image) ? this->m_emissive_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_normal_image) ? this->m_normal_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_base_color_image) ? this->m_base_color_image->get_image()->get_sampled_image() : NULL, (NULL != this->m_metallic_roughness_image) ? this->m_metallic_roughness_image->get_image()->get_sampled_image() : NULL);
        }
    }
}

inline void brx_anari_pal_surface::uninit(brx_anari_pal_device *device)
{
    // Descriptor
    {
        if (NULL != this->m_vertex_blending_buffer)
        {
            assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
        }
        else
        {
            assert(NULL != this->m_forward_shading_descriptor_set_per_surface_update);
            device->destroy_descriptor_set(this->m_forward_shading_descriptor_set_per_surface_update);
            this->m_forward_shading_descriptor_set_per_surface_update = NULL;
        }
    }

    // Auxiliary Buffer
    {
        assert(NULL != this->m_auxiliary_buffer);
        device->internal_destroy_asset_buffer(this->m_auxiliary_buffer);
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
        device->internal_destroy_asset_buffer(this->m_index_buffer);
        this->m_index_buffer = NULL;
    }

    assert(0U != this->m_index_count);
    this->m_index_count = 0U;

    // Vertex Position Buffer
    {
        assert(NULL != this->m_vertex_position_buffer);
        device->internal_destroy_asset_buffer(this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = NULL;
    }

    // Vertex Varying Buffer
    {
        assert(NULL != this->m_vertex_varying_buffer);
        device->internal_destroy_asset_buffer(this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = NULL;
    }

    // Vertex Blending Buffer
    {
        if (NULL != this->m_vertex_blending_buffer)
        {
            device->internal_destroy_asset_buffer(this->m_vertex_blending_buffer);
            this->m_vertex_blending_buffer = NULL;
        }
    }

    assert(0U != this->m_vertex_count);
    this->m_vertex_count = 0U;
}

inline uint32_t brx_anari_pal_surface::get_vertex_count() const
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

inline bool brx_anari_pal_surface::get_deforming() const
{
    if (NULL != this->m_vertex_blending_buffer)
    {
        assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
        return true;
    }
    else
    {
        assert(NULL != this->m_forward_shading_descriptor_set_per_surface_update);
        return false;
    }
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

inline uint32_t brx_anari_pal_surface_group::get_surface_count() const
{
    return static_cast<uint32_t>(this->m_surfaces.size());
}

inline brx_anari_pal_surface const *brx_anari_pal_surface_group::get_surfaces() const
{
    return this->m_surfaces.data();
}

inline brx_anari_pal_surface_instance::brx_anari_pal_surface_instance() : m_vertex_position_buffer(NULL), m_vertex_varying_buffer(NULL), m_deforming_descriptor_set_per_surface_update(NULL), m_forward_shading_descriptor_set_per_surface_update(NULL)
{
}

inline brx_anari_pal_surface_instance::~brx_anari_pal_surface_instance()
{
    assert(NULL == this->m_vertex_position_buffer);
    assert(NULL == this->m_vertex_varying_buffer);
    assert(NULL == this->m_deforming_descriptor_set_per_surface_update);
    assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
}

inline void brx_anari_pal_surface_instance::init(brx_anari_pal_device *device, brx_anari_pal_surface const *surface)
{
    if (surface->get_deforming())
    {
        uint32_t const vertex_count = surface->get_vertex_count();

        assert(NULL == this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = device->create_intermediate_buffer(sizeof(surface_vertex_position_buffer_element) * vertex_count);

        assert(NULL == this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = device->create_intermediate_buffer(sizeof(surface_vertex_varying_buffer_element) * vertex_count);

        assert(NULL == this->m_deforming_descriptor_set_per_surface_update);
        assert(NULL != surface->get_vertex_position_buffer());
        assert(NULL != surface->get_vertex_varying_buffer());
        assert(NULL != surface->get_vertex_blending_buffer());
        assert(NULL != this->m_vertex_position_buffer);
        assert(NULL != this->m_vertex_varying_buffer);
        this->m_deforming_descriptor_set_per_surface_update = device->create_deforming_per_surface_update_descriptor_set(surface->get_vertex_position_buffer()->get_read_only_storage_buffer(), surface->get_vertex_varying_buffer()->get_read_only_storage_buffer(), surface->get_vertex_blending_buffer()->get_read_only_storage_buffer(), this->m_vertex_position_buffer->get_storage_buffer(), this->m_vertex_varying_buffer->get_storage_buffer());

        assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
        assert(NULL != this->m_vertex_position_buffer);
        assert(NULL != this->m_vertex_varying_buffer);
        assert(NULL != surface->get_index_buffer());
        assert(NULL != surface->get_auxiliary_buffer());
        this->m_forward_shading_descriptor_set_per_surface_update = device->create_forward_shading_per_surface_update_descriptor_set(this->m_vertex_position_buffer->get_read_only_storage_buffer(), this->m_vertex_varying_buffer->get_read_only_storage_buffer(), surface->get_index_buffer()->get_read_only_storage_buffer(), surface->get_auxiliary_buffer()->get_read_only_storage_buffer(), (NULL != surface->get_emissive_image()) ? surface->get_emissive_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_normal_image()) ? surface->get_normal_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_base_color_image()) ? surface->get_base_color_image()->get_image()->get_sampled_image() : NULL, (NULL != surface->get_metallic_roughness_image()) ? surface->get_metallic_roughness_image()->get_image()->get_sampled_image() : NULL);
    }
    else
    {
        assert(NULL == this->m_vertex_position_buffer);
        assert(NULL == this->m_vertex_varying_buffer);
        assert(NULL == this->m_deforming_descriptor_set_per_surface_update);
        assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
    }
}

inline void brx_anari_pal_surface_instance::uninit(brx_anari_pal_device *device, brx_anari_pal_surface const *surface)
{
    if (surface->get_deforming())
    {
        assert(NULL != this->m_deforming_descriptor_set_per_surface_update);
        device->destroy_descriptor_set(this->m_deforming_descriptor_set_per_surface_update);
        this->m_deforming_descriptor_set_per_surface_update = NULL;

        assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
        device->destroy_descriptor_set(this->m_forward_shading_descriptor_set_per_surface_update);
        this->m_forward_shading_descriptor_set_per_surface_update = NULL;

        assert(NULL != this->m_vertex_position_buffer);
        device->destroy_intermediate_buffer(this->m_vertex_position_buffer);
        this->m_vertex_position_buffer = NULL;

        assert(NULL != this->m_vertex_varying_buffer);
        device->destroy_intermediate_buffer(this->m_vertex_varying_buffer);
        this->m_vertex_varying_buffer = NULL;
    }
    else
    {
        assert(NULL == this->m_vertex_position_buffer);
        assert(NULL == this->m_vertex_varying_buffer);
        assert(NULL == this->m_deforming_descriptor_set_per_surface_update);
        assert(NULL == this->m_forward_shading_descriptor_set_per_surface_update);
    }
}

inline brx_anari_pal_surface_group_instance::brx_anari_pal_surface_group_instance() : m_surface_group(NULL), m_surfaces{}
{
}

inline brx_anari_pal_surface_group_instance::~brx_anari_pal_surface_group_instance()
{
    assert(NULL == this->m_surface_group);
    assert(this->m_surfaces.empty());
    assert(NULL == this->m_deforming_per_surface_group_update_set_uniform_buffer);
    assert(NULL == this->m_deforming_descriptor_set_per_surface_group_update);
    assert(NULL == this->m_forward_shading_per_surface_group_update_set_uniform_buffer);
    assert(NULL == this->m_forward_shading_descriptor_set_per_surface_group_update);
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
        assert(NULL == this->m_deforming_per_surface_group_update_set_uniform_buffer);
        this->m_deforming_per_surface_group_update_set_uniform_buffer = device->create_upload_buffer(sizeof(deforming_per_surface_group_update_set_uniform_buffer_binding));

        assert(NULL == this->m_deforming_descriptor_set_per_surface_group_update);
        this->m_deforming_descriptor_set_per_surface_group_update = device->create_deforming_per_surface_group_update_descriptor_set(this->m_deforming_per_surface_group_update_set_uniform_buffer);
    }
    else
    {
        assert(NULL == this->m_deforming_per_surface_group_update_set_uniform_buffer);
        assert(NULL == this->m_deforming_descriptor_set_per_surface_group_update);
    }

    assert(NULL == this->m_forward_shading_per_surface_group_update_set_uniform_buffer);
    this->m_forward_shading_per_surface_group_update_set_uniform_buffer = device->create_upload_buffer(sizeof(forward_shading_per_surface_group_update_set_uniform_buffer_binding));

    assert(NULL == this->m_forward_shading_descriptor_set_per_surface_group_update);
    this->m_forward_shading_descriptor_set_per_surface_group_update = device->create_forward_shading_per_surface_group_update_descriptor_set(this->m_forward_shading_per_surface_group_update_set_uniform_buffer);
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
        assert(NULL != this->m_deforming_descriptor_set_per_surface_group_update);
        device->destroy_descriptor_set(this->m_deforming_descriptor_set_per_surface_group_update);
        this->m_deforming_descriptor_set_per_surface_group_update = NULL;

        assert(NULL != this->m_deforming_per_surface_group_update_set_uniform_buffer);
        device->destroy_upload_buffer(this->m_deforming_per_surface_group_update_set_uniform_buffer);
        this->m_deforming_per_surface_group_update_set_uniform_buffer = NULL;
    }
    else
    {
        assert(NULL == this->m_deforming_descriptor_set_per_surface_group_update);
        assert(NULL == this->m_deforming_per_surface_group_update_set_uniform_buffer);
    }

    assert(NULL != this->m_forward_shading_descriptor_set_per_surface_group_update);
    device->destroy_descriptor_set(this->m_forward_shading_descriptor_set_per_surface_group_update);
    this->m_forward_shading_descriptor_set_per_surface_group_update = NULL;

    assert(NULL != this->m_forward_shading_per_surface_group_update_set_uniform_buffer);
    device->destroy_upload_buffer(this->m_forward_shading_per_surface_group_update_set_uniform_buffer);
    this->m_forward_shading_per_surface_group_update_set_uniform_buffer = NULL;

    assert(NULL != this->m_surface_group);
    device->release_surface_group(this->m_surface_group);
    this->m_surface_group = NULL;
}
