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
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing.h"
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_resource.h"
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_zero.h"
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_clear.h"
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_voxelization.h"
#include "../../Voxel-Cone-Tracing/include/brx_voxel_cone_tracing_pack.h"
#include "../shaders/forward_shading_resource_binding.bsli"
#include "../shaders/post_processing_resource_binding.bsli"

void brx_anari_pal_device::voxel_cone_tracing_create_none_update_binding_resource()
{
    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_mask);

        DirectX::XMUINT3 const clipmap_mask_texture_extent = brx_voxel_cone_tracing_resource_clipmap_mask_texture_extent();

        this->m_voxel_cone_tracing_clipmap_mask = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R32_UINT, clipmap_mask_texture_extent.x, clipmap_mask_texture_extent.y, true, clipmap_mask_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_opacity_r32);

        DirectX::XMUINT3 const clipmap_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_opacity_texture_extent();

        this->m_voxel_cone_tracing_clipmap_opacity_r32 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R32_WRITE_UINT_READ_SFLOAT, clipmap_opacity_texture_extent.x, clipmap_opacity_texture_extent.y, true, clipmap_opacity_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_opacity_r16);

        DirectX::XMUINT3 const clipmap_opacity_texture_extent = brx_voxel_cone_tracing_resource_clipmap_opacity_texture_extent();

        this->m_voxel_cone_tracing_clipmap_opacity_r16 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R16_SFLOAT, clipmap_opacity_texture_extent.x, clipmap_opacity_texture_extent.y, true, clipmap_opacity_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_r32);

        DirectX::XMUINT3 const clipmap_illumination_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_texture_extent();

        this->m_voxel_cone_tracing_clipmap_illumination_r32 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R32_WRITE_UINT_READ_SFLOAT, clipmap_illumination_texture_extent.x, clipmap_illumination_texture_extent.y, true, clipmap_illumination_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_g32);

        DirectX::XMUINT3 const clipmap_illumination_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_texture_extent();

        this->m_voxel_cone_tracing_clipmap_illumination_g32 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R32_WRITE_UINT_READ_SFLOAT, clipmap_illumination_texture_extent.x, clipmap_illumination_texture_extent.y, true, clipmap_illumination_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_b32);

        DirectX::XMUINT3 const clipmap_illumination_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_texture_extent();

        this->m_voxel_cone_tracing_clipmap_illumination_b32 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R32_WRITE_UINT_READ_SFLOAT, clipmap_illumination_texture_extent.x, clipmap_illumination_texture_extent.y, true, clipmap_illumination_texture_extent.z, true);
    }

    {
        assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16);

        DirectX::XMUINT3 const clipmap_illumination_texture_extent = brx_voxel_cone_tracing_resource_clipmap_illumination_texture_extent();

        this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16 = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R16G16B16A16_SFLOAT, clipmap_illumination_texture_extent.x, clipmap_illumination_texture_extent.y, true, clipmap_illumination_texture_extent.z, true);
    }
}

void brx_anari_pal_device::voxel_cone_tracing_destroy_none_update_binding_resource()
{
    assert(NULL != this->m_voxel_cone_tracing_clipmap_mask);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_mask);
    this->m_voxel_cone_tracing_clipmap_mask = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_opacity_r32);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_opacity_r32);
    this->m_voxel_cone_tracing_clipmap_opacity_r32 = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_opacity_r16);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_opacity_r16);
    this->m_voxel_cone_tracing_clipmap_opacity_r16 = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_illumination_r32);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_illumination_r32);
    this->m_voxel_cone_tracing_clipmap_illumination_r32 = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_illumination_g32);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_illumination_g32);
    this->m_voxel_cone_tracing_clipmap_illumination_g32 = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_illumination_b32);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_illumination_b32);
    this->m_voxel_cone_tracing_clipmap_illumination_b32 = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16);
    this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16 = NULL;
}

void brx_anari_pal_device::voxel_cone_tracing_create_screen_size_dependent_none_update_binding_resource()
{
    uint32_t const width = std::max(1U, (this->m_intermediate_width + 1U) / 2U);
    uint32_t const height = std::max(1U, (this->m_intermediate_height + 1U) / 2U);

    assert(NULL == this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion);
    this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion = this->m_device->create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT_R16G16B16A16_SFLOAT, width, height, false, 1U, true);
}

void brx_anari_pal_device::voxel_cone_tracing_destroy_screen_size_dependent_none_update_binding_resource()
{
    assert(NULL != this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion);
    this->m_device->destroy_storage_image(this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion);
    this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion = NULL;
}

void brx_anari_pal_device::voxel_cone_tracing_create_pipeline()
{
    assert(NULL != this->m_post_processing_pipeline_layout);

    {
        assert(NULL == this->m_voxel_cone_tracing_zero_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_zero_compute.inl"
            this->m_voxel_cone_tracing_zero_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_zero_compute_shader_module_code), voxel_cone_tracing_zero_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_zero_compute.inl"
            this->m_voxel_cone_tracing_zero_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_zero_compute_shader_module_code), voxel_cone_tracing_zero_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }

        assert(NULL == this->m_voxel_cone_tracing_clear_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_clear_compute.inl"
            this->m_voxel_cone_tracing_clear_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_clear_compute_shader_module_code), voxel_cone_tracing_clear_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_clear_compute.inl"
            this->m_voxel_cone_tracing_clear_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_clear_compute_shader_module_code), voxel_cone_tracing_clear_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }

        assert(NULL == this->m_voxel_cone_tracing_pack_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_pack_compute.inl"
            this->m_voxel_cone_tracing_pack_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_pack_compute_shader_module_code), voxel_cone_tracing_pack_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_pack_compute.inl"
            this->m_voxel_cone_tracing_pack_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_pack_compute_shader_module_code), voxel_cone_tracing_pack_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }

        assert(NULL == this->m_voxel_cone_tracing_cone_tracing_low_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_cone_tracing_low_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_low_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_low_compute_shader_module_code), voxel_cone_tracing_cone_tracing_low_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_cone_tracing_low_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_low_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_low_compute_shader_module_code), voxel_cone_tracing_cone_tracing_low_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }

        assert(NULL == this->m_voxel_cone_tracing_cone_tracing_medium_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_cone_tracing_medium_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_medium_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_medium_compute_shader_module_code), voxel_cone_tracing_cone_tracing_medium_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_cone_tracing_medium_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_medium_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_medium_compute_shader_module_code), voxel_cone_tracing_cone_tracing_medium_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }

        assert(NULL == this->m_voxel_cone_tracing_cone_tracing_high_pipeline);
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_cone_tracing_high_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_high_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_high_compute_shader_module_code), voxel_cone_tracing_cone_tracing_high_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_cone_tracing_high_compute.inl"
            this->m_voxel_cone_tracing_cone_tracing_high_pipeline = this->m_device->create_compute_pipeline(this->m_post_processing_pipeline_layout, sizeof(voxel_cone_tracing_cone_tracing_high_compute_shader_module_code), voxel_cone_tracing_cone_tracing_high_compute_shader_module_code);
        }
        break;
        default:
        {
            assert(false);
        }
        }
    }

    assert(NULL != this->m_forward_shading_pipeline_layout);

    {
        assert(NULL == this->m_voxel_cone_tracing_voxelization_render_pass);
        this->m_voxel_cone_tracing_voxelization_render_pass = this->m_device->create_render_pass(0U, NULL, NULL);

        assert(NULL == this->m_voxel_cone_tracing_voxelization_pipeline);

        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/voxel_cone_tracing_voxelization_vertex.inl"
#include "../shaders/dxil/voxel_cone_tracing_voxelization_fragment.inl"
            this->m_voxel_cone_tracing_voxelization_pipeline = this->m_device->create_graphics_pipeline(this->m_voxel_cone_tracing_voxelization_render_pass, this->m_forward_shading_pipeline_layout, sizeof(voxel_cone_tracing_voxelization_vertex_shader_module_code), voxel_cone_tracing_voxelization_vertex_shader_module_code, sizeof(voxel_cone_tracing_voxelization_fragment_shader_module_code), voxel_cone_tracing_voxelization_fragment_shader_module_code, false, true, false, 8U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/voxel_cone_tracing_voxelization_vertex.inl"
#include "../shaders/spirv/voxel_cone_tracing_voxelization_fragment.inl"
            this->m_voxel_cone_tracing_voxelization_pipeline = this->m_device->create_graphics_pipeline(this->m_voxel_cone_tracing_voxelization_render_pass, this->m_forward_shading_pipeline_layout, sizeof(voxel_cone_tracing_voxelization_vertex_shader_module_code), voxel_cone_tracing_voxelization_vertex_shader_module_code, sizeof(voxel_cone_tracing_voxelization_fragment_shader_module_code), voxel_cone_tracing_voxelization_fragment_shader_module_code, false, true, false, 8U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        default:
        {
            assert(false);
        }
        }
    }

    assert(NULL == this->m_voxel_cone_tracing_voxelization_frame_buffer);
    this->m_voxel_cone_tracing_voxelization_frame_buffer = this->m_device->create_frame_buffer(this->m_voxel_cone_tracing_voxelization_render_pass, static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), 0U, NULL, NULL);
}

void brx_anari_pal_device::voxel_cone_tracing_destroy_pipeline()
{
    assert(NULL != this->m_voxel_cone_tracing_voxelization_frame_buffer);
    this->m_device->destroy_frame_buffer(this->m_voxel_cone_tracing_voxelization_frame_buffer);
    this->m_voxel_cone_tracing_voxelization_frame_buffer = NULL;

    assert(NULL != this->m_voxel_cone_tracing_voxelization_pipeline);
    this->m_device->destroy_graphics_pipeline(this->m_voxel_cone_tracing_voxelization_pipeline);
    this->m_voxel_cone_tracing_voxelization_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_voxelization_render_pass);
    this->m_device->destroy_render_pass(this->m_voxel_cone_tracing_voxelization_render_pass);
    this->m_voxel_cone_tracing_voxelization_render_pass = NULL;

    assert(NULL != this->m_voxel_cone_tracing_cone_tracing_low_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_cone_tracing_low_pipeline);
    this->m_voxel_cone_tracing_cone_tracing_low_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_cone_tracing_medium_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_cone_tracing_medium_pipeline);
    this->m_voxel_cone_tracing_cone_tracing_medium_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_cone_tracing_high_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_cone_tracing_high_pipeline);
    this->m_voxel_cone_tracing_cone_tracing_high_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_pack_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_pack_pipeline);
    this->m_voxel_cone_tracing_pack_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_clear_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_clear_pipeline);
    this->m_voxel_cone_tracing_clear_pipeline = NULL;

    assert(NULL != this->m_voxel_cone_tracing_zero_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_voxel_cone_tracing_zero_pipeline);
    this->m_voxel_cone_tracing_zero_pipeline = NULL;
}

void brx_anari_pal_device::renderer_set_gi_quality(BRX_ANARI_RENDERER_GI_QUALITY gi_quality)
{
#ifndef NDEBUG
    assert(!this->m_renderer_gi_quality_lock);
    this->m_renderer_gi_quality_lock = true;

    assert(!this->m_voxel_cone_tracing_dirty_lock);
    this->m_voxel_cone_tracing_dirty_lock = true;
#endif

    if (this->m_renderer_gi_quality != gi_quality)
    {
        this->m_renderer_gi_quality = gi_quality;

        this->m_voxel_cone_tracing_dirty = true;
    }

#ifndef NDEBUG
    this->m_voxel_cone_tracing_dirty_lock = false;

    this->m_renderer_gi_quality_lock = false;
#endif
}

BRX_ANARI_RENDERER_GI_QUALITY brx_anari_pal_device::renderer_get_gi_quality() const
{
    return this->m_renderer_gi_quality;
}

DirectX::XMFLOAT3 brx_anari_pal_device::voxel_cone_tracing_get_clipmap_anchor(DirectX::XMFLOAT3 const &in_eye_position, DirectX::XMFLOAT3 const &in_eye_direction)
{
    return brx_voxel_cone_tracing_resource_clipmap_anchor(in_eye_position, in_eye_direction);
}

DirectX::XMFLOAT3 brx_anari_pal_device::voxel_cone_tracing_get_clipmap_center(DirectX::XMFLOAT3 const &in_clipmap_anchor)
{
    return brx_voxel_cone_tracing_resource_clipmap_center(in_clipmap_anchor);
}

DirectX::XMFLOAT4X4 brx_anari_pal_device::voxel_cone_tracing_get_viewport_depth_direction_view_matrix(DirectX::XMFLOAT3 const &in_clipmap_center, uint32_t viewport_depth_direction_index)
{
    return brx_voxel_cone_tracing_voxelization_compute_viewport_depth_direction_view_matrix(in_clipmap_center, viewport_depth_direction_index);
}

DirectX::XMFLOAT4X4 brx_anari_pal_device::voxel_cone_tracing_get_clipmap_stack_level_projection_matrix(uint32_t clipmap_stack_level_index)
{
    return brx_voxel_cone_tracing_voxelization_compute_clipmap_stack_level_projection_matrix(clipmap_stack_level_index);
}

void brx_anari_pal_device::voxel_cone_tracing_render(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, bool &inout_voxel_cone_tracing_dirty, BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality)
{
    if ((BRX_ANARI_RENDERER_GI_QUALITY_LOW == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality))
    {
        graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Pass");

        if (inout_voxel_cone_tracing_dirty)
        {
            graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Zero Pass");

            {
                brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_mask, this->m_voxel_cone_tracing_clipmap_opacity_r32, this->m_voxel_cone_tracing_clipmap_opacity_r16, this->m_voxel_cone_tracing_clipmap_illumination_r32, this->m_voxel_cone_tracing_clipmap_illumination_g32, this->m_voxel_cone_tracing_clipmap_illumination_b32, this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16};

                graphics_command_buffer->storage_resource_load_dont_care(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
            }

            graphics_command_buffer->bind_compute_pipeline(this->m_voxel_cone_tracing_zero_pipeline);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_post_processing_descriptor_set_none_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<post_processing_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_post_processing_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            {
                DirectX::XMUINT3 const dispatch_extent = brx_voxel_cone_tracing_zero_dispatch_extent();

                graphics_command_buffer->dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
            }

            {
                brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_mask, this->m_voxel_cone_tracing_clipmap_opacity_r32, this->m_voxel_cone_tracing_clipmap_opacity_r16, this->m_voxel_cone_tracing_clipmap_illumination_r32, this->m_voxel_cone_tracing_clipmap_illumination_g32, this->m_voxel_cone_tracing_clipmap_illumination_b32, this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16};

                graphics_command_buffer->storage_resource_store(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
            }

            graphics_command_buffer->end_debug_utils_label();

            inout_voxel_cone_tracing_dirty = false;
        }

        {
            brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_mask, this->m_voxel_cone_tracing_clipmap_opacity_r32, this->m_voxel_cone_tracing_clipmap_opacity_r16, this->m_voxel_cone_tracing_clipmap_illumination_r32, this->m_voxel_cone_tracing_clipmap_illumination_g32, this->m_voxel_cone_tracing_clipmap_illumination_b32, this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16};

            graphics_command_buffer->storage_resource_load_load(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
        }

        {
            graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Clear Pass");

            graphics_command_buffer->bind_compute_pipeline(this->m_voxel_cone_tracing_clear_pipeline);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_post_processing_descriptor_set_none_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<post_processing_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_post_processing_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            {
                DirectX::XMUINT3 const dispatch_extent = brx_voxel_cone_tracing_clear_dispatch_extent();

                graphics_command_buffer->dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
            }

            graphics_command_buffer->end_debug_utils_label();
        }

        {
            brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_mask, this->m_voxel_cone_tracing_clipmap_opacity_r32, this->m_voxel_cone_tracing_clipmap_opacity_r16, this->m_voxel_cone_tracing_clipmap_illumination_r32, this->m_voxel_cone_tracing_clipmap_illumination_g32, this->m_voxel_cone_tracing_clipmap_illumination_b32, this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16};

            graphics_command_buffer->storage_resource_barrier(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
        }

        {
            graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Voxelization Pass");

            graphics_command_buffer->begin_render_pass(this->m_voxel_cone_tracing_voxelization_render_pass, this->m_voxel_cone_tracing_voxelization_frame_buffer, static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), 0U, NULL, NULL, NULL);

            graphics_command_buffer->set_view_port(static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE));

            graphics_command_buffer->set_scissor(0, 0, static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE), static_cast<uint32_t>(BRX_VCT_CLIPMAP_MAP_SIZE));

            graphics_command_buffer->bind_graphics_pipeline(this->m_voxel_cone_tracing_voxelization_pipeline);

            for (auto const &world_surface_group_instance : this->m_world_surface_group_instances)
            {
                brx_anari_surface_group const *const wrapped_surface_group = world_surface_group_instance.first;

                brx_anari_pal_surface_group const *const surface_group = static_cast<brx_anari_pal_surface_group const *>(wrapped_surface_group);

                for (brx_anari_surface_group_instance const *const wrapped_surface_group_instance : world_surface_group_instance.second)
                {
                    brx_anari_pal_surface_group_instance const *const surface_group_instance = static_cast<brx_anari_pal_surface_group_instance const *>(wrapped_surface_group_instance);

                    assert(surface_group_instance->get_surface_group() == surface_group);

                    uint32_t const surface_count = surface_group->get_surface_count();

                    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
                    {
                        brx_anari_pal_surface const *const surface = surface_group->get_surfaces() + surface_index;

                        brx_anari_pal_surface_instance const *const surface_instance = surface_group_instance->get_surfaces() + surface_index;

                        assert(surface->get_deforming() == surface_instance->get_deforming());

                        {
                            brx_pal_descriptor_set const *const descriptor_sets[] = {
                                this->m_forward_shading_descriptor_set_none_update,
                                surface_group_instance->get_forward_shading_per_surface_group_update_descriptor_set(),
                                surface->get_deforming() ? surface_instance->get_forward_shading_per_surface_update_descriptor_set() : surface->get_forward_shading_per_surface_update_descriptor_set()};

                            uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<forward_shading_none_update_set_uniform_buffer_binding>(frame_throttling_index), this->helper_compute_uniform_buffer_dynamic_offset<forward_shading_per_surface_group_update_set_uniform_buffer_binding>(frame_throttling_index)};

                            graphics_command_buffer->bind_graphics_descriptor_sets(this->m_forward_shading_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
                        }

                        graphics_command_buffer->draw(surface->get_index_count(), static_cast<uint32_t>(BRX_VCT_CLIPMAP_STACK_LEVEL_COUNT), 0U, 0U);
                    }
                }
            }

            graphics_command_buffer->end_render_pass();

            graphics_command_buffer->end_debug_utils_label();
        }

        {
            brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_mask, this->m_voxel_cone_tracing_clipmap_opacity_r32, this->m_voxel_cone_tracing_clipmap_illumination_r32, this->m_voxel_cone_tracing_clipmap_illumination_g32, this->m_voxel_cone_tracing_clipmap_illumination_b32};

            graphics_command_buffer->storage_resource_store(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
        }

        {
            graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Pack Pass");

            graphics_command_buffer->bind_compute_pipeline(this->m_voxel_cone_tracing_pack_pipeline);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_post_processing_descriptor_set_none_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<post_processing_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_post_processing_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            {
                DirectX::XMUINT3 const dispatch_extent = brx_voxel_cone_tracing_pack_dispatch_extent();

                graphics_command_buffer->dispatch(dispatch_extent.x, dispatch_extent.y, dispatch_extent.z);
            }

            graphics_command_buffer->end_debug_utils_label();
        }

        {
            brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_clipmap_opacity_r16, this->m_voxel_cone_tracing_clipmap_illumination_r16g16b16};

            graphics_command_buffer->storage_resource_store(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
        }

        {

            graphics_command_buffer->begin_debug_utils_label("Voxel Cone Tracing Cone Tracing Pass");

            {
                brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion};

                graphics_command_buffer->storage_resource_load_dont_care(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
            }

            {
                brx_pal_compute_pipeline *voxel_cone_tracing_cone_tracing_pipeline;
                switch (renderer_gi_quality)
                {
                case BRX_ANARI_RENDERER_GI_QUALITY_LOW:
                {
                    voxel_cone_tracing_cone_tracing_pipeline = this->m_voxel_cone_tracing_cone_tracing_low_pipeline;
                }
                break;
                case BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM:
                {
                    voxel_cone_tracing_cone_tracing_pipeline = this->m_voxel_cone_tracing_cone_tracing_medium_pipeline;
                }
                break;
                default:
                {
                    assert(BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality);
                    voxel_cone_tracing_cone_tracing_pipeline = this->m_voxel_cone_tracing_cone_tracing_high_pipeline;
                }
                }
                graphics_command_buffer->bind_compute_pipeline(voxel_cone_tracing_cone_tracing_pipeline);
            }

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_post_processing_descriptor_set_none_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<post_processing_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_post_processing_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            {
                uint32_t const width = std::max(1U, (this->m_intermediate_width + 1U) / 2U);
                uint32_t const height = std::max(1U, (this->m_intermediate_height + 1U) / 2U);

                graphics_command_buffer->dispatch(width, height, 1U);
            }

            {
                brx_pal_storage_image const *const storage_images[] = {this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion};

                graphics_command_buffer->storage_resource_store(0U, NULL, sizeof(storage_images) / sizeof(storage_images[0]), storage_images);
            }

            graphics_command_buffer->end_debug_utils_label();
        }

        graphics_command_buffer->end_debug_utils_label();
    }
    else
    {
        assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == renderer_gi_quality);
    }
}
