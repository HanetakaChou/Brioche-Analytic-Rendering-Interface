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

#ifndef _BRX_ANARI_BRX_DEVICE_H_
#define _BRX_ANARI_BRX_DEVICE_H_ 1

#include "../include/brx_anari.h"
#include "../../Brioche/include/brx_device.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_unordered_map.h"
#include "../../McRT-Malloc//include/mcrt_string.h"

// [Pipeline Throttling](https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/the-mali-gpu-an-abstract-machine-part-1---frame-pipelining)
static uint32_t constexpr const BRX_ANARI_FRAME_THROTTLING_COUNT = 3U;

// The unified facade for both the UI renderer and the scene renderer

class brx_anari_brx_device final : public brx_anari_device
{
	brx_device *m_device;
	brx_upload_queue *m_upload_queue;
	brx_graphics_queue *m_graphics_queue;

	brx_upload_command_buffer *m_asset_import_upload_command_buffer;
	brx_graphics_command_buffer *m_asset_import_graphics_command_buffer;
	brx_fence *m_asset_import_fence;
	uint32_t m_staging_upload_buffer_offset_alignment;
	uint32_t m_staging_upload_buffer_row_pitch_alignment;
	mcrt_vector<brx_storage_asset_buffer *> m_pending_commit_storage_asset_buffers;
	mcrt_vector<BRX_SAMPLED_ASSET_IMAGE_SUBRESOURCE> m_pending_commit_sampled_asset_image_subresources;
	mcrt_vector<brx_staging_upload_buffer *> m_pending_commit_staging_upload_buffers;

	// mcrt_unordered_map<mcrt_string, std::pair<brx_sampled_asset_image *, uint32_t>> m_imported_sampled_asset_images;
	// brx_sampled_asset_image* m_place_holder_texture;

	// brx_sampler* m_point_sampler;
	// brx_sampler* m_linear_sampler;

	uint32_t m_frame_throttling_index;
	mcrt_vector<brx_storage_asset_buffer *> m_pending_release_storage_asset_buffers[BRX_ANARI_FRAME_THROTTLING_COUNT];
	mcrt_vector<brx_sampled_asset_image *> m_pending_release_sampled_asset_images[BRX_ANARI_FRAME_THROTTLING_COUNT];

public:
	brx_anari_brx_device();
	~brx_anari_brx_device();
	void init(brx_device *device);
	void uninit();

private:
	brx_anari_image *new_image(brx_asset_import_image *image, bool force_srgb) override;
	void release_image(brx_anari_image *image) override;
	void commit_buffer_and_image() override;
	brx_anari_group* new_group(
		brx_asset_import_group* group,
		brx_asset_import_image* (pfn_get_or_create_buffer_user_callback)(brx_anari_device* device, char* url, void* user_data_x, void* user_data_y, void* user_data_z, void* user_data_w),
		brx_asset_import_image* (pfn_get_or_create_image_user_callback)(brx_anari_device* device, char* url, void* user_data_x, void* user_data_y, void* user_data_z, void* user_data_w),
		void* user_data_x, void* user_data_y, void* user_data_z, void* user_data_u) override;
	void release_group(brx_anari_group *group) override;
	brx_anari_instance *new_instance() override;
	void release_instance(brx_anari_instance *instance) override;
	brx_anari_world *new_world() override;
	void release_world(brx_anari_world *world) override;
	brx_anari_camera *new_camera() override;
	void release_camera(brx_anari_camera *camera) override;
	brx_anari_renderer *new_renderer() override;
	void release_renderer(brx_anari_renderer *renderer) override;
	brx_anari_frame *new_frame() override;
	void release_frame(brx_anari_frame *frame) override;
	void render_frame(brx_anari_frame *frame) override;
};

class brx_anari_brx_surface
{
	uint32_t m_morph_target_count;
	uint32_t m_skeleton_joint_count;
	uint32_t m_vertex_count;
	uint32_t m_index_count;
	mcrt_string *m_vertex_position_buffer_id_url;
	mcrt_string *m_vertex_varying_buffer_id_url;
	mcrt_string *m_vertex_joint_buffer_id_url;
	mcrt_string *m_index_buffer_id_url;
	mcrt_string *m_auxiliary_buffer_id_url;
	mcrt_string *m_base_color_texture_name_id_url;
	mcrt_string *m_metallic_roughness_texture_name_id_url;
	mcrt_string *m_normal_texture_name_id_url;
	mcrt_string *m_emissive_texture_name_id_url;
	// available when no morph and no skin
	brx_descriptor_set *m_rasterization_forward_shading_resource_binding_surface_update_descriptor_set;
};

class brx_anari_brx_surface_group
{
	mcrt_vector<brx_anari_brx_surface *> m_surfaces;
	// available when no morph and no skin
	brx_compacted_bottom_level_acceleration_structure *m_ray_tracing_compacted_bottom_level_acceleration_structure;
};

class brx_anari_brx_surface_instance
{
	brx_anari_brx_surface* m_surface;
	brx_storage_intermediate_buffer *m_deformed_vertex_position_buffer;
	brx_storage_intermediate_buffer *m_deformed_vertex_varying_buffer;
	brx_descriptor_set *m_deforming_resource_binding_deformed_surface_update_descriptor_set;
};

class brx_anari_brx_surface_group_instance final : brx_anari_surface_group_instance
{
	float m_transform[4][4];
	mcrt_vector<float> m_morph_target_weights;
	mcrt_vector<float> m_skeleton_joint_transforms;
	uint32_t m_vertex_count;
	uint32_t m_index_count;
	mcrt_vector<brx_anari_brx_surface_instance *> m_surface_instances;
	// available when morph or skin
	brx_descriptor_set *m_deforming_pipeline_instance_update_descriptor_set;
	brx_intermediate_bottom_level_acceleration_structure *m_ray_tracing_intermediate_bottom_level_acceleration_structure;
	brx_scratch_buffer *m_ray_tracing_intermediate_bottom_level_acceleration_structure_update_scratch_buffer;
};

class brx_anari_brx_camera final : public brx_anari_camera
{
};

class brx_anari_brx_world final : public brx_anari_world
{
	uint32_t m_surface_count;

	uint32_t m_surface_instance_count;

	BRX_ANARI_RENDERER_TYPE m_render_type;

	uint32_t m_ray_tracing_geometry_capaticy;

	uint32_t m_ray_tracing_instance_capacity;

	brx_descriptor_set_layout* m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_buffer_descriptor_set_layout;
	brx_descriptor_set* m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_buffer_descriptor_set;
	brx_descriptor_set_layout* m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_texture_descriptor_set_layout;
	brx_descriptor_set* m_ray_tracing_gbuffer_resource_binding_none_update_unbounded_texture_descriptor_set;

	brx_top_level_acceleration_structure* m_ray_tracing_top_level_acceleration_structure;
	brx_top_level_acceleration_structure_instance_upload_buffer* m_ray_tracing_top_level_acceleration_structure_instance_upload_buffers[BRX_ANARI_FRAME_THROTTLING_COUNT];
	brx_scratch_buffer* m_ray_tracing_top_level_acceleration_structure_update_scratch_buffer;
};

class brx_anari_brx_renderer final : public brx_anari_renderer
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
