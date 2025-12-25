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
#include "../../McRT-Malloc/include/mcrt_set.h"
#include "../../Brioche-Platform-Abstraction-Layer/include/brx_pal_device.h"
#if defined(__GNUC__)
// GCC or CLANG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <DirectXMath.h>
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
// MSVC or CLANG-CL
#include <DirectXMath.h>
#else
#error Unknown Compiler
#endif

class brx_anari_pal_device;
class brx_anari_pal_image;
class brx_anari_pal_surface_group;

// [Pipeline Throttling](https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/the-mali-gpu-an-abstract-machine-part-1---frame-pipelining)
static uint32_t constexpr const INTERNAL_FRAME_THROTTLING_COUNT = 3U;

// The unified facade for both the UI renderer and the scene renderer

class brx_anari_pal_device final : public brx_anari_device
{
	brx_pal_device *m_device;
	uint32_t m_uniform_upload_buffer_offset_alignment;

	// Scene Renderer
	brx_pal_sampler *m_shared_none_update_set_linear_wrap_sampler;
	brx_pal_sampler *m_shared_none_update_set_linear_clamp_sampler;
	// ---
	brx_pal_descriptor_set_layout *m_deforming_descriptor_set_layout_per_surface_group_update;
	brx_pal_descriptor_set_layout *m_deforming_descriptor_set_layout_per_surface_update;
	brx_pal_pipeline_layout *m_deforming_pipeline_layout;
	// ---
	brx_pal_uniform_upload_buffer *m_environment_lighting_none_update_set_uniform_buffer;
	brx_pal_descriptor_set *m_environment_lighting_descriptor_set_none_update;
	brx_pal_descriptor_set_layout *m_environment_lighting_descriptor_set_layout_per_environment_lighting_update;
	brx_pal_pipeline_layout *m_environment_lighting_pipeline_layout;
	// ---
	brx_pal_uniform_upload_buffer *m_forward_shading_none_update_set_uniform_buffer;
	brx_pal_descriptor_set_layout *m_forward_shading_descriptor_set_layout_none_update;
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_none_update;
	brx_pal_descriptor_set_layout *m_forward_shading_descriptor_set_layout_per_surface_group_update;
	brx_pal_descriptor_set_layout *m_forward_shading_descriptor_set_layout_per_surface_update;
	brx_pal_pipeline_layout *m_forward_shading_pipeline_layout;
	// ---
	brx_pal_uniform_upload_buffer *m_post_processing_none_update_set_uniform_buffer;
	brx_pal_descriptor_set_layout *m_post_processing_descriptor_set_layout_none_update;
	brx_pal_descriptor_set *m_post_processing_descriptor_set_none_update;
	brx_pal_pipeline_layout *m_post_processing_pipeline_layout;
	// ---
	brx_pal_compute_pipeline *m_deforming_pipeline;
	// ---
	brx_pal_compute_pipeline *m_environment_lighting_sh_projection_clear_pipeline;
	brx_pal_compute_pipeline *m_environment_lighting_sh_projection_equirectangular_map_pipeline;
	brx_pal_compute_pipeline *m_environment_lighting_sh_projection_octahedral_map_pipeline;
	// ---
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_direct_radiance_image_format;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_ambient_radiance_image_format;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_gbuffer_normal_image_format;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_gbuffer_base_color_image_format;
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_gbuffer_roughness_metallic_image_format;
	BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT m_scene_depth_image_format;
	brx_pal_render_pass *m_forward_shading_render_pass;
	brx_pal_graphics_pipeline *m_environment_lighting_skybox_equirectangular_map_pipeline;
	brx_pal_graphics_pipeline *m_environment_lighting_skybox_octahedral_map_pipeline;
	brx_pal_graphics_pipeline *m_forward_shading_pipeline;
	// ---
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const m_display_color_image_format;
	brx_pal_render_pass *m_post_processing_render_pass;
	brx_pal_graphics_pipeline *m_post_processing_pipeline;
	// ---
	uint32_t m_intermediate_width;
	uint32_t m_intermediate_height;
	// ---
	brx_pal_color_attachment_image *m_direct_radiance_image;
	brx_pal_color_attachment_image *m_ambient_radiance_image;
	brx_pal_color_attachment_image *m_gbuffer_normal_image;
	brx_pal_color_attachment_image *m_gbuffer_base_color_image;
	brx_pal_color_attachment_image *m_gbuffer_roughness_metallic_image;
	brx_pal_depth_stencil_attachment_image *m_scene_depth_image;
	brx_pal_frame_buffer *m_forward_shading_frame_buffer;
	// ---
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
	float m_intermediate_width_scale;
	float m_intermediate_height_scale;
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

	brx_pal_sampled_asset_image *m_lut_specular_hdr_fresnel_factors_asset_image;
	brx_pal_sampled_asset_image *m_lut_specular_transfer_function_sh_coefficients_asset_image;

	brx_pal_storage_asset_buffer *m_place_holder_asset_buffer;
	brx_pal_sampled_asset_image *m_place_holder_asset_image;
	brx_pal_storage_image *m_place_holder_storage_image;

	mcrt_unordered_map<brx_anari_surface_group const *, mcrt_set<brx_anari_surface_group_instance const *>> m_world_surface_group_instances;

	brx_anari_image *m_hdri_light_radiance;
#ifndef NDEBUG
	bool m_hdri_light_layout_lock;
#endif
	BRX_ANARI_HDRI_LIGHT_LAYOUT m_hdri_light_layout;
	brx_anari_vec3 m_hdri_light_direction;
	brx_anari_vec3 m_hdri_light_up;
#ifndef NDEBUG
	bool m_hdri_light_dirty_lock;
#endif
	bool m_hdri_light_dirty;
	brx_pal_storage_intermediate_buffer *m_hdri_light_environment_map_sh_coefficients;
	brx_pal_descriptor_set *m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update;

	brx_anari_vec3 m_camera_position;
	brx_anari_vec3 m_camera_direction;
	brx_anari_vec3 m_camera_up;
	float m_camera_fovy;
	float m_camera_near;
	float m_camera_far;

#ifndef NDEBUG
	bool m_voxel_cone_tracing_dirty_lock;
#endif
	bool m_voxel_cone_tracing_dirty;
	brx_pal_storage_image *m_voxel_cone_tracing_clipmap_mask;
	brx_pal_storage_image *m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16;
	brx_pal_storage_image *m_voxel_cone_tracing_clipmap_illumination_opacity_b16a16;
	brx_pal_storage_image *m_voxel_cone_tracing_clipmap_illumination_opacity_r16g16b16a16;
	brx_pal_storage_image *m_voxel_cone_tracing_indirect_radiance_and_ambient_occlusion;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_zero_pipeline;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_clear_pipeline;
	brx_pal_render_pass *m_voxel_cone_tracing_voxelization_render_pass;
	brx_pal_graphics_pipeline *m_voxel_cone_tracing_voxelization_pipeline;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_cone_tracing_low_pipeline;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_cone_tracing_medium_pipeline;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_cone_tracing_high_pipeline;
	brx_pal_compute_pipeline *m_voxel_cone_tracing_pack_pipeline;
	brx_pal_frame_buffer *m_voxel_cone_tracing_voxelization_frame_buffer;
#ifndef NDEBUG
	bool m_renderer_gi_quality_lock;
#endif
	BRX_ANARI_RENDERER_GI_QUALITY m_renderer_gi_quality;

public:
	brx_anari_pal_device();
	~brx_anari_pal_device();
	void init(void *wsi_connection);
	void uninit();

	void helper_destroy_upload_buffer(brx_pal_uniform_upload_buffer *const buffer);
	void helper_destroy_intermediate_buffer(brx_pal_storage_intermediate_buffer *const buffer);
	void helper_destroy_asset_buffer(brx_pal_storage_asset_buffer *const buffer);
	void helper_destroy_asset_image(brx_pal_sampled_asset_image *const image);
	void helper_destroy_descriptor_set(brx_pal_descriptor_set *descriptor_set);

	brx_pal_storage_asset_buffer *internal_create_asset_buffer(void const *const data_base, uint32_t const data_size);
	void init_image(BRX_ANARI_IMAGE_FORMAT const format, void const *const pixel_data, uint32_t const width, uint32_t const height, brx_pal_sampled_asset_image **const out_asset_image);

	inline brx_pal_storage_intermediate_buffer *create_deforming_surface_intermediate_vertex_position_buffer(uint32_t vertex_count);
	inline brx_pal_storage_intermediate_buffer *create_deforming_surface_intermediate_vertex_varying_buffer(uint32_t vertex_count);

	inline brx_pal_uniform_upload_buffer *create_deforming_per_surface_group_update_set_uniform_buffer();
	inline brx_pal_descriptor_set *create_deforming_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer);
	inline brx_pal_descriptor_set *create_deforming_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const vertex_blending_buffer, brx_pal_read_only_storage_buffer const *const *const morph_targets_vertex_position_buffers, brx_pal_read_only_storage_buffer const *const *const morph_targets_vertex_varying_buffers, brx_pal_storage_buffer const *const vertex_position_buffer_instance, brx_pal_storage_buffer const *const vertex_varying_buffer_instance);

	inline brx_pal_uniform_upload_buffer *create_forward_shading_per_surface_group_update_set_uniform_buffer();
	inline brx_pal_descriptor_set *create_forward_shading_per_surface_group_update_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer);
	inline brx_pal_descriptor_set *create_forward_shading_per_surface_update_descriptor_set(brx_pal_read_only_storage_buffer const *const vertex_position_buffer, brx_pal_read_only_storage_buffer const *const vertex_varying_buffer, brx_pal_read_only_storage_buffer const *const index_buffer, brx_pal_read_only_storage_buffer const *const auxiliary_buffer, brx_pal_sampled_image const *const emissive_image, brx_pal_sampled_image const *const normal_image, brx_pal_sampled_image const *const base_color_image, brx_pal_sampled_image const *const metallic_roughness_image);

	void release_image(brx_anari_pal_image *const image);
	void release_surface_group(brx_anari_pal_surface_group *const surface_group);

private:
	uint32_t helper_compute_uniform_buffer_size(uint32_t buffer_size) const;
	void *helper_compute_uniform_buffer_memory_address(uint32_t frame_throttling_index, brx_pal_uniform_upload_buffer const *uniform_upload_buffer, uint32_t buffer_size) const;
	uint32_t helper_compute_uniform_buffer_dynamic_offset(uint32_t frame_throttling_index, uint32_t buffer_size) const;

	template <typename T>
	inline uint32_t helper_compute_uniform_buffer_size() const
	{
		return this->helper_compute_uniform_buffer_size(static_cast<uint32_t>(sizeof(T)));
	}

	template <typename T>
	inline T *helper_compute_uniform_buffer_memory_address(uint32_t frame_throttling_index, brx_pal_uniform_upload_buffer const *uniform_upload_buffer) const
	{
		return static_cast<T *>(this->helper_compute_uniform_buffer_memory_address(frame_throttling_index, uniform_upload_buffer, static_cast<uint32_t>(sizeof(T))));
	}

	template <typename T>
	inline uint32_t helper_compute_uniform_buffer_dynamic_offset(uint32_t frame_throttling_index) const
	{
		return this->helper_compute_uniform_buffer_dynamic_offset(frame_throttling_index, static_cast<uint32_t>(sizeof(T)));
	}

	void init_lut_resource();

	void init_place_holder_resource();

	brx_anari_image *new_image(BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height) override;
	void release_image(brx_anari_image *image) override;

	brx_anari_surface_group *new_surface_group(uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces) override;
	void release_surface_group(brx_anari_surface_group *surface_group) override;

	brx_anari_surface_group_instance *world_new_surface_group_instance(brx_anari_surface_group *surface_group) override;
	void world_release_surface_group_instance(brx_anari_surface_group_instance *surface_group_instance) override;

	void hdri_light_set_radiance(brx_anari_image *radiance) override;
	void hdri_light_set_layout(BRX_ANARI_HDRI_LIGHT_LAYOUT layout) override;
	void hdri_light_set_direction(brx_anari_vec3 direction) override;
	void hdri_light_set_up(brx_anari_vec3 up) override;

	brx_anari_image *hdri_light_get_radiance() const override;
	BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_get_layout() const override;
	brx_anari_vec3 hdri_light_get_direction() const override;
	brx_anari_vec3 hdri_light_get_up() const override;

	void renderer_set_gi_quality(BRX_ANARI_RENDERER_GI_QUALITY gi_quality) override;
	BRX_ANARI_RENDERER_GI_QUALITY renderer_get_gi_quality() const override;

	void camera_set_position(brx_anari_vec3 position) override;
	void camera_set_direction(brx_anari_vec3 direction) override;
	void camera_set_up(brx_anari_vec3 up) override;
	void camera_set_fovy(float fovy) override;
	void camera_set_near(float near) override;
	void camera_set_far(float far) override;

	brx_anari_vec3 camera_get_position() const override;
	brx_anari_vec3 camera_get_direction() const override;
	brx_anari_vec3 camera_get_up() const override;
	float camera_get_fovy() const override;
	float camera_get_near() const override;
	float camera_get_far() const override;

	void frame_attach_window(void *wsi_window, float intermediate_width_scale, float intermediate_height_scale) override;
	void frame_resize_window(float intermediate_width_scale, float intermediate_height_scale) override;
	void frame_detach_window() override;

	void renderer_render_frame(bool ui_view) override;

	inline void create_swap_chain_compatible_render_pass_and_pipeline();
	inline void destroy_swap_chain_compatible_render_pass_and_pipeline();
	inline void attach_swap_chain(BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality);
	inline void detach_swap_chain(BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality);

	void hdri_light_create_none_update_binding_resource();
	void hdri_light_destroy_none_update_binding_resource();
	void hdri_light_create_none_update_descriptor();
	void hdri_light_destroy_none_update_descriptor();
	void hdri_light_create_pipeline();
	void hdri_light_destroy_pipeline();
	void hdri_light_create_per_environment_lighting_descriptor(brx_pal_sampled_image const *const radiance);
	void hdri_light_destroy_per_environment_lighting_descriptor();
	DirectX::XMFLOAT4X4 hdri_light_get_world_to_environment_map_transform();
	void hdri_light_update_uniform_buffer(uint32_t frame_throttling_index, DirectX::XMFLOAT4X4 const &inverse_view_transform, DirectX::XMFLOAT4X4 const &inverse_projection_transform, DirectX::XMFLOAT4X4 const &world_to_environment_map_transform);
	void hdri_light_render_sh_projection(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, bool &inout_hdri_light_sh_dirty, BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_layout);
	void hdri_light_render_skybox(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_layout);

	void voxel_cone_tracing_write_quality_dependent_place_holder_none_update_descriptor();
	void voxel_cone_tracing_create_quality_dependent_none_update_binding_resource();
	void voxel_cone_tracing_destroy_quality_dependent_none_update_binding_resource();
	void voxel_cone_tracing_create_screen_size_dependent_none_update_binding_resource();
	void voxel_cone_tracing_destroy_screen_size_dependent_none_update_binding_resource();
	void voxel_cone_tracing_create_pipeline();
	void voxel_cone_tracing_destroy_pipeline();
	DirectX::XMFLOAT3 voxel_cone_tracing_get_clipmap_anchor(DirectX::XMFLOAT3 const &in_eye_position, DirectX::XMFLOAT3 const &in_eye_direction);
	DirectX::XMFLOAT3 voxel_cone_tracing_get_clipmap_center(DirectX::XMFLOAT3 const &in_clipmap_anchor);
	DirectX::XMFLOAT4X4 voxel_cone_tracing_get_viewport_depth_direction_view_matrix(DirectX::XMFLOAT3 const &in_clipmap_center, uint32_t viewport_depth_direction_index);
	DirectX::XMFLOAT4X4 voxel_cone_tracing_get_clipmap_stack_level_projection_matrix(uint32_t clipmap_stack_level_index);
	void voxel_cone_tracing_render(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, bool &inout_voxel_cone_tracing_dirty, BRX_ANARI_RENDERER_GI_QUALITY renderer_gi_quality);
};

class brx_anari_pal_image final : public brx_anari_image
{
	uint32_t m_ref_count;
	brx_pal_sampled_asset_image *m_image;
	uint32_t m_width;
	uint32_t m_height;

public:
	inline brx_anari_pal_image();
	inline ~brx_anari_pal_image();
	inline void init(brx_anari_pal_device *device, BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height);
	inline void uninit(brx_anari_pal_device *device);
	void retain();
	inline uint32_t internal_release();
	brx_pal_sampled_asset_image const *get_image() const;
	uint32_t get_width() const;
	uint32_t get_height() const;
};

class brx_anari_pal_surface
{
	// uint32_t m_morph_target_count;
	// uint32_t m_skeleton_joint_count;

	uint32_t m_vertex_count;
	brx_pal_storage_asset_buffer *m_vertex_position_buffer;
	brx_pal_storage_asset_buffer *m_vertex_varying_buffer;
	brx_pal_storage_asset_buffer *m_vertex_blending_buffer;
	brx_pal_storage_asset_buffer *m_morph_targets_vertex_position_buffers[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
	brx_pal_storage_asset_buffer *m_morph_targets_vertex_varying_buffers[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
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
	uint32_t get_vertex_count() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_position_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_varying_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_vertex_blending_buffer() const;
	brx_pal_storage_asset_buffer const *const *get_morph_targets_vertex_position_buffers() const;
	brx_pal_storage_asset_buffer const *const *get_morph_targets_vertex_varying_buffers() const;
	uint32_t get_index_count() const;
	inline brx_pal_storage_asset_buffer const *get_index_buffer() const;
	inline brx_pal_storage_asset_buffer const *get_auxiliary_buffer() const;
	inline brx_anari_pal_image const *get_emissive_image() const;
	inline brx_anari_pal_image const *get_normal_image() const;
	inline brx_anari_pal_image const *get_base_color_image() const;
	inline brx_anari_pal_image const *get_metallic_roughness_image() const;
	bool get_deforming() const;
	brx_pal_descriptor_set const *get_forward_shading_per_surface_update_descriptor_set() const;
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
	uint32_t get_surface_count() const;
	brx_anari_pal_surface const *get_surfaces() const;
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
	brx_pal_storage_intermediate_buffer const *get_vertex_position_buffer() const;
	brx_pal_storage_intermediate_buffer const *get_vertex_varying_buffer() const;
	brx_pal_descriptor_set const *get_deforming_per_surface_update_descriptor_set() const;
	brx_pal_descriptor_set const *get_forward_shading_per_surface_update_descriptor_set() const;
#ifndef NDEBUG
	bool get_deforming() const;
#endif
};

class brx_anari_pal_surface_group_instance final : public brx_anari_surface_group_instance
{
	brx_anari_pal_surface_group *m_surface_group;

	mcrt_vector<brx_anari_pal_surface_instance> m_surfaces;

	// available when morph or skin
	brx_pal_uniform_upload_buffer *m_deforming_per_surface_group_update_set_uniform_buffer;
	brx_pal_descriptor_set *m_deforming_descriptor_set_per_surface_group_update;

	brx_pal_uniform_upload_buffer *m_forward_shading_per_surface_group_update_set_uniform_buffer;
	brx_pal_descriptor_set *m_forward_shading_descriptor_set_per_surface_group_update;

	// brx_pal_intermediate_bottom_level_acceleration_structure *m_ray_tracing_intermediate_bottom_level_acceleration_structure;
	// brx_pal_scratch_buffer *m_ray_tracing_intermediate_bottom_level_acceleration_structure_update_scratch_buffer;

	float m_morph_target_weights[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
	mcrt_vector<brx_anari_rigid_transform> m_skin_transforms;
	brx_anari_rigid_transform m_model_transform;

public:
	inline brx_anari_pal_surface_group_instance();
	inline ~brx_anari_pal_surface_group_instance();
	inline void init(brx_anari_pal_device *device, brx_anari_pal_surface_group *surface_group);
	inline void uninit(brx_anari_pal_device *device);
	brx_anari_pal_surface_group const *get_surface_group() const;
	brx_pal_uniform_upload_buffer const *get_deforming_per_surface_group_update_set_uniform_buffer() const;
	brx_pal_descriptor_set const *get_deforming_per_surface_group_update_descriptor_set() const;
	brx_pal_uniform_upload_buffer const *get_forward_shading_per_surface_group_update_set_uniform_buffer() const;
	brx_pal_descriptor_set const *get_forward_shading_per_surface_group_update_descriptor_set() const;
	inline uint32_t get_surface_count() const;
	brx_anari_pal_surface_instance const *get_surfaces() const;
	float get_morph_target_weight(BRX_ANARI_MORPH_TARGET_NAME morph_target_name) const;
	uint32_t get_skin_transform_count() const;

	brx_anari_rigid_transform const *get_skin_transforms() const;
	brx_anari_rigid_transform get_model_transform() const;

private:
	void set_morph_target_weight(BRX_ANARI_MORPH_TARGET_NAME morph_target_name, float morph_target_weight) override;
	void set_skin_transforms(uint32_t skin_transform_count, brx_anari_rigid_transform const *skin_transforms) override;
	void set_model_transform(brx_anari_rigid_transform model_transform) override;
};

#endif
