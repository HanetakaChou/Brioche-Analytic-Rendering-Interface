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
#include "../../Environment-Lighting/include/brx_environment_lighting_sh_projection_reduce.h"
#include "../shaders/environment_lighting_skybox_resource_binding.bsli"
#include "../shaders/forward_shading_resource_binding.bsli"

brx_pal_descriptor_set *brx_anari_pal_device::create_environment_lighting_sh_projection_per_environment_lighting_descriptor_set(brx_pal_sampled_image const *const radiance, brx_pal_storage_buffer const *const irradiance_coefficients)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_environment_lighting_sh_projection_descriptor_set_layout_per_environment_lighting_update, 0U);

    brx_pal_sampled_image const *images[] = {(NULL != radiance) ? radiance : this->m_place_holder_asset_image->get_sampled_image()};
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(images) / sizeof(images[0]), NULL, NULL, NULL, NULL, images, NULL, NULL, NULL);

    assert(NULL != irradiance_coefficients);
    brx_pal_storage_buffer const *buffers[] = {irradiance_coefficients};

    this->m_device->write_descriptor_set(descriptor_set, 1U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, sizeof(buffers) / sizeof(buffers[0]), NULL, NULL, NULL, buffers, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

brx_pal_descriptor_set *brx_anari_pal_device::create_environment_lighting_skybox_per_environment_lighting_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer, brx_pal_sampled_image const *const radiance)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_environment_lighting_skybox_descriptor_set_layout_per_environment_lighting_update, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(environment_lighting_skybox_per_environment_lighting_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    brx_pal_sampled_image const *images[] = {(NULL != radiance) ? radiance : this->m_place_holder_asset_image->get_sampled_image()};
    this->m_device->write_descriptor_set(descriptor_set, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(images) / sizeof(images[0]), NULL, NULL, NULL, NULL, images, NULL, NULL, NULL);

    return descriptor_set;
}

brx_pal_descriptor_set *brx_anari_pal_device::create_forward_shading_per_environment_lighting_descriptor_set(brx_pal_uniform_upload_buffer const *const uniform_buffer, brx_pal_read_only_storage_buffer const *const irradiance_coefficients)
{
    brx_pal_descriptor_set *descriptor_set = this->m_device->create_descriptor_set(this->m_forward_shading_descriptor_set_layout_per_environment_lighting_update, 0U);

    assert(NULL != uniform_buffer);

    constexpr uint32_t const dynamic_uniform_buffer_range = sizeof(forward_shading_per_environment_lighting_update_set_uniform_buffer_binding);
    this->m_device->write_descriptor_set(descriptor_set, 0U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &uniform_buffer, &dynamic_uniform_buffer_range, NULL, NULL, NULL, NULL, NULL, NULL);

    assert(NULL != irradiance_coefficients);
    brx_pal_read_only_storage_buffer const *buffers[] = {irradiance_coefficients};

    this->m_device->write_descriptor_set(descriptor_set, 1U, BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(buffers) / sizeof(buffers[0]), NULL, NULL, buffers, NULL, NULL, NULL, NULL, NULL);

    return descriptor_set;
}

void brx_anari_pal_device::hdri_light_set_radiance(brx_anari_image *radiance)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    if (this->m_hdri_light_radiance != radiance)
    {
        assert(NULL != this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update);
        this->destroy_descriptor_set(this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update);
        this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update = NULL;

        assert(NULL != this->m_hdri_light_environment_lighting_skybox_descriptor_set_per_environment_lighting_update);
        this->destroy_descriptor_set(this->m_hdri_light_environment_lighting_skybox_descriptor_set_per_environment_lighting_update);
        this->m_hdri_light_environment_lighting_skybox_descriptor_set_per_environment_lighting_update = NULL;

        if (NULL != this->m_hdri_light_radiance)
        {
            this->release_image(this->m_hdri_light_radiance);
            this->m_hdri_light_radiance = NULL;
        }

        if (NULL != radiance)
        {
            static_cast<brx_anari_pal_image *>(radiance)->retain();
            this->m_hdri_light_radiance = radiance;
        }

        assert(NULL == this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update);
        assert(NULL != this->m_hdri_light_irradiance_coefficients);
        this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update = this->create_environment_lighting_sh_projection_per_environment_lighting_descriptor_set((NULL != this->m_hdri_light_radiance) ? static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_image()->get_sampled_image() : NULL, this->m_hdri_light_irradiance_coefficients->get_storage_buffer());

        assert(NULL == this->m_hdri_light_environment_lighting_skybox_descriptor_set_per_environment_lighting_update);
        assert(NULL != this->m_hdri_light_irradiance_coefficients);
        this->m_hdri_light_environment_lighting_skybox_descriptor_set_per_environment_lighting_update = this->create_environment_lighting_skybox_per_environment_lighting_descriptor_set(this->m_hdri_light_environment_lighting_skybox_per_environment_lighting_update_set_uniform_buffer, (NULL != this->m_hdri_light_radiance) ? static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_image()->get_sampled_image() : NULL);

        this->m_hdri_light_dirty = true;
    }
    else
    {
        assert(NULL == radiance);
    }

    if (NULL == this->m_hdri_light_radiance)
    {
        this->m_hdri_light_layout = BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED;
    }

#ifndef NDEBUG
    this->m_hdri_light_dirty_lock = false;
#endif
}

void brx_anari_pal_device::hdri_light_set_layout(BRX_ANARI_HDRI_LIGHT_LAYOUT layout)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    if (this->m_hdri_light_layout != layout)
    {
        this->m_hdri_light_layout = layout;

        this->m_hdri_light_dirty = true;
    }

#ifndef NDEBUG
    this->m_hdri_light_dirty_lock = false;
#endif
}

void brx_anari_pal_device::hdri_light_set_direction(brx_anari_vec3 direction)
{
    this->m_hdri_light_direction = direction;
}

void brx_anari_pal_device::hdri_light_set_up(brx_anari_vec3 up)
{
    this->m_hdri_light_up = up;
}

brx_anari_image *brx_anari_pal_device::hdri_light_get_radiance() const
{
    return this->m_hdri_light_radiance;
}

BRX_ANARI_HDRI_LIGHT_LAYOUT brx_anari_pal_device::hdri_light_get_layout() const
{
    return this->m_hdri_light_layout;
}

brx_anari_vec3 brx_anari_pal_device::hdri_light_get_direction() const
{
    return this->m_hdri_light_direction;
}

brx_anari_vec3 brx_anari_pal_device::hdri_light_get_up() const
{
    return this->m_hdri_light_up;
}

void brx_anari_pal_device::render_hdri_light_sh_projection(brx_pal_graphics_command_buffer *graphics_command_buffer)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    if (this->m_hdri_light_dirty)
    {
        this->m_graphics_command_buffers[this->m_frame_throttling_index]->begin_debug_utils_label("Diffuse Environment Lighting SH Projection");

        {
            brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_irradiance_coefficients->get_storage_buffer()};
            BRX_PAL_COMPUTE_PASS_STORAGE_BUFFER_LOAD_OPERATION buffer_load_operations[] = {BRX_PAL_COMPUTE_PASS_STORAGE_BUFFER_LOAD_OPERATION_DONT_CARE};
            static_assert((sizeof(buffers) / sizeof(buffers[0])) == (sizeof(buffer_load_operations) / sizeof(buffer_load_operations[0])), "");
            graphics_command_buffer->compute_pass_load(sizeof(buffers) / sizeof(buffers[0]), buffers, buffer_load_operations, 0U, NULL, NULL);
        }

        {
            graphics_command_buffer->bind_compute_pipeline(this->m_environment_lighting_sh_projection_clear_pipeline);

            {
                brx_pal_descriptor_set const *const descritor_sets[] = {this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update};
                graphics_command_buffer->bind_compute_descriptor_sets(this->m_environment_lighting_sh_projection_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, 0U, NULL);
            }

            graphics_command_buffer->dispatch(1U, 1U, 1U);
        }

        if ((NULL != this->m_hdri_light_radiance) && ((BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR == this->m_hdri_light_layout) || (BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == this->m_hdri_light_layout)))
        {
            {
                brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_irradiance_coefficients->get_storage_buffer()};
                graphics_command_buffer->compute_pass_barrier(sizeof(buffers) / sizeof(buffers[0]), buffers, 0U, NULL);
            }

            if (BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR == this->m_hdri_light_layout)
            {
                graphics_command_buffer->bind_compute_pipeline(this->m_environment_lighting_sh_projection_equirectangular_map_pipeline);
            }
            else
            {
                assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == this->m_hdri_light_layout);
                graphics_command_buffer->bind_compute_pipeline(this->m_environment_lighting_sh_projection_octahedral_map_pipeline);
            }

            {
                brx_pal_descriptor_set const *const descritor_sets[] = {this->m_hdri_light_environment_lighting_sh_projection_descriptor_set_per_environment_lighting_update};
                graphics_command_buffer->bind_compute_descriptor_sets(this->m_environment_lighting_sh_projection_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, 0U, NULL);
            }

            {
                uint32_t const tile_num_width = static_cast<uint32_t>(static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_width() + static_cast<uint32_t>(BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_X) - 1U) / static_cast<uint32_t>(BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_X);
                uint32_t const tile_num_height = static_cast<uint32_t>(static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_height() + static_cast<uint32_t>(BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_Y) - 1U) / static_cast<uint32_t>(BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_Y);

                graphics_command_buffer->dispatch(tile_num_width, tile_num_height, 1U);
            }
        }

        {
            brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_irradiance_coefficients->get_storage_buffer()};
            BRX_PAL_COMPUTE_PASS_STORAGE_BUFFER_STORE_OPERATION buffer_store_operations[] = {BRX_PAL_COMPUTE_PASS_STORAGE_BUFFER_STORE_OPERATION_FLUSH_FOR_READ_ONLY_STORAGE_BUFFER_AND_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BUFFER};
            static_assert((sizeof(buffers) / sizeof(buffers[0])) == (sizeof(buffer_store_operations) / sizeof(buffer_store_operations[0])), "");
            graphics_command_buffer->compute_pass_store(sizeof(buffers) / sizeof(buffers[0]), buffers, buffer_store_operations, 0U, NULL, NULL);
        }

        graphics_command_buffer->end_debug_utils_label();

        this->m_hdri_light_dirty = false;
    }

#ifndef NDEBUG
    this->m_hdri_light_dirty_lock = false;
#endif
}
