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

#include <cstddef>
#include <cstdint>
#include "../../Brioche-Asset-Import/include/brx_asset_import_image.h"
#include "../../Brioche-Asset-Import/include/brx_asset_import_scene.h"
#include "../../Brioche-Motion/include/brx_motion_format.h"

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

class brx_anari_device
{
public:
	virtual brx_anari_buffer *new_buffer(void *data_base, size_t data_size) = 0;
	virtual void release_buffer(brx_anari_buffer *buffer) = 0;
	virtual brx_anari_image *new_image(brx_asset_import_image *image, bool force_srgb) = 0;
	virtual void release_image(brx_anari_image *image) = 0;
	virtual void commit_buffer_and_image() = 0;
	virtual brx_anari_surface_group *new_surface_group(
		brx_asset_import_surface_group *surface_group,
		brx_anari_buffer *(pfn_get_or_create_buffer_user_callback)(brx_anari_device *device, char *in_out_id_url, uint64_t *out_id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		brx_asset_import_image *(pfn_get_or_create_image_user_callback)(brx_anari_device *device, char *in_out_id_url, uint64_t *out_id_timestamp, void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n),
		void *user_data_x, void *user_data_y, void *user_data_z, void *user_data_u, void *user_data_v, void *user_data_w, void *user_data_l, void *user_data_m, void *user_data_n) = 0;
	virtual void release_group(
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
};

class brx_anari_surface_group_instance
{
	virtual void set_visible(bool visible) = 0;
	virtual void set_transform(float scale, float rotation[4], float translation[3]) = 0;
	virtual uint32_t set_morph_target_weights(float const *weights) = 0;
	// float[(4+3)*joint_count] = {{q_x, q_y, q_z, q_w, t_x, t_y, t_z }, ...}
	virtual uint32_t set_skeleton_skin_joint_transforms(brx_motion_skeleton_joint_transform const *skeleton_skin_joint_transforms) const = 0;
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

extern "C" brx_anari_device *brx_anari_new_device(void *device);
extern "C" void brx_anari_release_device(brx_anari_device *device);

#endif
