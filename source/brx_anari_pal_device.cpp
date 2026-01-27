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
#include <cassert>
#include <cstring>
#include <algorithm>
#include <tuple>
#include <new>
#include "../../Brioche-Platform-Abstraction-Layer/include/brx_pal_device.h"
#include "../../Brioche-ImGui/backends/imgui_impl_brx.h"
#include "../shaders/none_update_resource_binding.bsli"
#include "../shaders/deforming_surface_resource_binding.bsli"
#include "../shaders/surface_resource_binding.bsli"
#include "../../Brioche-Shader-Language/include/brx_reversed_z.h"
#include "../../DLB/DLB.h"

extern "C" brx_anari_device *brx_anari_new_device(void *wsi_connection)
{
    void *new_unwrapped_device_base = mcrt_malloc(sizeof(brx_anari_pal_device), alignof(brx_anari_pal_device));
    assert(NULL != new_unwrapped_device_base);

    brx_anari_pal_device *new_unwrapped_device = new (new_unwrapped_device_base) brx_anari_pal_device{};
    new_unwrapped_device->init(wsi_connection);
    return new_unwrapped_device;
}

extern "C" void brx_anari_release_device(brx_anari_device *wrapped_device)
{
    assert(NULL != wrapped_device);
    brx_anari_pal_device *delete_unwrapped_device = static_cast<brx_anari_pal_device *>(wrapped_device);

    delete_unwrapped_device->uninit();

    delete_unwrapped_device->~brx_anari_pal_device();
    mcrt_free(delete_unwrapped_device);
}

brx_anari_pal_device::brx_anari_pal_device()
    : m_device(NULL),
      m_uniform_upload_buffer_offset_alignment(0U),
      m_shared_none_update_set_linear_wrap_sampler(NULL),
      m_shared_none_update_set_linear_clamp_sampler(NULL),
      m_none_update_descriptor_set_uniform_buffer(NULL),
      m_none_update_descriptor_set_layout(NULL),
      m_none_update_descriptor_set(NULL),
      m_none_update_pipeline_layout(NULL),
      m_deforming_surface_group_update_descriptor_set_layout(NULL),
      m_deforming_surface_update_descriptor_set_layout(NULL),
      m_deforming_surface_update_pipeline_layout(NULL),
      m_surface_group_update_descriptor_set_layout(NULL),
      m_surface_update_descriptor_set_layout(NULL),
      m_surface_update_pipeline_layout(NULL),
      m_deforming_pipeline(NULL),
      m_environment_lighting_sh_projection_clear_pipeline(NULL),
      m_environment_lighting_sh_projection_equirectangular_map_pipeline(NULL),
      m_environment_lighting_sh_projection_octahedral_map_pipeline(NULL),
      m_direct_radiance_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16B16A16_SFLOAT),
      m_ambient_radiance_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16B16A16_SFLOAT),
      m_gbuffer_normal_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16_SNORM),
      m_gbuffer_base_color_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_SRGB),
      m_gbuffer_roughness_metallic_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8_UNORM),
      m_scene_depth_image_format(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED),
      m_forward_shading_render_pass(NULL),
      m_environment_lighting_skybox_equirectangular_map_pipeline(NULL),
      m_environment_lighting_skybox_octahedral_map_pipeline(NULL),
      m_forward_shading_pipeline(NULL),
      m_display_color_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32),
      m_post_processing_render_pass(NULL),
      m_post_processing_pipeline(NULL),
      m_intermediate_width(0U),
      m_intermediate_height(0U),
      m_direct_radiance_image(NULL),
      m_ambient_radiance_image(NULL),
      m_gbuffer_normal_image(NULL),
      m_gbuffer_base_color_image(NULL),
      m_gbuffer_roughness_metallic_image(NULL),
      m_scene_depth_image(NULL),
      m_forward_shading_frame_buffer(NULL),
      m_display_color_image(NULL),
      m_post_processing_frame_buffer(NULL),
      m_full_screen_transfer_descriptor_set_layout_none_update(NULL),
      m_full_screen_transfer_descriptor_set_none_update(NULL),
      m_full_screen_transfer_pipeline_layout(NULL),
      m_swap_chain_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_UNDEFINED),
      m_swap_chain_render_pass(NULL),
      m_full_screen_transfer_pipeline(NULL),
      m_surface(NULL),
      m_intermediate_width_scale(0.0F),
      m_intermediate_height_scale(0.0F),
      m_swap_chain(NULL),
      m_swap_chain_image_width(0U),
      m_swap_chain_image_height(0U),
      m_swap_chain_frame_buffers{},
      m_graphics_queue(NULL),
      m_fences{},
      m_pending_destroy_descriptor_sets{},
      m_pending_destroy_uniform_upload_buffers{},
      m_pending_destroy_storage_intermediate_buffers{},
      m_pending_destroy_storage_asset_buffers{},
      m_pending_destroy_sampled_asset_images{},
      m_graphics_command_buffers{},
#ifndef NDEBUG
      m_frame_throttling_index_lock(false),
#endif
      m_frame_throttling_index(BRX_ANARI_UINT32_INDEX_INVALID),
      m_lut_specular_hdr_fresnel_factor_asset_image(NULL),
      m_lut_specular_ltc_matrix_asset_image(NULL),
      m_lut_specular_transfer_function_sh_coefficient_asset_image(NULL),
      m_place_holder_asset_buffer(NULL),
      m_place_holder_asset_image(NULL),
      m_place_holder_storage_image(NULL),
      m_world_surface_group_instances{},
      m_quad_lights{},
      m_area_lighting_emissive_pipeline(NULL),
      m_quad_lights_enable_debug_renderer(true),
      m_hdri_light_radiance(NULL),
#ifndef NDEBUG
      m_hdri_light_layout_lock(false),
#endif
      m_hdri_light_layout(BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED),
      m_hdri_light_direction{1.0F, 0.0F, 0.0F},
      m_hdri_light_up{0.0F, 0.0F, 1.0F},
#ifndef NDEBUG
      m_hdri_light_dirty_lock(false),
#endif
      m_hdri_light_dirty(true),
      m_hdri_light_environment_map_sh_coefficients(NULL),
#ifndef NDEBUG
      m_voxel_cone_tracing_dirty_lock(false),
#endif
      m_voxel_cone_tracing_dirty(true),
      m_voxel_cone_tracing_clipmap_mask(NULL),
      m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16(NULL),
      m_voxel_cone_tracing_clipmap_illumination_opacity_b16a16(NULL),
      m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16b16a16(NULL),
      m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion(NULL),
      m_voxel_cone_tracing_zero_pipeline(NULL),
      m_voxel_cone_tracing_clear_pipeline(NULL),
      m_voxel_cone_tracing_voxelization_render_pass(NULL),
      m_voxel_cone_tracing_voxelization_pipeline(NULL),
      m_voxel_cone_tracing_cone_tracing_low_pipeline(NULL),
      m_voxel_cone_tracing_cone_tracing_medium_pipeline(NULL),
      m_voxel_cone_tracing_cone_tracing_high_pipeline(NULL),
      m_voxel_cone_tracing_pack_pipeline(NULL),
      m_voxel_cone_tracing_voxelization_frame_buffer(NULL),
#ifndef NDEBUG
      m_renderer_gi_quality_lock(false),
#endif
      m_renderer_gi_quality(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE)
{
}

brx_anari_pal_device::~brx_anari_pal_device()
{
    assert(NULL == this->m_device);
    assert(0U == this->m_uniform_upload_buffer_offset_alignment);
    assert(NULL == this->m_shared_none_update_set_linear_wrap_sampler);
    assert(NULL == this->m_shared_none_update_set_linear_clamp_sampler);
    assert(NULL == this->m_deforming_surface_group_update_descriptor_set_layout);
    assert(NULL == this->m_deforming_surface_update_descriptor_set_layout);
    assert(NULL == this->m_deforming_surface_update_pipeline_layout);
    assert(NULL == this->m_surface_group_update_descriptor_set_layout);
    assert(NULL == this->m_surface_update_descriptor_set_layout);
    assert(NULL == this->m_surface_update_pipeline_layout);
    assert(NULL == this->m_none_update_descriptor_set_uniform_buffer);
    assert(NULL == this->m_none_update_descriptor_set_layout);
    assert(NULL == this->m_none_update_descriptor_set);
    assert(NULL == this->m_none_update_pipeline_layout);
    assert(NULL == this->m_deforming_pipeline);
    assert(NULL == this->m_environment_lighting_sh_projection_clear_pipeline);
    assert(NULL == this->m_environment_lighting_sh_projection_equirectangular_map_pipeline);
    assert(NULL == this->m_environment_lighting_sh_projection_octahedral_map_pipeline);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16B16A16_SFLOAT == this->m_direct_radiance_image_format);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16B16A16_SFLOAT == this->m_ambient_radiance_image_format);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16_SNORM == this->m_gbuffer_normal_image_format);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_SRGB == this->m_gbuffer_base_color_image_format);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8_UNORM == this->m_gbuffer_roughness_metallic_image_format);
    assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_scene_depth_image_format);
    assert(NULL == this->m_forward_shading_render_pass);
    assert(NULL == this->m_forward_shading_pipeline);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32 == this->m_display_color_image_format);
    assert(NULL == this->m_post_processing_render_pass);
    assert(NULL == this->m_post_processing_pipeline);
    assert(0U == this->m_intermediate_width);
    assert(0U == this->m_intermediate_height);
    assert(NULL == this->m_direct_radiance_image);
    assert(NULL == this->m_ambient_radiance_image);
    assert(NULL == this->m_gbuffer_normal_image);
    assert(NULL == this->m_gbuffer_base_color_image);
    assert(NULL == this->m_gbuffer_roughness_metallic_image);
    assert(NULL == this->m_scene_depth_image);
    assert(NULL == this->m_forward_shading_frame_buffer);
    assert(NULL == this->m_display_color_image);
    assert(NULL == this->m_post_processing_frame_buffer);
    assert(NULL == this->m_full_screen_transfer_descriptor_set_layout_none_update);
    assert(NULL == this->m_full_screen_transfer_descriptor_set_none_update);
    assert(NULL == this->m_full_screen_transfer_pipeline_layout);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_UNDEFINED == this->m_swap_chain_image_format);
    assert(NULL == this->m_swap_chain_render_pass);
    assert(NULL == this->m_full_screen_transfer_pipeline);
    assert(NULL == this->m_surface);
    assert(0.0F == this->m_intermediate_width_scale);
    assert(0.0F == this->m_intermediate_height_scale);
    assert(NULL == this->m_swap_chain);
    assert(0U == this->m_swap_chain_image_width);
    assert(0U == this->m_swap_chain_image_height);
    assert(this->m_swap_chain_frame_buffers.empty());
    assert(NULL == this->m_graphics_queue);
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(NULL == this->m_fences[frame_throttling_index]);
        assert(this->m_pending_destroy_descriptor_sets[frame_throttling_index].empty());
        assert(this->m_pending_destroy_uniform_upload_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_storage_intermediate_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_storage_asset_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_sampled_asset_images[frame_throttling_index].empty());
        assert(NULL == this->m_graphics_command_buffers[frame_throttling_index]);
    }
    assert(BRX_ANARI_UINT32_INDEX_INVALID == this->m_frame_throttling_index);
    assert(NULL == this->m_lut_specular_hdr_fresnel_factor_asset_image);
    assert(NULL == this->m_lut_specular_ltc_matrix_asset_image);
    assert(NULL == this->m_lut_specular_transfer_function_sh_coefficient_asset_image);
    assert(NULL == this->m_place_holder_asset_buffer);
    assert(NULL == this->m_place_holder_asset_image);
    assert(this->m_world_surface_group_instances.empty());
    // assert(this->m_quad_lights.empty());
    assert(NULL == this->m_area_lighting_emissive_pipeline);
    assert(NULL == this->m_hdri_light_radiance);
    assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED == this->m_hdri_light_layout);
    assert(NULL == this->m_hdri_light_environment_map_sh_coefficients);
    assert(NULL == this->m_voxel_cone_tracing_clipmap_mask);
    assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16);
    assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_opacity_b16a16);
    assert(NULL == this->m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16b16a16);
    assert(NULL == this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion);
    assert(NULL == this->m_voxel_cone_tracing_zero_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_clear_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_voxelization_render_pass);
    assert(NULL == this->m_voxel_cone_tracing_voxelization_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_cone_tracing_low_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_cone_tracing_medium_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_cone_tracing_high_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_pack_pipeline);
    assert(NULL == this->m_voxel_cone_tracing_voxelization_frame_buffer);
}

void brx_anari_pal_device::init(void *wsi_connection)
{
    assert(NULL == this->m_device);
    this->m_device = brx_pal_create_device(wsi_connection, false);

    assert(0U == this->m_uniform_upload_buffer_offset_alignment);
    this->m_uniform_upload_buffer_offset_alignment = this->m_device->get_uniform_upload_buffer_offset_alignment();

    assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_scene_depth_image_format);
    this->m_scene_depth_image_format = this->m_device->get_depth_attachment_image_format();

    this->hdri_light_create_none_update_binding_resource();

    this->init_lut_resource();

    // Scene Renderer
    {
        // Sampler
        {
            {
                assert(NULL == this->m_shared_none_update_set_linear_wrap_sampler);

                this->m_shared_none_update_set_linear_wrap_sampler = this->m_device->create_sampler(BRX_PAL_SAMPLER_FILTER_LINEAR, BRX_PAL_SAMPLER_ADDRESS_MODE_WRAP);
            }

            {
                assert(NULL == this->m_shared_none_update_set_linear_clamp_sampler);

                this->m_shared_none_update_set_linear_clamp_sampler = this->m_device->create_sampler(BRX_PAL_SAMPLER_FILTER_LINEAR, BRX_PAL_SAMPLER_ADDRESS_MODE_CLAMP);
            }
        }

        // Descriptor/Pipeline Layout and None Update Descriptor
        {
            // None Update Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_none_update_descriptor_set_layout);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const none_update_pipeline_none_update_descriptor_set_layout_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U},
                    {1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U},
                    {2U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U},
                    {3U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U},
                    {4U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U},
                    {5U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U},
                    {6U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U},
                    {7U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 1U},
                    {8U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 1U},
                    {9U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 1U},
                    {10U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {11U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {12U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {13U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {14U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {15U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {16U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {17U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {18U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {19U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {20U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {21U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {22U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {23U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {24U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
                this->m_none_update_descriptor_set_layout = this->m_device->create_descriptor_set_layout(sizeof(none_update_pipeline_none_update_descriptor_set_layout_bindings) / sizeof(none_update_pipeline_none_update_descriptor_set_layout_bindings[0]), none_update_pipeline_none_update_descriptor_set_layout_bindings);

                assert(NULL == this->m_none_update_pipeline_layout);
                brx_pal_descriptor_set_layout *const none_update_pipeline_descriptor_set_layouts[] = {
                    this->m_none_update_descriptor_set_layout};
                this->m_none_update_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(none_update_pipeline_descriptor_set_layouts) / sizeof(none_update_pipeline_descriptor_set_layouts[0]), none_update_pipeline_descriptor_set_layouts);
            }

            // None Update Uniform Buffer
            {
                assert(NULL == this->m_none_update_descriptor_set_uniform_buffer);

                this->m_none_update_descriptor_set_uniform_buffer = this->m_device->create_uniform_upload_buffer(this->helper_compute_uniform_buffer_size<none_update_set_uniform_buffer_binding>());
            }

            // None Update Descriptor
            {
                assert(NULL == this->m_none_update_descriptor_set);
                this->m_none_update_descriptor_set = this->m_device->create_descriptor_set(this->m_none_update_descriptor_set_layout, 0U);
            }

            // Write None Update Descriptor
            {
                {
                    assert(NULL != this->m_hdri_light_environment_map_sh_coefficients);
                    brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_storage_buffer()};
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, sizeof(buffers) / sizeof(buffers[0]), NULL, NULL, NULL, buffers, NULL, NULL, NULL, NULL);
                }

                {
                    constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(none_update_set_uniform_buffer_binding);
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 6U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_none_update_descriptor_set_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
                }

                {
                    assert(NULL != this->m_hdri_light_environment_map_sh_coefficients);
                    brx_pal_read_only_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_read_only_storage_buffer()};
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 7U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(buffers) / sizeof(buffers[0]), NULL, NULL, buffers, NULL, NULL, NULL, NULL, NULL);
                }

                {
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 8U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_shared_none_update_set_linear_wrap_sampler, NULL);
                }

                {
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 9U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_shared_none_update_set_linear_clamp_sampler, NULL);
                }

                {
                    brx_pal_sampled_image const *const sampled_images[] = {this->m_lut_specular_hdr_fresnel_factor_asset_image->get_sampled_image()};
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 10U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                }

                {
                    brx_pal_sampled_image const *const sampled_images[] = {this->m_lut_specular_ltc_matrix_asset_image->get_sampled_image()};
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 11U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                }

                {
                    brx_pal_sampled_image const *const sampled_images[] = {this->m_lut_specular_transfer_function_sh_coefficient_asset_image->get_sampled_image()};
                    this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 12U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                }
            }

            // Deforming Surface Update Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_deforming_surface_group_update_descriptor_set_layout);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const deforming_surface_update_pipeline_descriptor_set_layout_per_surface_group_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
                this->m_deforming_surface_group_update_descriptor_set_layout = this->m_device->create_descriptor_set_layout(sizeof(deforming_surface_update_pipeline_descriptor_set_layout_per_surface_group_update_bindings) / sizeof(deforming_surface_update_pipeline_descriptor_set_layout_per_surface_group_update_bindings[0]), deforming_surface_update_pipeline_descriptor_set_layout_per_surface_group_update_bindings);

                assert(NULL == this->m_deforming_surface_update_descriptor_set_layout);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const deforming_surface_update_pipeline_descriptor_set_layout_per_surface_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT},
                    {1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, DEFORMING_SURFACE_OUTPUT_BUFFER_COUNT}};
                this->m_deforming_surface_update_descriptor_set_layout = this->m_device->create_descriptor_set_layout(sizeof(deforming_surface_update_pipeline_descriptor_set_layout_per_surface_update_bindings) / sizeof(deforming_surface_update_pipeline_descriptor_set_layout_per_surface_update_bindings[0]), deforming_surface_update_pipeline_descriptor_set_layout_per_surface_update_bindings);

                assert(NULL == this->m_deforming_surface_update_pipeline_layout);
                brx_pal_descriptor_set_layout *const deforming_surface_update_pipeline_descriptor_set_layouts[] = {
                    this->m_deforming_surface_group_update_descriptor_set_layout,
                    this->m_deforming_surface_update_descriptor_set_layout};
                this->m_deforming_surface_update_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(deforming_surface_update_pipeline_descriptor_set_layouts) / sizeof(deforming_surface_update_pipeline_descriptor_set_layouts[0]), deforming_surface_update_pipeline_descriptor_set_layouts);
            }

            // Surface Update Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_surface_group_update_descriptor_set_layout);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const surface_update_descriptor_set_layout_per_surface_group_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
                this->m_surface_group_update_descriptor_set_layout = this->m_device->create_descriptor_set_layout(sizeof(surface_update_descriptor_set_layout_per_surface_group_update_bindings) / sizeof(surface_update_descriptor_set_layout_per_surface_group_update_bindings[0]), surface_update_descriptor_set_layout_per_surface_group_update_bindings);

                assert(NULL == this->m_surface_update_descriptor_set_layout);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const surface_update_descriptor_set_layout_per_surface_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, FORWARD_SHADING_SURFACE_BUFFER_COUNT},
                    {4U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, FORWARD_SHADING_SURFACE_TEXTURE_COUNT}};
                this->m_surface_update_descriptor_set_layout = this->m_device->create_descriptor_set_layout(sizeof(surface_update_descriptor_set_layout_per_surface_update_bindings) / sizeof(surface_update_descriptor_set_layout_per_surface_update_bindings[0]), surface_update_descriptor_set_layout_per_surface_update_bindings);

                assert(NULL == this->m_surface_update_pipeline_layout);
                brx_pal_descriptor_set_layout *const surface_update_pipeline_descriptor_set_layouts[] = {
                    this->m_none_update_descriptor_set_layout,
                    this->m_surface_group_update_descriptor_set_layout,
                    this->m_surface_update_descriptor_set_layout};
                this->m_surface_update_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(surface_update_pipeline_descriptor_set_layouts) / sizeof(surface_update_pipeline_descriptor_set_layouts[0]), surface_update_pipeline_descriptor_set_layouts);
            }
        }

        // Render Pass and Pipeline
        {
            // Deforming Pipeline
            {
                assert(NULL == this->m_deforming_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_surface_update_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
#elif defined(__MACH__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_surface_update_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_surface_update_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_surface_update_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
#else
#error Unknown Compiler
#endif
            }

            // Forward Shading Render Pass
            {
                assert(NULL == this->m_forward_shading_render_pass);

                BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const color_attachment[] = {
                    {this->m_direct_radiance_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE},
                    {this->m_ambient_radiance_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE},
                    {this->m_gbuffer_normal_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE},
                    {this->m_gbuffer_base_color_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE},
                    {this->m_gbuffer_roughness_metallic_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE}};

                BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const depth_stencil_attachment = {
                    this->m_scene_depth_image_format,
                    BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_CLEAR,
                    BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE};

                this->m_forward_shading_render_pass = this->m_device->create_render_pass(sizeof(color_attachment) / sizeof(color_attachment[0]), color_attachment, &depth_stencil_attachment);
            }

            // Forward Shading Pipeline
            {
                // usually the vertices within the same model is organized from back to front
                // we can simply use the over operation to render the result correctly (just like how we render the imgui)

                assert(NULL == this->m_forward_shading_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/forward_shading_vertex.inl"
#include "../shaders/spirv/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_surface_update_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND);
                }
#elif defined(__MACH__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/forward_shading_vertex.inl"
#include "../shaders/spirv/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_surface_update_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND);
                }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/forward_shading_vertex.inl"
#include "../shaders/dxil/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_surface_update_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/forward_shading_vertex.inl"
#include "../shaders/spirv/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_surface_update_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
#else
#error Unknown Compiler
#endif
            }

            this->quad_light_create_pipeline();

            this->hdri_light_create_pipeline();

            // Post Process Pass
            {
                assert(NULL == this->m_post_processing_render_pass);

                BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const color_attachment = {
                    this->m_display_color_image_format,
                    BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                    BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE};

                this->m_post_processing_render_pass = this->m_device->create_render_pass(1U, &color_attachment, NULL);
            }

            // Post Process Pipeline
            {
                assert(NULL == this->m_post_processing_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/post_processing_vertex.inl"
#include "../shaders/spirv/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_none_update_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
                }
#elif defined(__MACH__)
                assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
                {
#include "../shaders/spirv/post_processing_vertex.inl"
#include "../shaders/spirv/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_none_update_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
                }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/post_processing_vertex.inl"
#include "../shaders/dxil/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_none_update_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/post_processing_vertex.inl"
#include "../shaders/spirv/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_none_update_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
#else
#error Unknown Compiler
#endif
            }

            this->voxel_cone_tracing_create_pipeline();
        }
    }

    // UI Renderer
    ImGui_ImplBrx_Init(this->m_device, INTERNAL_FRAME_THROTTLING_COUNT);

    // Facade Renderer
    {
        // Descriptor/Pipeline Layout and None Update Descriptor
        {
            // Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_full_screen_transfer_descriptor_set_layout_none_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const full_screen_transfer_none_update_descriptor_set_layout_bindings[] = {{0U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 1U}, {1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
                this->m_full_screen_transfer_descriptor_set_layout_none_update = this->m_device->create_descriptor_set_layout(sizeof(full_screen_transfer_none_update_descriptor_set_layout_bindings) / sizeof(full_screen_transfer_none_update_descriptor_set_layout_bindings[0]), full_screen_transfer_none_update_descriptor_set_layout_bindings);

                assert(NULL == this->m_full_screen_transfer_pipeline_layout);
                brx_pal_descriptor_set_layout *const full_screen_transfer_descriptor_set_layouts[] = {this->m_full_screen_transfer_descriptor_set_layout_none_update};
                this->m_full_screen_transfer_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(full_screen_transfer_descriptor_set_layouts) / sizeof(full_screen_transfer_descriptor_set_layouts[0]), full_screen_transfer_descriptor_set_layouts);
            }

            // None Update Descriptor
            {
                assert(NULL == this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_full_screen_transfer_descriptor_set_none_update = this->m_device->create_descriptor_set(this->m_full_screen_transfer_descriptor_set_layout_none_update, 0U);
            }

            // Write None Update Descriptor
            {
                this->m_device->write_descriptor_set(this->m_full_screen_transfer_descriptor_set_none_update, 0U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_shared_none_update_set_linear_clamp_sampler, NULL);
            }
        }

        // Render Pass and Pipeline
        {
            assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_UNDEFINED == this->m_swap_chain_image_format);
            this->m_swap_chain_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM;

            this->create_swap_chain_compatible_render_pass_and_pipeline();
        }
    }

    assert(NULL == this->m_graphics_queue);
    this->m_graphics_queue = this->m_device->create_graphics_queue();

    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(NULL == this->m_fences[frame_throttling_index]);
        this->m_fences[frame_throttling_index] = this->m_device->create_fence(true);

        assert(this->m_pending_destroy_descriptor_sets[frame_throttling_index].empty());
        assert(this->m_pending_destroy_uniform_upload_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_storage_intermediate_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_storage_asset_buffers[frame_throttling_index].empty());
        assert(this->m_pending_destroy_sampled_asset_images[frame_throttling_index].empty());

        assert(NULL == this->m_graphics_command_buffers[frame_throttling_index]);
        this->m_graphics_command_buffers[frame_throttling_index] = this->m_device->create_graphics_command_buffer();
    }

    // Frame Throttling
    assert(BRX_ANARI_UINT32_INDEX_INVALID == this->m_frame_throttling_index);
    this->m_frame_throttling_index = 0U;

    this->init_place_holder_resource();

    assert(this->m_world_surface_group_instances.empty());

    assert(NULL == this->m_hdri_light_radiance);

    assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED == this->m_hdri_light_layout);

    this->hdri_light_write_place_holder_none_update_descriptor();

    assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == this->m_renderer_gi_quality);

    this->voxel_cone_tracing_write_quality_dependent_place_holder_none_update_descriptor();
}

void brx_anari_pal_device::uninit()
{
    this->m_hdri_light_layout = BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED;

    if (NULL != this->m_hdri_light_radiance)
    {
        this->release_image(this->m_hdri_light_radiance);
        this->m_hdri_light_radiance = NULL;
    }

    if (BRX_ANARI_RENDERER_GI_QUALITY_DISABLE != this->m_renderer_gi_quality)
    {
        this->voxel_cone_tracing_destroy_quality_dependent_none_update_binding_resource();
    }

    this->hdri_light_destroy_none_update_binding_resource();

    assert(this->m_world_surface_group_instances.empty());

    assert(NULL != this->m_lut_specular_hdr_fresnel_factor_asset_image);
    this->helper_destroy_asset_image(this->m_lut_specular_hdr_fresnel_factor_asset_image);
    this->m_lut_specular_hdr_fresnel_factor_asset_image = NULL;

    assert(NULL != this->m_lut_specular_ltc_matrix_asset_image);
    this->helper_destroy_asset_image(this->m_lut_specular_ltc_matrix_asset_image);
    this->m_lut_specular_ltc_matrix_asset_image = NULL;

    assert(NULL != this->m_lut_specular_transfer_function_sh_coefficient_asset_image);
    this->helper_destroy_asset_image(this->m_lut_specular_transfer_function_sh_coefficient_asset_image);
    this->m_lut_specular_transfer_function_sh_coefficient_asset_image = NULL;

    assert(NULL != this->m_place_holder_asset_buffer);
    this->helper_destroy_asset_buffer(this->m_place_holder_asset_buffer);
    this->m_place_holder_asset_buffer = NULL;

    assert(NULL != this->m_place_holder_asset_image);
    this->helper_destroy_asset_image(this->m_place_holder_asset_image);
    this->m_place_holder_asset_image = NULL;

    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

    assert(NULL != this->m_place_holder_storage_image);
    this->m_device->destroy_storage_image(this->m_place_holder_storage_image);
    this->m_place_holder_storage_image = NULL;

    // Frame Throttling
    assert(BRX_ANARI_UINT32_INDEX_INVALID != this->m_frame_throttling_index);
    this->m_frame_throttling_index = BRX_ANARI_UINT32_INDEX_INVALID;

    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(NULL != this->m_fences[frame_throttling_index]);
        this->m_device->destroy_fence(this->m_fences[frame_throttling_index]);
        this->m_fences[frame_throttling_index] = NULL;

        for (brx_pal_descriptor_set *const pending_destroy_descriptor_set : this->m_pending_destroy_descriptor_sets[frame_throttling_index])
        {
            assert(NULL != pending_destroy_descriptor_set);
            this->m_device->destroy_descriptor_set(pending_destroy_descriptor_set);
        }
        this->m_pending_destroy_descriptor_sets[frame_throttling_index].clear();

        for (brx_pal_uniform_upload_buffer *const pending_destroy_uniform_upload_buffer : this->m_pending_destroy_uniform_upload_buffers[frame_throttling_index])
        {
            assert(NULL != pending_destroy_uniform_upload_buffer);
            this->m_device->destroy_uniform_upload_buffer(pending_destroy_uniform_upload_buffer);
        }
        this->m_pending_destroy_uniform_upload_buffers[frame_throttling_index].clear();

        for (brx_pal_storage_intermediate_buffer *const pending_destroy_storage_intermediate_buffer : this->m_pending_destroy_storage_intermediate_buffers[frame_throttling_index])
        {
            assert(NULL != pending_destroy_storage_intermediate_buffer);
            this->m_device->destroy_storage_intermediate_buffer(pending_destroy_storage_intermediate_buffer);
        }
        this->m_pending_destroy_storage_intermediate_buffers[frame_throttling_index].clear();

        for (brx_pal_storage_asset_buffer *const pending_destroy_storage_asset_buffer : this->m_pending_destroy_storage_asset_buffers[frame_throttling_index])
        {
            assert(NULL != pending_destroy_storage_asset_buffer);
            this->m_device->destroy_storage_asset_buffer(pending_destroy_storage_asset_buffer);
        }
        this->m_pending_destroy_storage_asset_buffers[frame_throttling_index].clear();

        for (brx_pal_sampled_asset_image *const pending_destroy_sampled_asset_image : this->m_pending_destroy_sampled_asset_images[frame_throttling_index])
        {
            assert(NULL != pending_destroy_sampled_asset_image);
            this->m_device->destroy_sampled_asset_image(pending_destroy_sampled_asset_image);
        }
        this->m_pending_destroy_sampled_asset_images[frame_throttling_index].clear();

        assert(NULL != this->m_graphics_command_buffers[frame_throttling_index]);
        this->m_device->destroy_graphics_command_buffer(this->m_graphics_command_buffers[frame_throttling_index]);
        this->m_graphics_command_buffers[frame_throttling_index] = NULL;
    }

    assert(NULL != this->m_graphics_queue);
    this->m_device->destroy_graphics_queue(this->m_graphics_queue);
    this->m_graphics_queue = NULL;

    // Facade Renderer
    {
        // Render Pass and Pipeline
        {
            this->destroy_swap_chain_compatible_render_pass_and_pipeline();

            assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_UNDEFINED != this->m_swap_chain_image_format);
            this->m_swap_chain_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_UNDEFINED;
        }

        // Descriptor/Pipeline Layout and None Update Descriptor
        {
            // None Update Descriptor
            {
                assert(NULL != this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_device->destroy_descriptor_set(this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_full_screen_transfer_descriptor_set_none_update = NULL;
            }

            // Deforming Surface Descriptor/Pipeline Layout
            {
                assert(NULL != this->m_full_screen_transfer_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_full_screen_transfer_pipeline_layout);
                this->m_full_screen_transfer_pipeline_layout = NULL;

                assert(NULL != this->m_full_screen_transfer_descriptor_set_layout_none_update);
                this->m_device->destroy_descriptor_set_layout(this->m_full_screen_transfer_descriptor_set_layout_none_update);
                this->m_full_screen_transfer_descriptor_set_layout_none_update = NULL;
            }
        }
    }

    // UI Renderer
    ImGui_ImplBrx_Shutdown(this->m_device);

    // Scene Renderer
    {
        // Render Pass and Pipeline
        {
            this->voxel_cone_tracing_destroy_pipeline();

            this->quad_light_destroy_pipeline();

            this->hdri_light_destroy_pipeline();

            assert(NULL != this->m_deforming_pipeline);
            this->m_device->destroy_compute_pipeline(this->m_deforming_pipeline);
            this->m_deforming_pipeline = NULL;

            assert(NULL != this->m_forward_shading_pipeline);
            this->m_device->destroy_graphics_pipeline(this->m_forward_shading_pipeline);
            this->m_forward_shading_pipeline = NULL;

            assert(NULL != this->m_forward_shading_render_pass);
            this->m_device->destroy_render_pass(this->m_forward_shading_render_pass);
            this->m_forward_shading_render_pass = NULL;

            assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED != this->m_scene_depth_image_format);
            this->m_scene_depth_image_format = BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED;

            assert(NULL != this->m_post_processing_pipeline);
            this->m_device->destroy_graphics_pipeline(this->m_post_processing_pipeline);
            this->m_post_processing_pipeline = NULL;

            assert(NULL != this->m_post_processing_render_pass);
            this->m_device->destroy_render_pass(this->m_post_processing_render_pass);
            this->m_post_processing_render_pass = NULL;
        }

        // Descriptor/Pipeline Layout and None Update Descriptor
        {
            // Deforming Surface Descriptor/Pipeline Layout
            {
                assert(NULL != this->m_deforming_surface_update_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_deforming_surface_update_pipeline_layout);
                this->m_deforming_surface_update_pipeline_layout = NULL;

                assert(NULL != this->m_deforming_surface_group_update_descriptor_set_layout);
                this->m_device->destroy_descriptor_set_layout(this->m_deforming_surface_group_update_descriptor_set_layout);
                this->m_deforming_surface_group_update_descriptor_set_layout = NULL;

                assert(NULL != this->m_deforming_surface_update_descriptor_set_layout);
                this->m_device->destroy_descriptor_set_layout(this->m_deforming_surface_update_descriptor_set_layout);
                this->m_deforming_surface_update_descriptor_set_layout = NULL;
            }

            // Surface Descriptor/Pipeline Layout
            {
                assert(NULL != this->m_surface_update_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_surface_update_pipeline_layout);
                this->m_surface_update_pipeline_layout = NULL;

                assert(NULL != this->m_surface_group_update_descriptor_set_layout);
                this->m_device->destroy_descriptor_set_layout(this->m_surface_group_update_descriptor_set_layout);
                this->m_surface_group_update_descriptor_set_layout = NULL;

                assert(NULL != this->m_surface_update_descriptor_set_layout);
                this->m_device->destroy_descriptor_set_layout(this->m_surface_update_descriptor_set_layout);
                this->m_surface_update_descriptor_set_layout = NULL;
            }

            // None Update Descriptor/Pipeline Layout
            {
                assert(NULL != this->m_none_update_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_none_update_pipeline_layout);
                this->m_none_update_pipeline_layout = NULL;

                assert(NULL != this->m_none_update_descriptor_set_layout);
                this->m_device->destroy_descriptor_set_layout(this->m_none_update_descriptor_set_layout);
                this->m_none_update_descriptor_set_layout = NULL;
            }

            // None Update Descriptor
            {
                assert(NULL != this->m_none_update_descriptor_set);
                this->m_device->destroy_descriptor_set(this->m_none_update_descriptor_set);
                this->m_none_update_descriptor_set = NULL;
            }

            // None Update Uniform Buffer
            {
                assert(NULL != this->m_none_update_descriptor_set_uniform_buffer);
                this->m_device->destroy_uniform_upload_buffer(this->m_none_update_descriptor_set_uniform_buffer);
                this->m_none_update_descriptor_set_uniform_buffer = NULL;
            }
        }

        // Sampler
        {
            assert(NULL != this->m_shared_none_update_set_linear_wrap_sampler);
            this->m_device->destroy_sampler(this->m_shared_none_update_set_linear_wrap_sampler);
            this->m_shared_none_update_set_linear_wrap_sampler = NULL;

            assert(NULL != this->m_shared_none_update_set_linear_clamp_sampler);
            this->m_device->destroy_sampler(this->m_shared_none_update_set_linear_clamp_sampler);
            this->m_shared_none_update_set_linear_clamp_sampler = NULL;
        }
    }

    assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);
    this->m_uniform_upload_buffer_offset_alignment = 0U;

    assert(NULL != this->m_device);
    brx_pal_destroy_device(this->m_device);
    this->m_device = NULL;
}

void brx_anari_pal_device::camera_set_position(brx_anari_vec3 position)
{
    this->m_camera_position = position;
}

void brx_anari_pal_device::camera_set_direction(brx_anari_vec3 direction)
{
    this->m_camera_direction = direction;
}

void brx_anari_pal_device::camera_set_up(brx_anari_vec3 up)
{
    this->m_camera_up = up;
}

void brx_anari_pal_device::camera_set_fovy(float fovy)
{
    this->m_camera_fovy = fovy;
}

void brx_anari_pal_device::camera_set_near(float near)
{
    this->m_camera_near = near;
}

void brx_anari_pal_device::camera_set_far(float far)
{
    this->m_camera_far = far;
}

brx_anari_vec3 brx_anari_pal_device::camera_get_position() const
{
    return this->m_camera_position;
}

brx_anari_vec3 brx_anari_pal_device::camera_get_direction() const
{
    return this->m_camera_direction;
}

brx_anari_vec3 brx_anari_pal_device::camera_get_up() const
{
    return this->m_camera_up;
}

float brx_anari_pal_device::camera_get_fovy() const
{
    return this->m_camera_fovy;
}

float brx_anari_pal_device::camera_get_near() const
{
    return this->m_camera_near;
}

float brx_anari_pal_device::camera_get_far() const
{
    return this->m_camera_far;
}

void brx_anari_pal_device::frame_attach_window(void *wsi_window, float intermediate_width_scale, float intermediate_height_scale)
{
    assert(NULL == this->m_surface);
    this->m_surface = this->m_device->create_surface(wsi_window);

    assert(0.0F == this->m_intermediate_width_scale);
    this->m_intermediate_width_scale = intermediate_width_scale;

    assert(0.0F == this->m_intermediate_height_scale);
    this->m_intermediate_height_scale = intermediate_height_scale;

#ifndef NDEBUG
    assert(!this->m_renderer_gi_quality_lock);
    this->m_renderer_gi_quality_lock = true;
#endif

    this->attach_swap_chain(this->m_renderer_gi_quality);

#ifndef NDEBUG
    this->m_renderer_gi_quality_lock = false;
#endif
}

void brx_anari_pal_device::frame_resize_window(float intermediate_width_scale, float intermediate_height_scale)
{
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

#ifndef NDEBUG
    assert(!this->m_renderer_gi_quality_lock);
    this->m_renderer_gi_quality_lock = true;
#endif

    this->detach_swap_chain(this->m_renderer_gi_quality);
    this->attach_swap_chain(this->m_renderer_gi_quality);

#ifndef NDEBUG
    this->m_renderer_gi_quality_lock = false;
#endif
}

void brx_anari_pal_device::frame_detach_window()
{
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

#ifndef NDEBUG
    assert(!this->m_renderer_gi_quality_lock);
    this->m_renderer_gi_quality_lock = true;
#endif

    this->detach_swap_chain(this->m_renderer_gi_quality);

#ifndef NDEBUG
    this->m_renderer_gi_quality_lock = false;
#endif

    assert(0.0F != this->m_intermediate_width_scale);
    this->m_intermediate_width_scale = 0.0F;

    assert(0.0F != this->m_intermediate_height_scale);
    this->m_intermediate_height_scale = 0.0F;

    assert(NULL != this->m_surface);
    this->m_device->destroy_surface(this->m_surface);
    this->m_surface = NULL;
}

inline void brx_anari_pal_device::create_swap_chain_compatible_render_pass_and_pipeline()
{
    // Facade Renderer
    {
        // Render Pass
        {
            assert(NULL == this->m_swap_chain_render_pass);
            BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const color_attachments[1] = {
                {this->m_swap_chain_image_format,
                 BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                 BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_PRESENT}};
            this->m_swap_chain_render_pass = this->m_device->create_render_pass(sizeof(color_attachments) / sizeof(color_attachments[0]), color_attachments, NULL);
        }

        // Pipeline
        {
            assert(NULL == this->m_full_screen_transfer_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
            assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
            {
#include "../shaders/spirv/full_screen_transfer_vertex.inl"
#include "../shaders/spirv/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
            }
#elif defined(__MACH__)
            assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
            {
#include "../shaders/spirv/full_screen_transfer_vertex.inl"
#include "../shaders/spirv/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
            }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
            switch (this->m_device->get_backend_name())
            {
            case BRX_PAL_BACKEND_NAME_D3D12:
            {
#include "../shaders/dxil/full_screen_transfer_vertex.inl"
#include "../shaders/dxil/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
            }
            break;
            case BRX_PAL_BACKEND_NAME_VK:
            {
#include "../shaders/spirv/full_screen_transfer_vertex.inl"
#include "../shaders/spirv/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
            }
            break;
            default:
            {
                assert(false);
            }
            }
#else
#error Unknown Compiler
#endif
        }
    }

    // UI Renderer
    ImGui_ImplBrx_Init_Pipeline(this->m_device, this->m_swap_chain_render_pass);
}

inline void brx_anari_pal_device::destroy_swap_chain_compatible_render_pass_and_pipeline()
{
    // UI Renderer
    ImGui_ImplBrx_Shutdown_Pipeline(this->m_device);

    // Facade Renderer
    {
        // Pipeline
        {
            assert(NULL != this->m_full_screen_transfer_pipeline);
            this->m_device->destroy_graphics_pipeline(this->m_full_screen_transfer_pipeline);
            this->m_full_screen_transfer_pipeline = NULL;
        }

        // Render Pass
        {
            assert(NULL != this->m_swap_chain_render_pass);
            this->m_device->destroy_render_pass(this->m_swap_chain_render_pass);
            this->m_swap_chain_render_pass = NULL;
        }
    }
}

inline void brx_anari_pal_device::attach_swap_chain(BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality)
{
    // Facade Renderer
    {
        assert(NULL == this->m_swap_chain);
        this->m_swap_chain = this->m_device->create_swap_chain(this->m_surface);

        assert(0U == this->m_swap_chain_image_width);
        assert(0U == this->m_swap_chain_image_height);
        this->m_swap_chain_image_width = this->m_swap_chain->get_image_width();
        this->m_swap_chain_image_height = this->m_swap_chain->get_image_height();

        if (this->m_swap_chain_image_format != this->m_swap_chain->get_image_format())
        {
            this->m_swap_chain_image_format = this->m_swap_chain->get_image_format();
            this->destroy_swap_chain_compatible_render_pass_and_pipeline();
            this->create_swap_chain_compatible_render_pass_and_pipeline();
        }

        uint32_t const swap_chain_image_count = this->m_swap_chain->get_image_count();
        assert(0U == this->m_swap_chain_frame_buffers.size());
        this->m_swap_chain_frame_buffers.resize(swap_chain_image_count);

        for (uint32_t swap_chain_image_index = 0U; swap_chain_image_index < swap_chain_image_count; ++swap_chain_image_index)
        {
            brx_pal_color_attachment_image const *const swap_chain_color_attachment_image = this->m_swap_chain->get_image(swap_chain_image_index);

            this->m_swap_chain_frame_buffers[swap_chain_image_index] = this->m_device->create_frame_buffer(this->m_swap_chain_render_pass, this->m_swap_chain_image_width, this->m_swap_chain_image_height, 1U, &swap_chain_color_attachment_image, NULL);
        }
    }

    // Scene Renderer
    {
        // Intermediate Image and Frame Buffer
        {
            assert(0U == this->m_intermediate_width);
            this->m_intermediate_width = this->m_swap_chain_image_width * this->m_intermediate_width_scale;

            assert(0U == this->m_intermediate_height);
            this->m_intermediate_height = this->m_swap_chain_image_height * this->m_intermediate_height_scale;

            // Forward Shading
            {
                assert(NULL == this->m_direct_radiance_image);
                this->m_direct_radiance_image = this->m_device->create_color_attachment_image(this->m_direct_radiance_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_ambient_radiance_image);
                this->m_ambient_radiance_image = this->m_device->create_color_attachment_image(this->m_ambient_radiance_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_gbuffer_normal_image);
                this->m_gbuffer_normal_image = this->m_device->create_color_attachment_image(this->m_gbuffer_normal_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_gbuffer_base_color_image);
                this->m_gbuffer_base_color_image = this->m_device->create_color_attachment_image(this->m_gbuffer_base_color_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_gbuffer_roughness_metallic_image);
                this->m_gbuffer_roughness_metallic_image = this->m_device->create_color_attachment_image(this->m_gbuffer_roughness_metallic_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_scene_depth_image);
                this->m_scene_depth_image = this->m_device->create_depth_stencil_attachment_image(this->m_scene_depth_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_forward_shading_frame_buffer);
                brx_pal_color_attachment_image const *const color_attachments[] = {this->m_direct_radiance_image, this->m_ambient_radiance_image, this->m_gbuffer_normal_image, this->m_gbuffer_base_color_image, this->m_gbuffer_roughness_metallic_image};
                this->m_forward_shading_frame_buffer = this->m_device->create_frame_buffer(this->m_forward_shading_render_pass, this->m_intermediate_width, this->m_intermediate_height, sizeof(color_attachments) / sizeof(color_attachments[0]), color_attachments, this->m_scene_depth_image);
            }

            // Post Processing
            {
                assert(NULL == this->m_display_color_image);
                this->m_display_color_image = this->m_device->create_color_attachment_image(this->m_display_color_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_post_processing_frame_buffer);
                this->m_post_processing_frame_buffer = this->m_device->create_frame_buffer(this->m_post_processing_render_pass, this->m_intermediate_width, this->m_intermediate_height, 1U, &this->m_display_color_image, NULL);
            }

            if ((BRX_ANARI_RENDERER_GI_QUALITY_LOW == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality))
            {
                this->voxel_cone_tracing_create_screen_size_dependent_none_update_binding_resource();
            }
            else
            {
                assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == renderer_gi_quality);
            }
        }

        // Write Descriptor
        {
            // Post Process
            {
                // The VkDescriptorSetLayout should still be valid when perform write update on VkDescriptorSet
                assert(NULL != this->m_none_update_descriptor_set_layout);
                {
                    {
                        brx_pal_storage_image const *storage_images[1] = {NULL};
                        if ((BRX_ANARI_RENDERER_GI_QUALITY_LOW == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality))
                        {
                            storage_images[0] = this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion;
                        }
                        else
                        {
                            assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == renderer_gi_quality);
                            storage_images[0] = this->m_place_holder_storage_image;
                        }
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 5U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0U, sizeof(storage_images) / sizeof(storage_images[0]), NULL, NULL, NULL, NULL, NULL, storage_images, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_direct_radiance_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 14U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_ambient_radiance_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 15U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_gbuffer_normal_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 16U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_gbuffer_base_color_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 17U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_gbuffer_roughness_metallic_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 18U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_scene_depth_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 19U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *sampled_images[1] = {NULL};
                        if ((BRX_ANARI_RENDERER_GI_QUALITY_LOW == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality))
                        {
                            sampled_images[0] = this->m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion->get_sampled_image();
                        }
                        else
                        {
                            assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == renderer_gi_quality);
                            sampled_images[0] = this->m_place_holder_asset_image->get_sampled_image();
                        }
                        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 24U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }
                }
            }
        }
    }

    // Facade Renderer
    {
        // Write Descriptor
        {
            // Full Screen Transfer
            {
                // The VkDescriptorSetLayout should still be valid when perform write update on VkDescriptorSet
                assert(NULL != this->m_full_screen_transfer_descriptor_set_layout_none_update);
                {
                    brx_pal_sampled_image const *const sampled_images[] = {this->m_display_color_image->get_sampled_image()};
                    this->m_device->write_descriptor_set(this->m_full_screen_transfer_descriptor_set_none_update, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                }
            }
        }
    }
}

inline void brx_anari_pal_device::detach_swap_chain(BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality)
{
    // Scene Renderer
    {
        // Intermediate Image and Frame Buffer
        {
            if ((BRX_ANARI_RENDERER_GI_QUALITY_LOW == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM == renderer_gi_quality) || (BRX_ANARI_RENDERER_GI_QUALITY_HIGH == renderer_gi_quality))
            {
                this->voxel_cone_tracing_destroy_screen_size_dependent_none_update_binding_resource();
            }
            else
            {
                assert(BRX_ANARI_RENDERER_GI_QUALITY_DISABLE == renderer_gi_quality);
            }

            // Forward Shading
            {
                assert(NULL != this->m_forward_shading_frame_buffer);
                this->m_device->destroy_frame_buffer(this->m_forward_shading_frame_buffer);
                this->m_forward_shading_frame_buffer = NULL;

                assert(NULL != this->m_direct_radiance_image);
                this->m_device->destroy_color_attachment_image(this->m_direct_radiance_image);
                this->m_direct_radiance_image = NULL;

                assert(NULL != this->m_ambient_radiance_image);
                this->m_device->destroy_color_attachment_image(this->m_ambient_radiance_image);
                this->m_ambient_radiance_image = NULL;

                assert(NULL != this->m_gbuffer_normal_image);
                this->m_device->destroy_color_attachment_image(this->m_gbuffer_normal_image);
                this->m_gbuffer_normal_image = NULL;

                assert(NULL != this->m_gbuffer_base_color_image);
                this->m_device->destroy_color_attachment_image(this->m_gbuffer_base_color_image);
                this->m_gbuffer_base_color_image = NULL;

                assert(NULL != this->m_gbuffer_roughness_metallic_image);
                this->m_device->destroy_color_attachment_image(this->m_gbuffer_roughness_metallic_image);
                this->m_gbuffer_roughness_metallic_image = NULL;

                assert(NULL != this->m_scene_depth_image);
                this->m_device->destroy_depth_stencil_attachment_image(this->m_scene_depth_image);
                this->m_scene_depth_image = NULL;
            }

            // Post Processing
            {
                assert(NULL != this->m_post_processing_frame_buffer);
                this->m_device->destroy_frame_buffer(this->m_post_processing_frame_buffer);
                this->m_post_processing_frame_buffer = NULL;

                assert(NULL != this->m_display_color_image);
                this->m_device->destroy_color_attachment_image(this->m_display_color_image);
                this->m_display_color_image = NULL;
            }

            assert(0U != this->m_intermediate_width);
            this->m_intermediate_width = 0U;

            assert(0U != this->m_intermediate_height);
            this->m_intermediate_height = 0U;
        }
    }

    // Facade Renderer
    {
        uint32_t const swap_chain_image_count = static_cast<uint32_t>(this->m_swap_chain_frame_buffers.size());
        assert(this->m_swap_chain->get_image_count() == swap_chain_image_count);
        for (uint32_t swap_chain_image_index = 0U; swap_chain_image_index < swap_chain_image_count; ++swap_chain_image_index)
        {
            this->m_device->destroy_frame_buffer(this->m_swap_chain_frame_buffers[swap_chain_image_index]);
        }
        this->m_swap_chain_frame_buffers.clear();

        assert(this->m_swap_chain->get_image_width() == this->m_swap_chain_image_width);
        this->m_swap_chain_image_width = 0U;

        assert(this->m_swap_chain->get_image_height() == this->m_swap_chain_image_height);
        this->m_swap_chain_image_height = 0U;

        assert(NULL != this->m_swap_chain);
        this->m_device->destroy_swap_chain(this->m_swap_chain);
        this->m_swap_chain = NULL;
    }
}

void brx_anari_pal_device::renderer_render_frame(bool ui_view)
{
    if (NULL == this->m_surface)
    {
        // skip this frame
        return;
    }

    assert(NULL != this->m_swap_chain);

#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;

    assert(!this->m_hdri_light_layout_lock);
    this->m_hdri_light_layout_lock = true;

    assert(!this->m_renderer_gi_quality_lock);
    this->m_renderer_gi_quality_lock = true;

    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;

    assert(!this->m_voxel_cone_tracing_dirty_lock);
    this->m_voxel_cone_tracing_dirty_lock = true;
#endif

    this->m_device->wait_for_fence(this->m_fences[this->m_frame_throttling_index]);

    // Destroy Resources NOT Used By GPU
    {
        for (brx_pal_descriptor_set *const pending_destroy_descriptor_set : this->m_pending_destroy_descriptor_sets[this->m_frame_throttling_index])
        {
            assert(NULL != pending_destroy_descriptor_set);
            this->m_device->destroy_descriptor_set(pending_destroy_descriptor_set);
        }
        this->m_pending_destroy_descriptor_sets[this->m_frame_throttling_index].clear();

        for (brx_pal_uniform_upload_buffer *const pending_destroy_uniform_upload_buffer : this->m_pending_destroy_uniform_upload_buffers[this->m_frame_throttling_index])
        {
            assert(NULL != pending_destroy_uniform_upload_buffer);
            this->m_device->destroy_uniform_upload_buffer(pending_destroy_uniform_upload_buffer);
        }
        this->m_pending_destroy_uniform_upload_buffers[this->m_frame_throttling_index].clear();

        for (brx_pal_storage_intermediate_buffer *const pending_destroy_storage_intermediate_buffer : this->m_pending_destroy_storage_intermediate_buffers[this->m_frame_throttling_index])
        {
            assert(NULL != pending_destroy_storage_intermediate_buffer);
            this->m_device->destroy_storage_intermediate_buffer(pending_destroy_storage_intermediate_buffer);
        }
        this->m_pending_destroy_storage_intermediate_buffers[this->m_frame_throttling_index].clear();

        for (brx_pal_storage_asset_buffer *const pending_destroy_storage_asset_buffer : this->m_pending_destroy_storage_asset_buffers[this->m_frame_throttling_index])
        {
            assert(NULL != pending_destroy_storage_asset_buffer);
            this->m_device->destroy_storage_asset_buffer(pending_destroy_storage_asset_buffer);
        }
        this->m_pending_destroy_storage_asset_buffers[this->m_frame_throttling_index].clear();

        for (brx_pal_sampled_asset_image *const pending_destroy_sampled_asset_image : this->m_pending_destroy_sampled_asset_images[this->m_frame_throttling_index])
        {
            assert(NULL != pending_destroy_sampled_asset_image);
            this->m_device->destroy_sampled_asset_image(pending_destroy_sampled_asset_image);
        }
        this->m_pending_destroy_sampled_asset_images[this->m_frame_throttling_index].clear();
    }

    this->m_device->reset_graphics_command_buffer(this->m_graphics_command_buffers[this->m_frame_throttling_index]);

    this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin();

    // Scene Renderer
    {
        brx_pal_graphics_command_buffer *const command_buffer = this->m_graphics_command_buffers[this->m_frame_throttling_index];
        assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);

        // TODO: Frustum Culling

        // Upload Camera
        DirectX::XMFLOAT4X4 camera_view_transform;
        DirectX::XMFLOAT4X4 camera_projection_transform;
        DirectX::XMFLOAT4X4 camera_inverse_view_transform;
        DirectX::XMFLOAT4X4 camera_inverse_projection_transform;
        {
            {
                DirectX::XMFLOAT3 const camera_position(this->m_camera_position.m_x, this->m_camera_position.m_y, this->m_camera_position.m_z);
                DirectX::XMFLOAT3 const camera_direction(this->m_camera_direction.m_x, this->m_camera_direction.m_y, this->m_camera_direction.m_z);
                DirectX::XMFLOAT3 const camera_up(this->m_camera_up.m_x, this->m_camera_up.m_y, this->m_camera_up.m_z);

                DirectX::XMMATRIX simd_camera_view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&camera_position), DirectX::XMLoadFloat3(&camera_direction), DirectX::XMLoadFloat3(&camera_up));
                DirectX::XMStoreFloat4x4(&camera_view_transform, simd_camera_view_transform);

                DirectX::XMVECTOR unused_determinant;
                DirectX::XMMATRIX simd_inverse_view_transform = DirectX::XMMatrixInverse(&unused_determinant, simd_camera_view_transform);
                DirectX::XMStoreFloat4x4(&camera_inverse_view_transform, simd_inverse_view_transform);
            }

            {
                float const aspect = static_cast<float>(this->m_intermediate_width) / static_cast<float>(this->m_intermediate_height);

                DirectX::XMMATRIX simd_camera_projection_transform = brx_DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(this->m_camera_fovy, aspect, this->m_camera_near, this->m_camera_far);
                DirectX::XMStoreFloat4x4(&camera_projection_transform, simd_camera_projection_transform);

                DirectX::XMVECTOR unused_determinant;
                DirectX::XMMATRIX simd_camera_inverse_projection_transform = DirectX::XMMatrixInverse(&unused_determinant, simd_camera_projection_transform);
                DirectX::XMStoreFloat4x4(&camera_inverse_projection_transform, simd_camera_inverse_projection_transform);
            }
        }

        // Upload None Update Uniform Buffer
        {
            none_update_set_uniform_buffer_binding *const none_update_set_uniform_buffer_destination = this->helper_compute_uniform_buffer_memory_address<none_update_set_uniform_buffer_binding>(this->m_frame_throttling_index, this->m_none_update_descriptor_set_uniform_buffer);
            none_update_set_uniform_buffer_destination->g_view_transform = camera_view_transform;
            none_update_set_uniform_buffer_destination->g_projection_transform = camera_projection_transform;
            none_update_set_uniform_buffer_destination->g_inverse_view_transform = camera_inverse_view_transform;
            none_update_set_uniform_buffer_destination->g_inverse_projection_transform = camera_inverse_projection_transform;

            this->quad_light_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_destination);
            this->hdri_light_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_destination);
            this->voxel_cone_tracing_none_update_set_uniform_buffer(none_update_set_uniform_buffer_destination);
        }

        // HDRI Light SH Projection
        this->hdri_light_render_sh_projection(this->m_frame_throttling_index, command_buffer, this->m_hdri_light_dirty, this->m_hdri_light_layout);

        // Deforming Pass
        {
            mcrt_vector<brx_pal_storage_buffer const *> buffers;
            mcrt_vector<brx_pal_descriptor_set const *> descriptor_sets;
            mcrt_vector<uint32_t> vertex_counts;

            for (auto const &world_surface_group_instance : this->m_world_surface_group_instances)
            {
                brx_anari_surface_group const *const wrapped_surface_group = world_surface_group_instance.first;

                brx_anari_pal_surface_group const *const surface_group = static_cast<brx_anari_pal_surface_group const *>(wrapped_surface_group);

                for (brx_anari_surface_group_instance const *const wrapped_surface_group_instance : world_surface_group_instance.second)
                {
                    brx_anari_pal_surface_group_instance const *const surface_group_instance = static_cast<brx_anari_pal_surface_group_instance const *>(wrapped_surface_group_instance);

                    assert(surface_group_instance->get_surface_group() == surface_group);

                    bool deforming = false;

                    uint32_t const surface_count = surface_group->get_surface_count();

                    for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
                    {
                        brx_anari_pal_surface const *const surface = surface_group->get_surfaces() + surface_index;

                        brx_anari_pal_surface_instance const *const surface_instance = surface_group_instance->get_surfaces() + surface_index;

                        assert(surface->get_deforming() == surface_instance->get_deforming());

                        if (surface->get_deforming())
                        {
                            if (!deforming)
                            {
                                deforming = true;
                            }

                            buffers.push_back(surface_instance->get_vertex_position_buffer()->get_storage_buffer());
                            buffers.push_back(surface_instance->get_vertex_varying_buffer()->get_storage_buffer());

                            descriptor_sets.push_back(surface_group_instance->get_deforming_surface_group_update_descriptor_set());
                            descriptor_sets.push_back(surface_instance->get_deforming_surface_update_descriptor_set());

                            vertex_counts.push_back(surface->get_vertex_count());
                        }
                    }

                    // Upload Deforming Surface Uniform Buffer
                    if (deforming)
                    {
                        deforming_surface_group_update_set_uniform_buffer_binding *const deforming_surface_group_update_set_uniform_buffer_destination = this->helper_compute_uniform_buffer_memory_address<deforming_surface_group_update_set_uniform_buffer_binding>(this->m_frame_throttling_index, surface_group_instance->get_deforming_surface_group_update_set_uniform_buffer());
                        static_assert(BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT == MORPH_TARGET_WEIGHT_COUNT, "");
                        static_assert(BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT <= DEFORMING_SURFACE_MAX_MORPH_TARGET_WEIGHT_COUNT, "");
                        for (uint32_t morph_target_name_index = 0U; morph_target_name_index < BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT; ++morph_target_name_index)
                        {
                            BRX_ANARI_MORPH_TARGET_NAME const morph_target_name = static_cast<BRX_ANARI_MORPH_TARGET_NAME>(morph_target_name_index);

                            float const morph_target_weight = surface_group_instance->get_morph_target_weight(morph_target_name);

                            uint32_t const packed_vector_index = morph_target_name_index / 4U;
                            uint32_t const component_index = morph_target_name_index % 4U;
                            switch (component_index)
                            {
                            case 0U:
                            {
                                assert(0U == component_index);
                                deforming_surface_group_update_set_uniform_buffer_destination->g_packed_vector_morph_target_weights[packed_vector_index].x = morph_target_weight;
                            }
                            break;
                            case 1U:
                            {
                                assert(1U == component_index);
                                deforming_surface_group_update_set_uniform_buffer_destination->g_packed_vector_morph_target_weights[packed_vector_index].y = morph_target_weight;
                            }
                            break;
                            case 2U:
                            {
                                assert(2U == component_index);
                                deforming_surface_group_update_set_uniform_buffer_destination->g_packed_vector_morph_target_weights[packed_vector_index].z = morph_target_weight;
                            }
                            break;
                            default:
                            {
                                assert(3U == component_index);
                                deforming_surface_group_update_set_uniform_buffer_destination->g_packed_vector_morph_target_weights[packed_vector_index].w = morph_target_weight;
                            }
                            }
                        }

                        uint32_t const joint_count = surface_group_instance->get_skin_transform_count();
                        assert(joint_count <= DEFORMING_SURFACE_MAX_JOINT_COUNT);
                        for (uint32_t joint_index = 0U; joint_index < joint_count; ++joint_index)
                        {
                            DirectX::XMFLOAT4 skin_dual_quaternion[2];
                            {
                                brx_anari_rigid_transform const skin_rigid_transform = surface_group_instance->get_skin_transforms()[joint_index];
                                DirectX::XMFLOAT4 const skin_rotation(skin_rigid_transform.m_rotation[0], skin_rigid_transform.m_rotation[1], skin_rigid_transform.m_rotation[2], skin_rigid_transform.m_rotation[3]);
                                DirectX::XMFLOAT3 const skin_translation(skin_rigid_transform.m_translation[0], skin_rigid_transform.m_translation[1], skin_rigid_transform.m_translation[2]);
                                unit_dual_quaternion_from_rigid_transform(skin_dual_quaternion, skin_rotation, skin_translation);
                            }

                            deforming_surface_group_update_set_uniform_buffer_destination->g_dual_quaternions[2 * joint_index] = skin_dual_quaternion[0];
                            deforming_surface_group_update_set_uniform_buffer_destination->g_dual_quaternions[2 * joint_index + 1] = skin_dual_quaternion[1];
                        }
                    }
                }
            }

            uint32_t const surface_count = static_cast<uint32_t>(vertex_counts.size());

            assert(buffers.size() == (2U * surface_count));
            assert(descriptor_sets.size() == (2U * surface_count));

            if (surface_count > 0U)
            {
                command_buffer->begin_debug_utils_label("Deforming Pass");

                command_buffer->storage_resource_load_dont_care(static_cast<uint32_t>(buffers.size()), buffers.data(), 0U, NULL);

                command_buffer->bind_compute_pipeline(this->m_deforming_pipeline);

                for (uint32_t surface_index = 0U; surface_index < surface_count; ++surface_index)
                {
                    brx_pal_descriptor_set const *const descritor_sets[] = {
                        descriptor_sets[2U * surface_index],
                        descriptor_sets[2U * surface_index + 1U]};

                    uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<deforming_surface_group_update_set_uniform_buffer_binding>(this->m_frame_throttling_index)};

                    command_buffer->bind_compute_descriptor_sets(this->m_deforming_surface_update_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

                    uint32_t const vertex_count = vertex_counts[surface_index];

                    uint32_t const group_count_x = (vertex_count <= DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION) ? vertex_count : DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION;
                    uint32_t const group_count_y = (vertex_count + DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION - 1U) / DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION;
                    assert(group_count_x <= DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION);
                    assert(group_count_y <= DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION);

                    command_buffer->dispatch(group_count_x, group_count_y, 1U);
                }

                command_buffer->storage_resource_store(static_cast<size_t>(buffers.size()), buffers.data(), 0U, NULL);

                command_buffer->end_debug_utils_label();
            }
        }

        // Upload Surface Uniform Buffer
        {
            for (auto const &world_surface_group_instance : this->m_world_surface_group_instances)
            {
                brx_anari_surface_group const *const wrapped_surface_group = world_surface_group_instance.first;

                brx_anari_pal_surface_group const *const surface_group = static_cast<brx_anari_pal_surface_group const *>(wrapped_surface_group);

                for (brx_anari_surface_group_instance const *const wrapped_surface_group_instance : world_surface_group_instance.second)
                {
                    brx_anari_pal_surface_group_instance const *const surface_group_instance = static_cast<brx_anari_pal_surface_group_instance const *>(wrapped_surface_group_instance);

                    assert(surface_group_instance->get_surface_group() == surface_group);

                    // Upload Uniform Buffer
                    {
                        surface_group_update_set_uniform_buffer_binding *const surface_group_update_set_uniform_buffer_destination = this->helper_compute_uniform_buffer_memory_address<surface_group_update_set_uniform_buffer_binding>(this->m_frame_throttling_index, surface_group_instance->get_surface_group_update_set_uniform_buffer());

                        DirectX::XMFLOAT4X4 model_transform;
                        {
                            brx_anari_rigid_transform model_rigid_transform = surface_group_instance->get_model_transform();
                            DirectX::XMFLOAT4 const model_rotation(model_rigid_transform.m_rotation[0], model_rigid_transform.m_rotation[1], model_rigid_transform.m_rotation[2], model_rigid_transform.m_rotation[3]);
                            DirectX::XMFLOAT3 const model_translation(model_rigid_transform.m_translation[0], model_rigid_transform.m_translation[1], model_rigid_transform.m_translation[2]);
                            DirectX::XMMATRIX simd_model_transform = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&model_rotation)), DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&model_translation)));
                            DirectX::XMStoreFloat4x4(&model_transform, simd_model_transform);
                        }

                        surface_group_update_set_uniform_buffer_destination->g_model_transform = model_transform;
                    }
                }
            }
        }

        // Forward Shading Pass
        {
            command_buffer->begin_debug_utils_label("Forward Shading Pass");

            {
                float const color_clear_values[5][4] = {{0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}};
                float const depth_clear_value = 0.0F;
                command_buffer->begin_render_pass(this->m_forward_shading_render_pass, this->m_forward_shading_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, sizeof(color_clear_values) / sizeof(color_clear_values[0]), &color_clear_values[0], &depth_clear_value, NULL);
            }

            command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

            command_buffer->set_scissor(0, 0, this->m_intermediate_width, this->m_intermediate_height);

            // Forward Shading
            {
                command_buffer->bind_graphics_pipeline(this->m_forward_shading_pipeline);

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
                                    this->m_none_update_descriptor_set,
                                    surface_group_instance->get_surface_group_update_descriptor_set(),
                                    surface->get_deforming() ? surface_instance->get_surface_update_descriptor_set() : surface->get_surface_update_descriptor_set()};

                                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(this->m_frame_throttling_index), this->helper_compute_uniform_buffer_dynamic_offset<surface_group_update_set_uniform_buffer_binding>(this->m_frame_throttling_index)};

                                command_buffer->bind_graphics_descriptor_sets(this->m_surface_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
                            }

                            command_buffer->draw(surface->get_index_count(), 1U, 0U, 0U);
                        }
                    }
                }
            }

            // Quad Light Emissive
            this->quad_light_render_emissive(this->m_frame_throttling_index, command_buffer);

            // HDRI Light Skybox
            this->hdri_light_render_skybox(this->m_frame_throttling_index, command_buffer, this->m_hdri_light_layout);

            command_buffer->end_render_pass();

            command_buffer->end_debug_utils_label();
        }

        // GI
        this->voxel_cone_tracing_render(this->m_frame_throttling_index, command_buffer, this->m_voxel_cone_tracing_dirty, this->m_renderer_gi_quality);

        // Post Processing Pass
        {
            command_buffer->begin_debug_utils_label("Post Processing Pass");

            float const color_clear_values[4] = {0.0F, 0.0F, 0.0F, 0.0F};
            command_buffer->begin_render_pass(this->m_post_processing_render_pass, this->m_post_processing_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, 1U, &color_clear_values, NULL, NULL);

            command_buffer->bind_graphics_pipeline(this->m_post_processing_pipeline);

            command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

            command_buffer->set_scissor(0, 0, this->m_intermediate_width, this->m_intermediate_height);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_none_update_descriptor_set};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(this->m_frame_throttling_index)};

                command_buffer->bind_graphics_descriptor_sets(this->m_none_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            command_buffer->draw(3U, 1U, 0U, 0U);

            command_buffer->end_render_pass();

            command_buffer->end_debug_utils_label();
        }
    }

    uint32_t swap_chain_image_index = -1;
    bool acquire_next_image_not_out_of_date = this->m_device->acquire_next_image(this->m_graphics_command_buffers[this->m_frame_throttling_index], this->m_swap_chain, &swap_chain_image_index);
    if (!acquire_next_image_not_out_of_date)
    {
        // NOTE: we should end the command buffer before we destroy the bound image
        this->m_graphics_command_buffers[this->m_frame_throttling_index]->end();

        for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
        {
            this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
        }

        this->detach_swap_chain(this->m_renderer_gi_quality);
        this->attach_swap_chain(this->m_renderer_gi_quality);

        // skip this frame
        return;
    }

    // Swap Chain Pass
    {
        this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin_debug_utils_label("Swap Chain Pass");

        float const color_clear_values[4] = {0.0F, 0.0F, 0.0F, 0.0F};
        this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin_render_pass(this->m_swap_chain_render_pass, this->m_swap_chain_frame_buffers[swap_chain_image_index], this->m_swap_chain_image_width, this->m_swap_chain_image_height, 1U, &color_clear_values, NULL, NULL);

        // Full Screen Transfer
        {
            this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin_debug_utils_label("Full Screen Transfer");

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->set_view_port(this->m_swap_chain_image_width, this->m_swap_chain_image_height);

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->set_scissor(0, 0, this->m_swap_chain_image_width, this->m_swap_chain_image_height);

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->bind_graphics_pipeline(this->m_full_screen_transfer_pipeline);

            brx_pal_descriptor_set *const descritor_sets[] = {this->m_full_screen_transfer_descriptor_set_none_update};
            this->m_graphics_command_buffers[this->m_frame_throttling_index]->bind_graphics_descriptor_sets(this->m_full_screen_transfer_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, 0U, NULL);

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->draw(3U, 1U, 0U, 0U);

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->end_debug_utils_label();
        }

        // ImGui
        if (ui_view)
        {
            this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin_debug_utils_label("ImGui");

            ImGui_ImplBrx_RenderDrawData(this->m_graphics_command_buffers[this->m_frame_throttling_index], this->m_frame_throttling_index);

            this->m_graphics_command_buffers[this->m_frame_throttling_index]->end_debug_utils_label();
        }

        this->m_graphics_command_buffers[this->m_frame_throttling_index]->end_render_pass();

        this->m_graphics_command_buffers[this->m_frame_throttling_index]->end_debug_utils_label();
    }

    this->m_graphics_command_buffers[this->m_frame_throttling_index]->end();

    this->m_device->reset_fence(this->m_fences[this->m_frame_throttling_index]);

    bool present_not_out_of_date = this->m_graphics_queue->submit_and_present(this->m_graphics_command_buffers[this->m_frame_throttling_index], this->m_swap_chain, swap_chain_image_index, this->m_fences[this->m_frame_throttling_index]);
    if (!present_not_out_of_date)
    {
        for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
        {
            this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
        }

        this->detach_swap_chain(this->m_renderer_gi_quality);
        this->attach_swap_chain(this->m_renderer_gi_quality);

        // continue this frame
    }

    ++this->m_frame_throttling_index;
    this->m_frame_throttling_index %= INTERNAL_FRAME_THROTTLING_COUNT;

#ifndef NDEBUG
    this->m_voxel_cone_tracing_dirty_lock = false;

    this->m_hdri_light_dirty_lock = false;

    this->m_renderer_gi_quality_lock = false;

    this->m_hdri_light_layout_lock = false;

    this->m_frame_throttling_index_lock = false;
#endif
}
