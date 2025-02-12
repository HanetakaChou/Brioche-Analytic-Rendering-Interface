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

static constexpr uint32_t const BRX_ANARI_UINT32_INDEX_INVALID = static_cast<uint32_t>(~static_cast<uint32_t>(0U));

class brx_anari_device;
class brx_anari_buffer;
class brx_anari_image;
class brx_anari_surface_group;
class brx_anari_surface_group_instance;
class brx_anari_hdri_light;
class brx_anari_world;
class brx_anari_camera;
class brx_anari_renderer;
class brx_anari_frame;

enum BRX_ANARI_BUFFER_FORMAT
{
	BRX_ANARI_BUFFER_FORMAT_UNDEFINED = 0,
	BRX_ANARI_BUFFER_FORMAT_VERTEX_POSITION = 1,
	BRX_ANARI_BUFFER_FORMAT_VERTEX_VARYING = 2,
	BRX_ANARI_BUFFER_FORMAT_VERTEX_BLENDING = 3
};

enum BRX_ANARI_IMAGE_FORMAT
{
	BRX_ANARI_IMAGE_FORMAT_UNDEFINED = 0,
	BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_UNORM = 1,
	BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_SRGB = 2
};

enum BRX_ANARI_PBR_TEXTURE_NAME
{
	BRX_ANARI_TEXTURE_NAME_PBR_BASE_COLOR = 0,
	BRX_ANARI_TEXTURE_NAME_PBR_ROUGHNESS_METALLIC = 1,
	BRX_ANARI_TEXTURE_NAME_PBR_NORMAL = 2,
	BRX_ANARI_TEXTURE_NAME_PBR_EMISSIVE = 3,
	BRX_ANARI_TEXTURE_NAME_PBR_COUNT = 4
};

enum BRX_ANARI_HDRI_LIGHT_LAYOUT
{
	BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED = 0,
	BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR = 1,
	BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL = 2
};

enum BRX_ANARI_RENDERER_TYPE
{
	BRX_ANARI_RENDERER_TYPE_UNDEFINED = 0,
	BRX_ANARI_RENDERER_TYPE_RASTERIZATION = 1,
	BRX_ANARI_RENDERER_TYPE_RAY_TRACING = 2
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
	virtual brx_anari_buffer *new_buffer(BRX_ANARI_BUFFER_FORMAT format, void *data_base, uint32_t data_size) = 0;
	virtual void release_buffer(brx_anari_buffer *buffer) = 0;
	virtual brx_anari_image *new_image(BRX_ANARI_IMAGE_FORMAT format, void *pixel_data, uint32_t width, uint32_t height) = 0;
	virtual void release_image(brx_anari_image *image) = 0;

	virtual void frame_attach_window(void *wsi_window) = 0;
	virtual void frame_resize_window() = 0;
	virtual void frame_detach_window() = 0;

	// renderer_set_type()
	virtual void renderer_render_frame() = 0;
#if 0
	virtual void commit_buffer_and_image() = 0;
	virtual brx_anari_surface_group *new_surface_group(
		brx_asset_import_surface_group *surface_group,
		brx_anari_buffer *(pfn_get_or_create_buffer_user_callback)(brx_anari_device * device, char *in_out_id_url, uint64_t *out_id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		brx_asset_import_image *(pfn_get_or_create_image_user_callback)(brx_anari_device * device, char *in_out_id_url, uint64_t *out_id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n) = 0;
	virtual void release_surface_group(
		brx_anari_surface_group *surface_group,
		void (*pfn_get_or_create_buffer_user_callback)(brx_anari_device *device, char const *id_url, uint64_t id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		void (*pfn_get_or_create_image_user_callback)(brx_anari_device *device, char const *id_url, uint64_t id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n) = 0;
	virtual brx_anari_world *new_world() = 0;
	virtual void release_world(brx_anari_world *world) = 0;
	virtual brx_anari_surface_group_instance *new_surface_group_instance(brx_anari_world *world, brx_anari_surface_group *group) = 0;
	virtual void release_surface_group_instance(brx_anari_world *world, brx_anari_surface_group_instance *surface_group_instance) = 0;
	virtual brx_anari_hdri_light *new_hdri_light(brx_anari_world *world, brx_anari_image *image, BRX_ANARI_HDRI_LIGHT_LAYOUT layout) = 0;
	virtual void release_hdri_light(brx_anari_world *world, brx_anari_hdri_light *hdri_light) = 0;
	virtual brx_anari_camera *new_camera() = 0;
	virtual void release_camera(brx_anari_camera *camera) = 0;
	virtual brx_anari_renderer *new_renderer() = 0;
	virtual void release_renderer(brx_anari_renderer *renderer) = 0;
	virtual brx_anari_frame *new_frame() = 0;
	virtual void release_frame(brx_anari_frame *frame) = 0;
	virtual void render_frame(brx_anari_frame *frame) = 0;
#endif
};

class brx_anari_image
{
};

class brx_anari_surface_group_instance
{
	virtual void set_visible(bool visible) = 0;
	virtual void set_model_transform(brx_anari_rigid_transform const *model_transform) = 0;
	virtual uint32_t set_morph_target_weights(uint32_t morph_target_weight_count, float const *morph_target_weights) = 0;
	virtual uint32_t set_skin_transforms(uint32_t skin_transform_count, brx_anari_rigid_transform const *skin_transforms) = 0;
};

class brx_anari_hdri_light
{
	virtual void set_visible(bool visible) = 0;
	//  the environment map space (+Z Up; +X Front) may be different from the scene, the normal (N) should be properly transformed before use
	virtual void set_up(float const *up) const = 0;
	virtual void set_direction(float const *direction) const = 0;
	virtual void set_scale(float scale) const = 0;
};

class brx_anari_world
{
	// float[2*3] = {min_x, min_y, min_z, max_x, max_y, max_z}
	virtual float const *get_bounds() const = 0;
};

class brx_anari_camera
{
	virtual void set_position(float position[3]) = 0;
	virtual void set_direction(float direction[3]) = 0;
	virtual void set_up(float up[3]) = 0;
	virtual void set_fovy(float fovy) = 0;
	virtual void set_aspect(float aspect) = 0;
	virtual void set_near(float near) = 0;
	virtual void set_far(float far) = 0;
};

class brx_anari_renderer
{
	virtual void set_type(BRX_ANARI_RENDERER_TYPE type) = 0;
	// virtual BRX_ANARI_RENDERER_TYPE get_type() const = 0;
};

class brx_anari_frame
{
	// virtual void set_frame_camera(brx_anari_camera *camera) = 0;
	// virtual void set_frame_world(brx_anari_world *world) = 0;
	// virtual void set_frame_renderer(brx_anari_renderer *renderer) = 0;
};

extern "C" brx_anari_device *brx_anari_new_device(void *wsi_connection);
extern "C" void brx_anari_release_device(brx_anari_device *device);
#endif

#endif
