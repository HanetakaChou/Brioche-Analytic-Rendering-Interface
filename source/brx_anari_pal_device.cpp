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
#include "../shaders/shared_none_update_set_uniform_buffer_binding.sli"

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
      m_shared_none_update_set_uniform_buffer(NULL),
      m_shared_none_update_set_sampler(NULL),
      m_deforming_descriptor_set_layout_per_surface_group_update(NULL),
      m_deforming_descriptor_set_layout_per_surface_update(NULL),
      m_deforming_pipeline_layout(NULL),
      m_forward_shading_descriptor_set_none_update(NULL),
      m_forward_shading_descriptor_set_layout_per_surface_group_update(NULL),
      m_forward_shading_descriptor_set_layout_per_surface_update(NULL),
      m_forward_shading_pipeline_layout(NULL),
      m_post_processing_descriptor_set_layout_none_update(NULL),
      m_post_processing_descriptor_set_none_update(NULL),
      m_post_processing_pipeline_layout(NULL),
      m_deforming_pipeline(NULL),
      m_scene_color_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32),
      m_gbuffer_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32_UINT),
      m_scene_depth_image_format(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED),
      m_forward_shading_render_pass(NULL),
      m_forward_shading_pipeline(NULL),
      m_display_color_image_format(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32),
      m_post_processing_render_pass(NULL),
      m_post_processing_pipeline(NULL),
      m_intermediate_width(0U),
      m_intermediate_height(0U),
      m_scene_color_image(NULL),
      m_gbuffer_image(NULL),
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
      m_frame_throttling_index(BRX_ANARI_UINT32_INDEX_INVALID)
{
}

brx_anari_pal_device::~brx_anari_pal_device()
{
    assert(NULL == this->m_device);
    assert(0U == this->m_uniform_upload_buffer_offset_alignment);
    assert(NULL == this->m_shared_none_update_set_uniform_buffer);
    assert(NULL == this->m_shared_none_update_set_sampler);
    assert(NULL == this->m_deforming_descriptor_set_layout_per_surface_group_update);
    assert(NULL == this->m_deforming_descriptor_set_layout_per_surface_update);
    assert(NULL == this->m_deforming_pipeline_layout);
    assert(NULL == this->m_forward_shading_descriptor_set_none_update);
    assert(NULL == this->m_forward_shading_descriptor_set_layout_per_surface_group_update);
    assert(NULL == this->m_forward_shading_descriptor_set_layout_per_surface_update);
    assert(NULL == this->m_forward_shading_pipeline_layout);
    assert(NULL == this->m_post_processing_descriptor_set_layout_none_update);
    assert(NULL == this->m_post_processing_descriptor_set_none_update);
    assert(NULL == this->m_post_processing_pipeline_layout);
    assert(NULL == this->m_deforming_pipeline);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32 == this->m_scene_color_image_format);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32_UINT == this->m_gbuffer_image_format);
    assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_scene_depth_image_format);
    assert(NULL == this->m_forward_shading_render_pass);
    assert(NULL == this->m_forward_shading_pipeline);
    assert(BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32 == this->m_display_color_image_format);
    assert(NULL == this->m_post_processing_render_pass);
    assert(NULL == this->m_post_processing_pipeline);
    assert(0U == this->m_intermediate_width);
    assert(0U == this->m_intermediate_height);
    assert(NULL == this->m_scene_color_image);
    assert(NULL == this->m_gbuffer_image);
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
}

void brx_anari_pal_device::init(void *wsi_connection)
{
    assert(NULL == this->m_device);
    this->m_device = brx_pal_create_device(wsi_connection, false);

    assert(0U == this->m_uniform_upload_buffer_offset_alignment);
    this->m_uniform_upload_buffer_offset_alignment = this->m_device->get_uniform_upload_buffer_offset_alignment();

    // Scene Renderer
    {
        // Uniform Buffer
        {
            assert(NULL == this->m_shared_none_update_set_uniform_buffer);

            this->m_shared_none_update_set_uniform_buffer = this->m_device->create_uniform_upload_buffer(internal_align_up(static_cast<uint32_t>(sizeof(shared_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * INTERNAL_FRAME_THROTTLING_COUNT);
        }

        // Sampler
        {
            assert(NULL == this->m_shared_none_update_set_sampler);

            this->m_shared_none_update_set_sampler = this->m_device->create_sampler(BRX_PAL_SAMPLER_FILTER_LINEAR);
        }

        // Descriptor/Pipeline Layout and None Update Descriptor
        {
            brx_pal_descriptor_set_layout *forward_shading_descriptor_set_layout_none_update;

            // Deforming Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_deforming_descriptor_set_layout_per_surface_group_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const skin_pipeline_descriptor_set_layout_per_surface_group_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
                this->m_deforming_descriptor_set_layout_per_surface_group_update = this->m_device->create_descriptor_set_layout(sizeof(skin_pipeline_descriptor_set_layout_per_surface_group_update_bindings) / sizeof(skin_pipeline_descriptor_set_layout_per_surface_group_update_bindings[0]), skin_pipeline_descriptor_set_layout_per_surface_group_update_bindings);

                assert(NULL == this->m_deforming_descriptor_set_layout_per_surface_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const skin_pipeline_descriptor_set_layout_per_surface_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 3U},
                    {1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2U}};
                this->m_deforming_descriptor_set_layout_per_surface_update = this->m_device->create_descriptor_set_layout(sizeof(skin_pipeline_descriptor_set_layout_per_surface_update_bindings) / sizeof(skin_pipeline_descriptor_set_layout_per_surface_update_bindings[0]), skin_pipeline_descriptor_set_layout_per_surface_update_bindings);

                assert(NULL == this->m_deforming_pipeline_layout);
                brx_pal_descriptor_set_layout *const skin_pipeline_descriptor_set_layouts[] = {
                    this->m_deforming_descriptor_set_layout_per_surface_group_update, this->m_deforming_descriptor_set_layout_per_surface_update};
                this->m_deforming_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(skin_pipeline_descriptor_set_layouts) / sizeof(skin_pipeline_descriptor_set_layouts[0]), skin_pipeline_descriptor_set_layouts);
            }

            // Forward Shading Descriptor/Pipeline Layout
            {
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const forward_shading_descriptor_set_layout_none_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U},
                    {1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 1U}};
                forward_shading_descriptor_set_layout_none_update = this->m_device->create_descriptor_set_layout(sizeof(forward_shading_descriptor_set_layout_none_update_bindings) / sizeof(forward_shading_descriptor_set_layout_none_update_bindings[0]), forward_shading_descriptor_set_layout_none_update_bindings);

                assert(NULL == this->m_forward_shading_descriptor_set_layout_per_surface_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const forward_shading_descriptor_set_layout_per_surface_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 4U},
                    {4U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4U}};
                this->m_forward_shading_descriptor_set_layout_per_surface_update = this->m_device->create_descriptor_set_layout(sizeof(forward_shading_descriptor_set_layout_per_surface_update_bindings) / sizeof(forward_shading_descriptor_set_layout_per_surface_update_bindings[0]), forward_shading_descriptor_set_layout_per_surface_update_bindings);

                assert(NULL == this->m_forward_shading_descriptor_set_layout_per_surface_group_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const forward_shading_descriptor_set_layout_per_surface_group_update_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
                this->m_forward_shading_descriptor_set_layout_per_surface_group_update = this->m_device->create_descriptor_set_layout(sizeof(forward_shading_descriptor_set_layout_per_surface_group_update_bindings) / sizeof(forward_shading_descriptor_set_layout_per_surface_group_update_bindings[0]), forward_shading_descriptor_set_layout_per_surface_group_update_bindings);

                assert(NULL == this->m_forward_shading_pipeline_layout);
                brx_pal_descriptor_set_layout *const forward_shading_pipeline_descriptor_set_layouts[] = {
                    forward_shading_descriptor_set_layout_none_update,
                    this->m_forward_shading_descriptor_set_layout_per_surface_update,
                    this->m_forward_shading_descriptor_set_layout_per_surface_group_update};
                this->m_forward_shading_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(forward_shading_pipeline_descriptor_set_layouts) / sizeof(forward_shading_pipeline_descriptor_set_layouts[0]), forward_shading_pipeline_descriptor_set_layouts);
            }

            // Forward Shading None Update Descriptor
            {
                assert(NULL == this->m_forward_shading_descriptor_set_none_update);
                this->m_forward_shading_descriptor_set_none_update = this->m_device->create_descriptor_set(forward_shading_descriptor_set_layout_none_update, 0U);
            }

            // Write Forward Shading None Update Descriptor
            {
                {
                    constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(shared_none_update_set_uniform_buffer_binding);

                    this->m_device->write_descriptor_set(this->m_forward_shading_descriptor_set_none_update, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_shared_none_update_set_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
                }

                {
                    this->m_device->write_descriptor_set(this->m_forward_shading_descriptor_set_none_update, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_shared_none_update_set_sampler, NULL);
                }
            }

            // Post Processing Descriptor/Pipeline Layout
            {
                assert(NULL == this->m_post_processing_descriptor_set_layout_none_update);
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const post_processing_pipeline_none_update_descriptor_set_layout_bindings[] = {
                    {0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U},
                    {1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {2U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
                    {3U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
                this->m_post_processing_descriptor_set_layout_none_update = this->m_device->create_descriptor_set_layout(sizeof(post_processing_pipeline_none_update_descriptor_set_layout_bindings) / sizeof(post_processing_pipeline_none_update_descriptor_set_layout_bindings[0]), post_processing_pipeline_none_update_descriptor_set_layout_bindings);

                assert(NULL == this->m_post_processing_pipeline_layout);
                brx_pal_descriptor_set_layout *const post_processing_pipeline_descriptor_set_layouts[] = {
                    this->m_post_processing_descriptor_set_layout_none_update};
                this->m_post_processing_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(post_processing_pipeline_descriptor_set_layouts) / sizeof(post_processing_pipeline_descriptor_set_layouts[0]), post_processing_pipeline_descriptor_set_layouts);
            }

            // Post Processing None Update Descriptor
            {
                assert(NULL == this->m_post_processing_descriptor_set_none_update);
                this->m_post_processing_descriptor_set_none_update = this->m_device->create_descriptor_set(this->m_post_processing_descriptor_set_layout_none_update, 0U);
            }

            // Write Post Processing None Update Descriptor
            {
                constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(shared_none_update_set_uniform_buffer_binding);
                this->m_device->write_descriptor_set(this->m_post_processing_descriptor_set_none_update, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_shared_none_update_set_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
            }

            this->m_device->destroy_descriptor_set_layout(forward_shading_descriptor_set_layout_none_update);
            forward_shading_descriptor_set_layout_none_update = NULL;
        }

        // Render Pass and Pipeline
        {
            // Deforming Pipeline
            {
                assert(NULL == this->m_deforming_pipeline);
                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/deforming_compute.inl"
                    this->m_deforming_pipeline = this->m_device->create_compute_pipeline(this->m_deforming_pipeline_layout, sizeof(deforming_compute_shader_module_code), deforming_compute_shader_module_code);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
            }

            // Forward Shading Render Pass
            {
                assert(NULL == this->m_forward_shading_render_pass);

                assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_scene_depth_image_format);
                this->m_scene_depth_image_format = this->m_device->get_depth_attachment_image_format();

                BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const color_attachment[] = {
                    {this->m_scene_color_image_format,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
                     BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE},
                    {this->m_gbuffer_image_format,
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
                assert(NULL == this->m_forward_shading_pipeline);

                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/forward_shading_vertex.inl"
#include "../shaders/dxil/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_forward_shading_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/forward_shading_vertex.inl"
#include "../shaders/spirv/forward_shading_fragment.inl"
                    this->m_forward_shading_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_forward_shading_pipeline_layout, sizeof(forward_shading_vertex_shader_module_code), forward_shading_vertex_shader_module_code, sizeof(forward_shading_fragment_shader_module_code), forward_shading_fragment_shader_module_code, true, true, true, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
            }

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

                switch (this->m_device->get_backend_name())
                {
                case BRX_PAL_BACKEND_NAME_D3D12:
                {
#include "../shaders/dxil/post_processing_vertex.inl"
#include "../shaders/dxil/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_post_processing_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, true, true, false, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_UNKNOWN, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
                }
                break;
                case BRX_PAL_BACKEND_NAME_VK:
                {
#include "../shaders/spirv/post_processing_vertex.inl"
#include "../shaders/spirv/post_processing_fragment.inl"
                    this->m_post_processing_pipeline = this->m_device->create_graphics_pipeline(this->m_post_processing_render_pass, this->m_post_processing_pipeline_layout, sizeof(post_processing_vertex_shader_module_code), post_processing_vertex_shader_module_code, sizeof(post_processing_fragment_shader_module_code), post_processing_fragment_shader_module_code, true, true, false, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_UNKNOWN, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
                }
                break;
                default:
                {
                    assert(false);
                }
                }
            }
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
                BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const full_screen_transfer_none_update_descriptor_set_layout_bindings[] = {{0U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
                this->m_full_screen_transfer_descriptor_set_layout_none_update = this->m_device->create_descriptor_set_layout(sizeof(full_screen_transfer_none_update_descriptor_set_layout_bindings) / sizeof(full_screen_transfer_none_update_descriptor_set_layout_bindings[0]), full_screen_transfer_none_update_descriptor_set_layout_bindings);

                assert(NULL == this->m_full_screen_transfer_pipeline_layout);
                brx_pal_descriptor_set_layout *const full_screen_transfer_descriptor_set_layouts[] = {this->m_full_screen_transfer_descriptor_set_layout_none_update};
                this->m_full_screen_transfer_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(full_screen_transfer_descriptor_set_layouts) / sizeof(full_screen_transfer_descriptor_set_layouts[0]), full_screen_transfer_descriptor_set_layouts);
            }

            //  None Update Descriptor
            {
                assert(NULL == this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_full_screen_transfer_descriptor_set_none_update = this->m_device->create_descriptor_set(this->m_full_screen_transfer_descriptor_set_layout_none_update, 0U);
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
}

void brx_anari_pal_device::uninit()
{
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

    // Frame Throttling
    assert(BRX_ANARI_UINT32_INDEX_INVALID != this->m_frame_throttling_index);
    this->m_frame_throttling_index = BRX_ANARI_UINT32_INDEX_INVALID;

    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(NULL != this->m_fences[frame_throttling_index]);
        this->m_device->destroy_fence(this->m_fences[frame_throttling_index]);
        this->m_fences[frame_throttling_index] = NULL;

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
            //  None Update Descriptor
            {
                assert(NULL != this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_device->destroy_descriptor_set(this->m_full_screen_transfer_descriptor_set_none_update);
                this->m_full_screen_transfer_descriptor_set_none_update = NULL;
            }

            // Deforming Descriptor/Pipeline Layout
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
            // Deforming Descriptor/Pipeline Layout
            {

                assert(NULL != this->m_deforming_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_deforming_pipeline_layout);
                this->m_deforming_pipeline_layout = NULL;

                assert(NULL != this->m_deforming_descriptor_set_layout_per_surface_group_update);
                this->m_device->destroy_descriptor_set_layout(this->m_deforming_descriptor_set_layout_per_surface_group_update);
                this->m_deforming_descriptor_set_layout_per_surface_group_update = NULL;

                assert(NULL != this->m_deforming_descriptor_set_layout_per_surface_update);
                this->m_device->destroy_descriptor_set_layout(this->m_deforming_descriptor_set_layout_per_surface_update);
                this->m_deforming_descriptor_set_layout_per_surface_update = NULL;
            }

            // Forward Shading None Update Descriptor
            {
                assert(NULL != this->m_forward_shading_descriptor_set_none_update);
                this->m_device->destroy_descriptor_set(this->m_forward_shading_descriptor_set_none_update);
                this->m_forward_shading_descriptor_set_none_update = NULL;
            }

            // Forward Shading Descriptor/Pipeline Layout
            {

                assert(NULL != this->m_forward_shading_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_forward_shading_pipeline_layout);
                this->m_forward_shading_pipeline_layout = NULL;

                assert(NULL != this->m_forward_shading_descriptor_set_layout_per_surface_group_update);
                this->m_device->destroy_descriptor_set_layout(this->m_forward_shading_descriptor_set_layout_per_surface_group_update);
                this->m_forward_shading_descriptor_set_layout_per_surface_group_update = NULL;

                assert(NULL != this->m_forward_shading_descriptor_set_layout_per_surface_update);
                this->m_device->destroy_descriptor_set_layout(this->m_forward_shading_descriptor_set_layout_per_surface_update);
                this->m_forward_shading_descriptor_set_layout_per_surface_update = NULL;
            }

            // Post Processing None Update Descriptor
            {
                assert(NULL != this->m_post_processing_descriptor_set_none_update);
                this->m_device->destroy_descriptor_set(this->m_post_processing_descriptor_set_none_update);
                this->m_post_processing_descriptor_set_none_update = NULL;
            }

            // Post Processing Descriptor/Pipeline Layout
            {
                assert(NULL != this->m_post_processing_pipeline_layout);
                this->m_device->destroy_pipeline_layout(this->m_post_processing_pipeline_layout);
                this->m_post_processing_pipeline_layout = NULL;

                assert(NULL != this->m_post_processing_descriptor_set_layout_none_update);
                this->m_device->destroy_descriptor_set_layout(this->m_post_processing_descriptor_set_layout_none_update);
                this->m_post_processing_descriptor_set_layout_none_update = NULL;
            }
        }

        // Uniform Buffer
        {
            assert(NULL != this->m_shared_none_update_set_uniform_buffer);
            this->m_device->destroy_uniform_upload_buffer(this->m_shared_none_update_set_uniform_buffer);
            this->m_shared_none_update_set_uniform_buffer = NULL;
        }

        // Sampler
        {
            assert(NULL != this->m_shared_none_update_set_sampler);
            this->m_device->destroy_sampler(this->m_shared_none_update_set_sampler);
            this->m_shared_none_update_set_sampler = NULL;
        }
    }

    assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);
    this->m_uniform_upload_buffer_offset_alignment = 0U;

    assert(NULL != this->m_device);
    brx_pal_destroy_device(this->m_device);
    this->m_device = NULL;
}

void brx_anari_pal_device::frame_attach_window(void *wsi_window)
{
    assert(NULL == this->m_surface);
    this->m_surface = this->m_device->create_surface(wsi_window);

    this->attach_swap_chain();
}

void brx_anari_pal_device::frame_resize_window()
{
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

    this->detach_swap_chain();
    this->attach_swap_chain();
}

void brx_anari_pal_device::frame_detach_window()
{
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
    }

    this->detach_swap_chain();

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

            switch (this->m_device->get_backend_name())
            {
            case BRX_PAL_BACKEND_NAME_D3D12:
            {
#include "../shaders/dxil/full_screen_transfer_vertex.inl"
#include "../shaders/dxil/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, false, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_UNKNOWN, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
            }
            break;
            case BRX_PAL_BACKEND_NAME_VK:
            {
#include "../shaders/spirv/full_screen_transfer_vertex.inl"
#include "../shaders/spirv/full_screen_transfer_fragment.inl"
                this->m_full_screen_transfer_pipeline = this->m_device->create_graphics_pipeline(this->m_swap_chain_render_pass, this->m_full_screen_transfer_pipeline_layout, sizeof(full_screen_transfer_vertex_shader_module_code), full_screen_transfer_vertex_shader_module_code, sizeof(full_screen_transfer_fragment_shader_module_code), full_screen_transfer_fragment_shader_module_code, false, true, false, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_UNKNOWN, false, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN);
            }
            break;
            default:
            {
                assert(false);
            }
            }
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

inline void brx_anari_pal_device::attach_swap_chain()
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
            this->m_intermediate_width = this->m_swap_chain_image_width;

            assert(0U == this->m_intermediate_height);
            this->m_intermediate_height = this->m_swap_chain_image_height;

            // Forward Shading
            {
                assert(NULL == this->m_scene_color_image);
                this->m_scene_color_image = this->m_device->create_color_attachment_image(this->m_scene_color_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_gbuffer_image);
                this->m_gbuffer_image = this->m_device->create_color_attachment_image(this->m_gbuffer_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_scene_depth_image);
                this->m_scene_depth_image = this->m_device->create_depth_stencil_attachment_image(this->m_scene_depth_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_forward_shading_frame_buffer);
                brx_pal_color_attachment_image const *const color_attachments[] = {this->m_scene_color_image, this->m_gbuffer_image};
                this->m_forward_shading_frame_buffer = this->m_device->create_frame_buffer(this->m_forward_shading_render_pass, this->m_intermediate_width, this->m_intermediate_height, sizeof(color_attachments) / sizeof(color_attachments[0]), color_attachments, this->m_scene_depth_image);
            }

            // Post Processing
            {
                assert(NULL == this->m_display_color_image);
                this->m_display_color_image = this->m_device->create_color_attachment_image(this->m_display_color_image_format, this->m_intermediate_width, this->m_intermediate_height, true);

                assert(NULL == this->m_post_processing_frame_buffer);
                this->m_post_processing_frame_buffer = this->m_device->create_frame_buffer(this->m_post_processing_render_pass, this->m_intermediate_width, this->m_intermediate_height, 1U, &this->m_display_color_image, NULL);
            }
        }

        // Write Descriptor
        {
            // Post Process
            {
                // The VkDescriptorSetLayout should still be valid when perform write update on VkDescriptorSet
                assert(NULL != this->m_post_processing_descriptor_set_layout_none_update);
                {
                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_scene_color_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_post_processing_descriptor_set_none_update, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_gbuffer_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_post_processing_descriptor_set_none_update, 2U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                    }

                    {
                        brx_pal_sampled_image const *const sampled_images[] = {this->m_scene_depth_image->get_sampled_image()};
                        this->m_device->write_descriptor_set(this->m_post_processing_descriptor_set_none_update, 3U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
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
                    this->m_device->write_descriptor_set(this->m_full_screen_transfer_descriptor_set_none_update, 0U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
                }
            }
        }
    }
}

inline void brx_anari_pal_device::detach_swap_chain()
{
    // Scene Renderer
    {
        // Intermediate Image and Frame Buffer
        {
            // Forward Shading
            {
                assert(NULL != this->m_forward_shading_frame_buffer);
                this->m_device->destroy_frame_buffer(this->m_forward_shading_frame_buffer);
                this->m_forward_shading_frame_buffer = NULL;

                assert(NULL != this->m_scene_color_image);
                this->m_device->destroy_color_attachment_image(this->m_scene_color_image);
                this->m_scene_color_image = NULL;

                assert(NULL != this->m_gbuffer_image);
                this->m_device->destroy_color_attachment_image(this->m_gbuffer_image);
                this->m_gbuffer_image = NULL;

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

void brx_anari_pal_device::renderer_render_frame()
{
    if (NULL == this->m_surface)
    {
        // skip this frame
        return;
    }

    assert(NULL != this->m_swap_chain);

    this->m_device->wait_for_fence(this->m_fences[this->m_frame_throttling_index]);

    // Destroy Resources NOT Used By GPU
    {
        if (!this->m_pending_destroy_descriptor_sets[this->m_frame_throttling_index].empty())
        {
            for (brx_pal_descriptor_set *(&pending_destroy_descriptor_set) : this->m_pending_destroy_descriptor_sets[this->m_frame_throttling_index])
            {
                assert(NULL != pending_destroy_descriptor_set);
                this->m_device->destroy_descriptor_set(pending_destroy_descriptor_set);
                pending_destroy_descriptor_set = NULL;
            }
            this->m_pending_destroy_descriptor_sets[this->m_frame_throttling_index].clear();
        }

        if (!this->m_pending_destroy_uniform_upload_buffers[this->m_frame_throttling_index].empty())
        {
            for (brx_pal_uniform_upload_buffer *(&pending_destroy_uniform_upload_buffer) : this->m_pending_destroy_uniform_upload_buffers[this->m_frame_throttling_index])
            {
                assert(NULL != pending_destroy_uniform_upload_buffer);
                this->m_device->destroy_uniform_upload_buffer(pending_destroy_uniform_upload_buffer);
                pending_destroy_uniform_upload_buffer = NULL;
            }
            this->m_pending_destroy_uniform_upload_buffers[this->m_frame_throttling_index].clear();
        }

        if (!this->m_pending_destroy_storage_intermediate_buffers[this->m_frame_throttling_index].empty())
        {
            for (brx_pal_storage_intermediate_buffer *(&pending_destroy_storage_intermediate_buffer) : this->m_pending_destroy_storage_intermediate_buffers[this->m_frame_throttling_index])
            {
                assert(NULL != pending_destroy_storage_intermediate_buffer);
                this->m_device->destroy_storage_intermediate_buffer(pending_destroy_storage_intermediate_buffer);
                pending_destroy_storage_intermediate_buffer = NULL;
            }
            this->m_pending_destroy_storage_intermediate_buffers[this->m_frame_throttling_index].clear();
        }

        if (!this->m_pending_destroy_storage_asset_buffers[this->m_frame_throttling_index].empty())
        {
            for (brx_pal_storage_asset_buffer *(&pending_destroy_storage_asset_buffer) : this->m_pending_destroy_storage_asset_buffers[this->m_frame_throttling_index])
            {
                assert(NULL != pending_destroy_storage_asset_buffer);
                this->m_device->destroy_storage_asset_buffer(pending_destroy_storage_asset_buffer);
                pending_destroy_storage_asset_buffer = NULL;
            }
            this->m_pending_destroy_storage_asset_buffers[this->m_frame_throttling_index].clear();
        }

        if (!this->m_pending_destroy_sampled_asset_images[this->m_frame_throttling_index].empty())
        {
            for (brx_pal_sampled_asset_image *(&pending_destroy_sampled_asset_image) : this->m_pending_destroy_sampled_asset_images[this->m_frame_throttling_index])
            {
                assert(NULL != pending_destroy_sampled_asset_image);
                this->m_device->destroy_sampled_asset_image(pending_destroy_sampled_asset_image);
                pending_destroy_sampled_asset_image = NULL;
            }
            this->m_pending_destroy_sampled_asset_images[this->m_frame_throttling_index].clear();
        }
    }

    this->m_device->reset_graphics_command_buffer(this->m_graphics_command_buffers[this->m_frame_throttling_index]);

    this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin();

    // Scene Renderer
    {
        brx_pal_graphics_command_buffer *const command_buffer = this->m_graphics_command_buffers[this->m_frame_throttling_index];
        assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);

        // Deforming
        {
        }

        // Forward Shading Pass
        {

            command_buffer->begin_debug_utils_label("Forward Shading Pass");

            float const color_clear_values[2][4] = {{0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}};
            float const depth_clear_value = 0.0F;
            command_buffer->begin_render_pass(this->m_forward_shading_render_pass, this->m_forward_shading_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, 2U, &color_clear_values[0], &depth_clear_value, NULL);

            command_buffer->bind_graphics_pipeline(this->m_forward_shading_pipeline);

            command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

            command_buffer->set_scissor(0, 0, this->m_intermediate_width, this->m_intermediate_height);

            command_buffer->end_render_pass();

            command_buffer->end_debug_utils_label();
        }

        // Post Processing Pass
        {
            command_buffer->begin_debug_utils_label("Post Processing Pass");

            float const color_clear_values[4] = {0.0F, 0.0F, 0.0F, 0.0F};
            command_buffer->begin_render_pass(this->m_post_processing_render_pass, this->m_post_processing_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, 1U, &color_clear_values, NULL, NULL);

            command_buffer->bind_graphics_pipeline(this->m_post_processing_pipeline);

            command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

            command_buffer->set_scissor(0, 0, this->m_intermediate_width, this->m_intermediate_height);

            brx_pal_descriptor_set *const descriptor_sets[] = {this->m_post_processing_descriptor_set_none_update};
            uint32_t const dynamic_offsets[] = {internal_align_up(static_cast<uint32_t>(sizeof(shared_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * this->m_frame_throttling_index};
            command_buffer->bind_graphics_descriptor_sets(this->m_post_processing_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

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

        this->detach_swap_chain();
        this->attach_swap_chain();

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
        if (true) // ui_draw_data
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

        this->detach_swap_chain();
        this->attach_swap_chain();

        // continue this frame
    }

    ++this->m_frame_throttling_index;
    this->m_frame_throttling_index %= INTERNAL_FRAME_THROTTLING_COUNT;
}

extern uint32_t internal_align_up(uint32_t value, uint32_t alignment)
{
    //
    //  Copyright (c) 2005-2019 Intel Corporation
    //
    //  Licensed under the Apache License, Version 2.0 (the "License");
    //  you may not use this file except in compliance with the License.
    //  You may obtain a copy of the License at
    //
    //      http://www.apache.org/licenses/LICENSE-2.0
    //
    //  Unless required by applicable law or agreed to in writing, software
    //  distributed under the License is distributed on an "AS IS" BASIS,
    //  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    //  See the License for the specific language governing permissions and
    //  limitations under the License.
    //

    // [alignUp](https://github.com/oneapi-src/oneTBB/blob/tbb_2019/src/tbbmalloc/shared_utils.h#L42)

    assert(alignment != static_cast<uint32_t>(0U));

    // power-of-2 alignment
    assert((alignment & (alignment - static_cast<uint32_t>(1U))) == static_cast<uint32_t>(0U));

    return (((value - static_cast<uint32_t>(1U)) | (alignment - static_cast<uint32_t>(1U))) + static_cast<uint32_t>(1U));
}
