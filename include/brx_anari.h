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

#ifndef _BRX_ANARI_H_
#define _BRX_ANARI_H_ 1

// [Khronos ANARI: glTF To ANARI](https://github.com/KhronosGroup/ANARI-SDK/blob/next_release/src/anari_test_scenes/scenes/file/gltf2anari.h)
// [Khronos ANARI: Hydra ANARI Render Delegate](https://github.com/KhronosGroup/ANARI-SDK/blob/next_release/src/hdanari/renderDelegate.h)
// [Pixar OpenUSD: Hydra Storm Render Delegate](https://github.com/PixarAnimationStudios/OpenUSD/blob/dev/pxr/imaging/hdSt/renderDelegate.h)

#include <cstddef>
#include <cstdint>

class brx_anari_device;
class brx_anari_image;
class brx_anari_surface_group;
class brx_anari_surface_group_instance;
// class brx_anari_hdri_light;
// class brx_anari_world;
// class brx_anari_camera;
// class brx_anari_renderer;
// class brx_anari_frame;

static constexpr uint32_t const BRX_ANARI_UINT32_INDEX_INVALID = static_cast<uint32_t>(~static_cast<uint32_t>(0U));

enum BRX_ANARI_MORPH_TARGET_NAME : uint32_t
{
	// にこり
	BRX_ANARI_MORPH_TARGET_NAME_MMD_BROW_HAPPY = 0,
	// 怒り
	// 真面目
	BRX_ANARI_MORPH_TARGET_NAME_MMD_BROW_ANGRY = 1,
	// 困る
	BRX_ANARI_MORPH_TARGET_NAME_MMD_BROW_SAD = 2,
	// 上
	BRX_ANARI_MORPH_TARGET_NAME_MMD_BROW_SURPRISED = 3,
	// まばたき
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_BLINK = 4,
	// ウィンク２
	// ウィンク
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_BLINK_L = 5,
	// ｳｨﾝｸ２右
	// ウィンク右
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_BLINK_R = 6,
	// 笑い
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_HAPPY = 7,
	// ｷﾘｯ
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_ANGRY = 8,
	// じと目
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_SAD = 9,
	// びっくり
	BRX_ANARI_MORPH_TARGET_NAME_MMD_EYE_SURPRISED = 10,
	// あ
	// あ２
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_A = 11,
	// い
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_I = 12,
	// う
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_U = 13,
	// え
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_E = 14,
	// お
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_O = 15,
	// にっこり
	// にやり
	// にやり２
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_HAPPY = 16,
	// ∧
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_ANGRY = 17,
	// 口角下げ
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_SAD = 18,
	// ▲
	BRX_ANARI_MORPH_TARGET_NAME_MMD_MOUTH_SURPRISED = 19,
	//
	BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT = 20
};

enum BRX_ANARI_IMAGE_FORMAT : uint32_t
{
	BRX_ANARI_IMAGE_FORMAT_UNDEFINED = 0,
	BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_UNORM = 1,
	BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_SRGB = 2,
	BRX_ANARI_IMAGE_FORMAT_R16G16B16A16_SFLOAT = 3
};

enum BRX_ANARI_HDRI_LIGHT_LAYOUT : uint32_t
{
	BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED = 0,
	BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR = 1,
	BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL = 2,
	BRX_ANARI_HDRI_LIGHT_LAYOUT_COUNT = 3
};

enum BRX_ANARI_RENDERER_GI_QUALITY : uint32_t
{
	BRX_ANARI_RENDERER_GI_QUALITY_DISABLE = 0,
	BRX_ANARI_RENDERER_GI_QUALITY_LOW = 1,
	BRX_ANARI_RENDERER_GI_QUALITY_MEDIUM = 2,
	BRX_ANARI_RENDERER_GI_QUALITY_HIGH = 3,
	BRX_ANARI_RENDERER_GI_QUALITY_COUNT = 4
};

struct brx_anari_surface_vertex_position
{
	float m_position[3];
};

struct brx_anari_surface_vertex_varying
{
	float m_normal[3];
	float m_tangent[4];
	float m_texcoord[2];
};

struct brx_anari_surface_vertex_blending
{
	uint32_t m_indices[4];
	float m_weights[4];
};

struct brx_anari_vec3
{
	float m_x;
	float m_y;
	float m_z;
};

struct brx_anari_vec4
{
	float m_x;
	float m_y;
	float m_z;
	float m_w;
};

struct BRX_ANARI_SURFACE
{
	uint32_t m_vertex_count;
	brx_anari_surface_vertex_position const *m_vertex_positions;
	brx_anari_surface_vertex_varying const *m_vertex_varyings;
	brx_anari_surface_vertex_blending const *m_vertex_blendings;
	brx_anari_surface_vertex_position const *m_morph_targets_vertex_positions[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
	brx_anari_surface_vertex_varying const *m_morph_targets_vertex_varyings[BRX_ANARI_MORPH_TARGET_NAME_MMD_COUNT];
	uint32_t m_index_count;
	uint32_t const *m_indices;
	bool m_is_double_sided;
	brx_anari_image *m_emissive_image;
	brx_anari_vec3 m_emissive_factor;
	brx_anari_image *m_normal_image;
	float m_normal_scale;
	brx_anari_image *m_base_color_image;
	brx_anari_vec4 m_base_color_factor;
	brx_anari_image *m_metallic_roughness_image;
	float m_metallic_factor;
	float m_roughness_factor;
};

struct brx_anari_rigid_transform
{
	float m_rotation[4];
	float m_translation[3];
};

#if 1
class brx_anari_device
{
public:
	virtual brx_anari_image *new_image(BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height) = 0;
	virtual void release_image(brx_anari_image *image) = 0;

	virtual brx_anari_surface_group *new_surface_group(uint32_t surface_count, BRX_ANARI_SURFACE const *surfaces) = 0;
	virtual void release_surface_group(brx_anari_surface_group *surface_group) = 0;

	virtual brx_anari_surface_group_instance *world_new_surface_group_instance(brx_anari_surface_group *surface_group) = 0;
	virtual void world_release_surface_group_instance(brx_anari_surface_group_instance *surface_group_instance) = 0;

	// virtual void directional_light_set_direction(brx_anari_vec3 direction) = 0;
	// virtual void directional_light_set_irradiance(float irradiance) = 0;

	// virtual brx_anari_vec3 directional_light_get_direction() const = 0;
	// virtual float directional_light_get_irradiance() const = 0;

	virtual void hdri_light_set_radiance(brx_anari_image *radiance) = 0;
	virtual void hdri_light_set_layout(BRX_ANARI_HDRI_LIGHT_LAYOUT layout) = 0;
	virtual void hdri_light_set_direction(brx_anari_vec3 direction) = 0;
	virtual void hdri_light_set_up(brx_anari_vec3 up) = 0;

	virtual brx_anari_image *hdri_light_get_radiance() const = 0;
	virtual BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_get_layout() const = 0;
	virtual brx_anari_vec3 hdri_light_get_direction() const = 0;
	virtual brx_anari_vec3 hdri_light_get_up() const = 0;

	virtual void renderer_set_gi_quality(BRX_ANARI_RENDERER_GI_QUALITY gi_quality) = 0;
	virtual BRX_ANARI_RENDERER_GI_QUALITY renderer_get_gi_quality() const = 0;

	virtual void camera_set_position(brx_anari_vec3 position) = 0;
	virtual void camera_set_direction(brx_anari_vec3 direction) = 0;
	virtual void camera_set_up(brx_anari_vec3 up) = 0;
	virtual void camera_set_fovy(float fovy) = 0;
	virtual void camera_set_near(float near) = 0;
	virtual void camera_set_far(float far) = 0;

	virtual brx_anari_vec3 camera_get_position() const = 0;
	virtual brx_anari_vec3 camera_get_direction() const = 0;
	virtual brx_anari_vec3 camera_get_up() const = 0;
	virtual float camera_get_fovy() const = 0;
	virtual float camera_get_near() const = 0;
	virtual float camera_get_far() const = 0;

	virtual void frame_attach_window(void *wsi_window) = 0;
	virtual void frame_resize_window() = 0;
	virtual void frame_detach_window() = 0;

	virtual void renderer_render_frame(bool ui_view) = 0;
};

class brx_anari_image
{
};

class brx_anari_surface_group
{
};

class brx_anari_surface_group_instance
{
public:
	virtual void set_morph_target_weight(BRX_ANARI_MORPH_TARGET_NAME morph_target_name, float morph_target_weight) = 0;
	virtual void set_skin_transforms(uint32_t skin_transform_count, brx_anari_rigid_transform const *skin_transforms) = 0;
	virtual void set_model_transform(brx_anari_rigid_transform model_transform) = 0;
};

extern "C" brx_anari_device *brx_anari_new_device(void *wsi_connection);
extern "C" void brx_anari_release_device(brx_anari_device *device);
#endif

#endif
