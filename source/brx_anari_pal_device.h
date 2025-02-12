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

#ifndef _BRX_ANARI_PAL_DEVICE_H_
#define _BRX_ANARI_PAL_DEVICE_H_ 1

#include "../include/brx_anari.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_unordered_map.h"
#include "../../McRT-Malloc/include/mcrt_string.h"
#include "../../Brioche-Platform-Abstraction-Layer/include/brx_pal_device.h"

class brx_anari_pal_device;
class brx_anari_pal_image;
class brx_anari_pal_surface_group;

// [Pipeline Throttling](https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/the-mali-gpu-an-abstract-machine-part-1---frame-pipelining)
static uint32_t constexpr const INTERNAL_FRAME_THROTTLING_COUNT = 3U;

extern uint32_t internal_align_up(uint32_t value, uint32_t alignment);

// The unified facade for both the UI renderer and the scene renderer

class brx_anari_pal_device final : public brx_anari_device
{
	brx_pal_device *m_device;
	uint32_t m_uniform_upload_buffer_offset_alignment;

	// Scene Renderer
	brx_pal_uniform_upload_buffer *m_shared_none_update_set_uniform_buffer;
	brx_pal_sampler *m_shared_none_update_set_sampler;
	brx_pal_descriptor_set_layout *m_deforming_descriptor_set_layout_per_surface_group_update;
	brx_pal_descriptor_set_layout *m_deforming_descriptor_set_layout_per_surface_update;
	brx_pal_pipeline_layout *m_deforming_pipeline_layout;
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_none_update;
	brx_pal_descriptor_set_layout *m_forward_shading_descriptor_set_layout_per_surface_group_update;
	brx_pal_descriptor_set_layout *m_forward_shading_descriptor_set_layout_per_surface_update;
	brx_pal_pipeline_layout *m_forward_shading_pipeline_layout;
	brx_pal_descriptor_set_layout *m_post_processing_descriptor_set_layout_none_update;
	brx_pal_descriptor_set *m_post_processing_descriptor_set_none_update;
	brx_pal_pipeline_layout *m_post_processing_pipeline_layout;
	brx_pal_compute_pipeline *m_deforming_pipeline;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_scene_color_image_format;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_gbuffer_image_format;
	BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT m_scene_depth_image_format;
	brx_pal_render_pass *m_forward_shading_render_pass;
	brx_pal_graphics_pipeline *m_forward_shading_pipeline;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_display_color_image_format;
	brx_pal_render_pass *m_post_processing_render_pass;
	brx_pal_graphics_pipeline *m_post_processing_pipeline;
	uint32_t m_intermediate_width;
	uint32_t m_intermediate_height;
	brx_pal_color_attachment_image *m_scene_color_image;
	brx_pal_color_attachment_image *m_gbuffer_image;
	brx_pal_depth_stencil_attachment_image *m_scene_depth_image;
	brx_pal_frame_buffer *m_forward_shading_frame_buffer;
	brx_pal_color_attachment_image *m_display_color_image;
	brx_pal_frame_buffer *m_post_processing_frame_buffer;

	// Facade Renderer
	brx_pal_descriptor_set_layout *m_full_screen_transfer_descriptor_set_layout_none_update;
	brx_pal_descriptor_set *m_full_screen_transfer_descriptor_set_none_update;
	brx_pal_pipeline_layout *m_full_screen_transfer_pipeline_layout;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT m_swap_chain_image_format;
	brx_pal_render_pass *m_swap_chain_render_pass;
	brx_pal_graphics_pipeline *m_full_screen_transfer_pipeline;
	brx_pal_surface *m_surface;
	brx_pal_swap_chain *m_swap_chain;
	uint32_t m_swap_chain_image_width;
	uint32_t m_swap_chain_image_height;
	mcrt_vector<brx_pal_frame_buffer *> m_swap_chain_frame_buffers;

	brx_pal_graphics_queue *m_graphics_queue;
	brx_pal_fence *m_fences[INTERNAL_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_pal_descriptor_set *> m_pending_destroy_descriptor_sets[INTERNAL_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_pal_uniform_upload_buffer *> m_pending_destroy_uniform_upload_buffers[INTERNAL_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_pal_storage_intermediate_buffer *> m_pending_destroy_storage_intermediate_buffers[INTERNAL_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_pal_storage_asset_buffer *> m_pending_destroy_storage_asset_buffers[INTERNAL_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_pal_sampled_asset_image *> m_pending_destroy_sampled_asset_images[INTERNAL_FRAME_THROTTLING_COUNT];
	brx_pal_graphics_command_buffer *m_graphics_command_buffers[INTERNAL_FRAME_THROTTLING_COUNT];

#ifndef NDEBUG
	bool m_frame_throttling_index_lock;
#endif
	uint32_t m_frame_throttling_index;

	brx_pal_storage_asset_buffer *m_place_holder_asset_buffer;
	brx_pal_sampled_asset_image *m_place_holder_asset_image;

public:
	brx_anari_pal_device();
	~brx_anari_pal_device();
	void init(void *wsi_connection);
	void uninit();

	inline brx_pal_uniform_upload_buffer *create_upload_buffer(uint32_t const size);
	inline void destroy_upload_buffer(brx_pal_uniform_upload_buffer *const buffer);

	inline brx_pal_storage_intermediate_buffer *create_intermediate_buffer(uint32_t const size);
	inline void destroy_intermediate_buffer(brx_pal_storage_intermediate_buffer *const buffer);

	brx_pal_storage_asset_buffer *internal_create_asset_buffer(void const *const data_base, uint32_t const data_size);
	void internal_destroy_asset_buffer(brx_pal_storage_asset_buffer *const buffer);

	brx_pal_sampled_asset_image *internal_create_asset_image(BRX_ANARI_IMAGE_FORMAT const format, void const *const pixel_data, uint32_t const width, uint32_t const height);
	void internal_destroy_asset_image(brx_pal_sampled_asset_image *const image);

	inline brx_pal_descriptor_set *create_deforming_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer);
	inline brx_pal_descriptor_set *create_deforming_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const vertex_blending_buffer, brx_pal_storage_buffer const *const vertex_position_buffer_instance, brx_pal_storage_buffer const *const vertex_varying_buffer_instance);
	inline brx_pal_descriptor_set *create_forward_shading_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer);
	inline brx_pal_descriptor_set *create_forward_shading_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const index_buffer, brx_pal_read_only_storage_buffer const *const auxiliary_buffer, brx_pal_sampled_image const *const emissive_image, brx_pal_sampled_image const *const normal_image, brx_pal_sampled_image const *const base_color_image, brx_pal_sampled_image const *const metallic_roughness_image);
	inline void destroy_descriptor_set(brx_pal_descriptor_set *descriptor_set);

	void release_image(brx_anari_pal_image *const image);
	void release_surface_group(brx_anari_pal_surface_group *const surface_group);

private:
	brx_anari_image *new_image(BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height) override;
	void release_image(brx_anari_image *image) override;

	brx_anari_surface_group *new_surface_group(uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces) override;
	void release_surface_group(brx_anari_surface_group *surface_group) override;

	brx_anari_surface_group_instance *world_new_surface_group_instance(brx_anari_surface_group *surface_group) override;
	void world_release_surface_group_instance(brx_anari_surface_group_instance *surface_group_instance) override;

	void frame_attach_window(void *wsi_window) override;
	void frame_resize_window() override;
	void frame_detach_window() override;

	void renderer_render_frame() override;

	inline void create_swap_chain_compatible_render_pass_and_pipeline();
	inline void destroy_swap_chain_compatible_render_pass_and_pipeline();
	inline void attach_swap_chain();
	inline void detach_swap_chain();

#if 0
	brx_anari_image *new_image(brx_asset_import_image *image, bool force_srgb) override;
	void release_image(brx_anari_image *image) override;
	void commit_buffer_and_image() override;
	brx_anari_surface_group *new_surface_group(
		brx_asset_import_surface_group *group,
		brx_asset_import_image *(pfn_get_or_create_buffer_user_callback)(brx_anari_device * device, char *url, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_w),
		brx_asset_import_image *(pfn_get_or_create_image_user_callback)(brx_anari_device * device, char *url, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_w),
		void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u) override;
	void release_surface_group(brx_anari_surface_group *group) override;
	brx_anari_surface_group_instance *new_surface_group_instance() override;
	void release_instance(brx_anari_surface_group_instance *instance) override;
	brx_anari_world *new_world() override;
	void release_world(brx_anari_world *world) override;
	brx_anari_camera *new_camera() override;
	void release_camera(brx_anari_camera *camera) override;
	brx_anari_renderer *new_renderer() override;
	void release_renderer(brx_anari_renderer *renderer) override;
	brx_anari_frame *new_frame() override;
	void release_frame(brx_anari_frame *frame) override;
	void render_frame(brx_anari_frame *frame) override;
#endif
};

class brx_anari_pal_image final : public brx_anari_image
{
	uint32_t m_ref_count;
	brx_pal_sampled_asset_image *m_image;

public:
	inline brx_anari_pal_image();
	inline ~brx_anari_pal_image();
	inline void init(brx_anari_pal_device *device, BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height);
	inline void uninit(brx_anari_pal_device *device);
	void retain();
	inline uint32_t internal_release();
	brx_pal_sampled_asset_image const *get_image() const;
};

class brx_anari_pal_surface
{
	// uint32_t m_morph_target_count;
	// uint32_t m_skeleton_joint_count;

	uint32_t m_vertex_count;
	brx_pal_storage_asset_buffer *m_vertex_position_buffer;
	brx_pal_storage_asset_buffer *m_vertex_varying_buffer;
	brx_pal_storage_asset_buffer *m_vertex_blending_buffer;
	uint32_t m_index_count;
	brx_pal_storage_asset_buffer *m_index_buffer;
	brx_anari_pal_image *m_emissive_image;
	brx_anari_pal_image *m_normal_image;
	brx_anari_pal_image *m_base_color_image;
	brx_anari_pal_image *m_metallic_roughness_image;
	brx_pal_storage_asset_buffer *m_auxiliary_buffer;

	// rasterization // available when no morph and no skin
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_per_surface_update;

public:
	inline brx_anari_pal_surface();
	inline ~brx_anari_pal_surface();
	inline void init(brx_anari_pal_device *device, BRX_ANARI_SURFACE const *surface);
	inline void uninit(brx_anari_pal_device *device);
	inline uint32_t get_vertex_count() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_position_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_varying_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_blending_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_index_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_auxiliary_buffer() const;
	inline brx_anari_pal_image const *get_emissive_image() const;
	inline brx_anari_pal_image const *get_normal_image() const;
	inline brx_anari_pal_image const *get_base_color_image() const;
	inline brx_anari_pal_image const *get_metallic_roughness_image() const;
	inline bool get_deforming() const;
};

class brx_anari_pal_surface_group final : public brx_anari_surface_group
{
	uint32_t m_ref_count;
	mcrt_vector<brx_anari_pal_surface> m_surfaces;

	// available when no morph and no skin
	// brx_pal_compacted_bottom_level_acceleration_structure *m_ray_tracing_compacted_bottom_level_acceleration_structure;

public:
	inline brx_anari_pal_surface_group();
	inline ~brx_anari_pal_surface_group();
	inline void init(brx_anari_pal_device *device, uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces);
	inline void uninit(brx_anari_pal_device *device);
	inline void retain();
	uint32_t internal_release();
	inline uint32_t get_surface_count() const;
	inline brx_anari_pal_surface const *get_surfaces() const;
};

class brx_anari_pal_surface_instance
{
	// brx_anari_brx_surface *m_surface;

	// available when morph or skin
	brx_pal_storage_intermediate_buffer *m_vertex_position_buffer;
	brx_pal_storage_intermediate_buffer *m_vertex_varying_buffer;
	brx_pal_descriptor_set *m_deforming_descriptor_set_per_surface_update;

	// rasterization // available when morph or skin
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_per_surface_update;

public:
	inline brx_anari_pal_surface_instance();
	inline ~brx_anari_pal_surface_instance();
	inline void init(brx_anari_pal_device *device, brx_anari_pal_surface const *surface);
	inline void uninit(brx_anari_pal_device *device, brx_anari_pal_surface const *surface);
};

class brx_anari_pal_surface_group_instance final : public brx_anari_surface_group_instance
{
	brx_anari_pal_surface_group *m_surface_group;

	// DirectX::XMFLOAT4X4 m_model_transform;
	// mcrt_vector<float> m_morph_target_weights;
	// mcrt_vector<DirectX::XMFLOAT4> m_skin_transforms;
	// uint32_t m_vertex_count;
	// uint32_t m_index_count;
	mcrt_vector<brx_anari_pal_surface_instance> m_surfaces;

	// available when morph or skin
	brx_pal_uniform_upload_buffer *m_deforming_per_surface_group_update_set_uniform_buffer;
	brx_pal_descriptor_set *m_deforming_descriptor_set_per_surface_group_update;

	brx_pal_uniform_upload_buffer *m_forward_shading_per_surface_group_update_set_uniform_buffer;
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_per_surface_group_update;

	// brx_pal_intermediate_bottom_level_acceleration_structure *m_ray_tracing_intermediate_bottom_level_acceleration_structure;
	// brx_pal_scratch_buffer *m_ray_tracing_intermediate_bottom_level_acceleration_structure_update_scratch_buffer;

public:
	inline brx_anari_pal_surface_group_instance();
	inline ~brx_anari_pal_surface_group_instance();
	inline void init(brx_anari_pal_device *device, brx_anari_pal_surface_group *surface_group);
	inline void uninit(brx_anari_pal_device *device);
};

#if 0
class brx_anari_pal_camera final : public brx_anari_camera
{
};

class brx_anari_pal_world final : public brx_anari_world
{
	uint32_t m_surface_count;

	uint32_t m_surface_instance_count;

	BRX_ANARI_RENDERER_TYPE m_render_type;

	uint32_t m_ray_tracing_geometry_capaticy;

	uint32_t m_ray_tracing_instance_capacity;

	brx_pal_descriptor_set_layout *m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_buffer_descriptor_set_layout;
	brx_pal_descriptor_set *m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_buffer_descriptor_set;
	brx_pal_descriptor_set_layout *m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_texture_descriptor_set_layout;
	brx_pal_descriptor_set *m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_texture_descriptor_set;

	brx_pal_top_level_acceleration_structure *m_ray_tracing_top_level_acceleration_structure;
	brx_pal_top_level_acceleration_structure_instance_upload_buffer *m_ray_tracing_top_level_acceleration_structure_instance_upload_buffers[BRX_ANARI_FRAME_THROTTLING_COUNT];
	brx_pal_scratch_buffer *m_ray_tracing_top_level_acceleration_structure_update_scratch_buffer;
};

class brx_anari_pal_renderer final : public brx_anari_renderer
{
	brx_descriptor_set_layout *m_morphing_resource_binding_instance_update_descriptor_set_layout;
	brx_descriptor_set_layout *m_morphing_resource_binding_deformed_surface_update_descriptor_set_layout;
	brx_pipeline_layout *m_morphing_resource_binding_pipeline_layout;
	brx_compute_pipeline *m_morphing_pipeline;

	brx_descriptor_set_layout *m_skinning_resource_binding_instance_update_descriptor_set_layout;
	brx_descriptor_set_layout *m_skinning_resource_binding_deformed_surface_update_descriptor_set_layout;
	brx_pipeline_layout *m_skinning_resource_binding_pipeline_layout;
	brx_compute_pipeline *m_skinning_pipeline;

	brx_descriptor_set_layout *m_morphing_skinning_resource_binding_instance_update_descriptor_set_layout;
	brx_descriptor_set_layout *m_morphing_skinning_resource_binding_deformed_surface_update_descriptor_set_layout;
	brx_pipeline_layout *m_morphing_skinning_resource_binding_pipeline_layout;
	brx_compute_pipeline *m_morphing_skinning_pipeline;

	brx_uniform_upload_buffer *m_frame_none_update_uniform_buffer;

	BRX_COLOR_ATTACHMENT_IMAGE_FORMAT const m_rasterization_forward_shading_output_scene_color_image_format;
	BRX_COLOR_ATTACHMENT_IMAGE_FORMAT const m_rasterization_forward_shading_output_gbuffer_image_format;
	BRX_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT m_rasterization_forward_shading_output_depth_image_format;
	brx_render_pass *m_rasterization_forward_shading_render_pass;
	brx_descriptor_set *m_rasterization_forward_shading_resource_binding_none_update_descriptor_set;
	brx_pipeline_layout *m_rasterization_forward_shading_resource_binding_pipeline_layout;
	brx_graphics_pipeline *m_rasterization_forward_shading_pipeline;

	BRX_COLOR_ATTACHMENT_IMAGE_FORMAT const m_rasterization_post_process_output_color_format;
	brx_render_pass *m_rasterization_post_process_render_pass;
	brx_pipeline_layout *m_rasterization_post_process_resource_binding_pipeline_layout;
	brx_graphics_pipeline *m_rasterization_post_process_pipeline;

	brx_descriptor_set_layout *m_ray_tracing_gbuffer_resource_binding_none_update_descriptor_set_layout;
	brx_descriptor_set *m_ray_tracing_gbuffer_resource_binding_none_update_descriptor_set;
};

class brx_anari_brx_frame final : public brx_anari_frame
{
	uint32_t m_width;
	uint32_t m_height;

	brx_color_attachment_image *m_rasterization_forward_shading_output_scene_color_image;
	brx_color_attachment_image *m_rasterization_forward_shading_output_gbuffer_image;
	brx_depth_stencil_attachment_image *m_rasterization_forward_shading_depth_image;
	brx_frame_buffer *m_rasterization_forward_shading_frame_buffer;
	brx_color_attachment_image *m_rasterization_post_process_output_color_image;
	brx_frame_buffer *m_rasterization_post_process_frame_buffer;

	brx_descriptor_set_layout *m_rasterization_post_process_resource_binding_none_update_descriptor_set_layout;
	brx_descriptor_set *m_rasterization_post_process_resource_binding_none_update_descriptor_set;
};
#endif

#endif
