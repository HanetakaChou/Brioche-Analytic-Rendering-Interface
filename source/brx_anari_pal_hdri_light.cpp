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
#include "../../Spherical-Harmonic/include/brx_spherical_harmonic.h"
#include "../../Spherical-Harmonic/include/brx_spherical_harmonic_projection_environment_map_reduce.h"
#include "../shaders/environment_lighting_resource_binding.bsli"
#include "../shaders/forward_shading_resource_binding.bsli"

void brx_anari_pal_device::hdri_light_create_none_update_binding_resource()
{
    assert(NULL == this->m_hdri_light_environment_map_sh_coefficients);
    this->m_hdri_light_environment_map_sh_coefficients = this->m_device->create_storage_intermediate_buffer(sizeof(float) * (3U * BRX_SH_COEFFICIENT_COUNT));

    assert(NULL == this->m_environment_lighting_none_update_set_uniform_buffer);
    this->m_environment_lighting_none_update_set_uniform_buffer = this->m_device->create_uniform_upload_buffer(this->helper_compute_uniform_buffer_size<environment_lighting_none_update_set_uniform_buffer_binding>());
}

void brx_anari_pal_device::hdri_light_destroy_none_update_binding_resource()
{
    assert(NULL != this->m_hdri_light_environment_map_sh_coefficients);
    this->helper_destroy_intermediate_buffer(this->m_hdri_light_environment_map_sh_coefficients);
    this->m_hdri_light_environment_map_sh_coefficients = NULL;

    assert(NULL != this->m_environment_lighting_none_update_set_uniform_buffer);
    this->m_device->destroy_uniform_upload_buffer(this->m_environment_lighting_none_update_set_uniform_buffer);
    this->m_environment_lighting_none_update_set_uniform_buffer = NULL;
}

void brx_anari_pal_device::hdri_light_create_none_update_descriptor()
{
    brx_pal_descriptor_set_layout *environment_lighting_descriptor_set_layout_none_update = NULL;
    {
        assert(NULL == environment_lighting_descriptor_set_layout_none_update);
        BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const environment_lighting_descriptor_set_layout_none_update_bindings[] = {
            {0U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U},
            {1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 1U},
            {2U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
        environment_lighting_descriptor_set_layout_none_update = this->m_device->create_descriptor_set_layout(sizeof(environment_lighting_descriptor_set_layout_none_update_bindings) / sizeof(environment_lighting_descriptor_set_layout_none_update_bindings[0]), environment_lighting_descriptor_set_layout_none_update_bindings);
    }

    {
        assert(NULL == this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update);
        BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const environment_lighting_descriptor_set_layout_per_environment_lighting_update_bindings[] = {
            {1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
        this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update = this->m_device->create_descriptor_set_layout(sizeof(environment_lighting_descriptor_set_layout_per_environment_lighting_update_bindings) / sizeof(environment_lighting_descriptor_set_layout_per_environment_lighting_update_bindings[0]), environment_lighting_descriptor_set_layout_per_environment_lighting_update_bindings);
    }

    {
        assert(NULL == this->m_environment_lighting_pipeline_layout);
        brx_pal_descriptor_set_layout *const environment_lighting_pipeline_descriptor_set_layouts[] = {
            environment_lighting_descriptor_set_layout_none_update,
            this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update};
        this->m_environment_lighting_pipeline_layout = this->m_device->create_pipeline_layout(sizeof(environment_lighting_pipeline_descriptor_set_layouts) / sizeof(environment_lighting_pipeline_descriptor_set_layouts[0]), environment_lighting_pipeline_descriptor_set_layouts);
    }

    {
        assert(NULL == this->m_environment_lighting_descriptor_set_none_update);
        this->m_environment_lighting_descriptor_set_none_update = this->m_device->create_descriptor_set(environment_lighting_descriptor_set_layout_none_update, 0U);
    }

    {
        {
            assert(NULL != this->m_hdri_light_environment_map_sh_coefficients);
            brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_storage_buffer()};
            this->m_device->write_descriptor_set(this->m_environment_lighting_descriptor_set_none_update, 0U, BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, sizeof(buffers) / sizeof(buffers[0]), NULL, NULL, NULL, buffers, NULL, NULL, NULL, NULL);
        }

        {
            assert(NULL != this->m_shared_none_update_set_linear_wrap_sampler);
            this->m_device->write_descriptor_set(this->m_environment_lighting_descriptor_set_none_update, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_shared_none_update_set_linear_wrap_sampler, NULL);
        }

        {
            assert(NULL != this->m_environment_lighting_none_update_set_uniform_buffer);
            constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(environment_lighting_none_update_set_uniform_buffer_binding);
            this->m_device->write_descriptor_set(this->m_environment_lighting_descriptor_set_none_update, 2U, BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_environment_lighting_none_update_set_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
        }
    }

    this->m_device->destroy_descriptor_set_layout(environment_lighting_descriptor_set_layout_none_update);
    environment_lighting_descriptor_set_layout_none_update = NULL;
}

void brx_anari_pal_device::hdri_light_destroy_none_update_descriptor()
{
    assert(NULL != this->m_environment_lighting_descriptor_set_none_update);
    this->m_device->destroy_descriptor_set(this->m_environment_lighting_descriptor_set_none_update);
    this->m_environment_lighting_descriptor_set_none_update = NULL;

    assert(NULL != this->m_environment_lighting_pipeline_layout);
    this->m_device->destroy_pipeline_layout(this->m_environment_lighting_pipeline_layout);
    this->m_environment_lighting_pipeline_layout = NULL;

    assert(NULL != this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update);
    this->m_device->destroy_descriptor_set_layout(this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update);
    this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update = NULL;
}

void brx_anari_pal_device::hdri_light_create_pipeline()
{
    {
        assert(NULL == this->m_environment_lighting_sh_projection_clear_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_environment_map_clear_compute.inl"
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/environment_lighting_sh_projection_environment_map_clear_compute.inl"
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_environment_map_clear_compute.inl"
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
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

        assert(NULL == this->m_environment_lighting_sh_projection_equirectangular_map_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
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

        assert(NULL == this->m_environment_lighting_sh_projection_octahedral_map_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_octahedral_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/environment_lighting_sh_projection_octahedral_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_octahedral_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
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

    assert(NULL != this->m_forward_shading_render_pass);

    {
        assert(NULL == this->m_environment_lighting_skybox_equirectangular_map_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_equirectangular_map_fragment.inl"
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/environment_lighting_skybox_vertex.inl"
#include "../shaders/dxil/environment_lighting_skybox_equirectangular_map_fragment.inl"
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_equirectangular_map_fragment.inl"
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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

    {
        assert(NULL == this->m_environment_lighting_skybox_octahedral_map_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_octahedral_map_fragment.inl"
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/environment_lighting_skybox_vertex.inl"
#include "../shaders/dxil/environment_lighting_skybox_octahedral_map_fragment.inl"
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_octahedral_map_fragment.inl"
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_environment_lighting_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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

void brx_anari_pal_device::hdri_light_destroy_pipeline()
{
    assert(NULL != this->m_environment_lighting_sh_projection_clear_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_environment_lighting_sh_projection_clear_pipeline);
    this->m_environment_lighting_sh_projection_clear_pipeline = NULL;

    assert(NULL != this->m_environment_lighting_sh_projection_equirectangular_map_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_environment_lighting_sh_projection_equirectangular_map_pipeline);
    this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = NULL;

    assert(NULL != this->m_environment_lighting_sh_projection_octahedral_map_pipeline);
    this->m_device->destroy_compute_pipeline(this->m_environment_lighting_sh_projection_octahedral_map_pipeline);
    this->m_environment_lighting_sh_projection_octahedral_map_pipeline = NULL;

    assert(NULL != this->m_environment_lighting_skybox_equirectangular_map_pipeline);
    this->m_device->destroy_graphics_pipeline(this->m_environment_lighting_skybox_equirectangular_map_pipeline);
    this->m_environment_lighting_skybox_equirectangular_map_pipeline = NULL;

    assert(NULL != this->m_environment_lighting_skybox_octahedral_map_pipeline);
    this->m_device->destroy_graphics_pipeline(this->m_environment_lighting_skybox_octahedral_map_pipeline);
    this->m_environment_lighting_skybox_octahedral_map_pipeline = NULL;
}

void brx_anari_pal_device::hdri_light_create_per_environment_lighting_descriptor(brx_pal_sampled_image const *const radiance)
{
    assert(NULL != this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update);

    {
        assert(NULL == this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update);
        this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update = this->m_device->create_descriptor_set(this->m_environment_lighting_descriptor_set_layout_per_environment_lighting_update, 0U);
    }

    {
        assert((NULL != radiance));
        brx_pal_sampled_image const *images[] = {radiance};
        this->m_device->write_descriptor_set(this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update, 1U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(images) / sizeof(images[0]), NULL, NULL, NULL, NULL, images, NULL, NULL, NULL);
    }
}

void brx_anari_pal_device::hdri_light_destroy_per_environment_lighting_descriptor()
{
    assert(NULL != this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update);
    this->helper_destroy_descriptor_set(this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update);
    this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update = NULL;
}

void brx_anari_pal_device::hdri_light_set_radiance(brx_anari_image *radiance)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_layout_lock);
    this->m_hdri_light_layout_lock = true;

    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    if (this->m_hdri_light_radiance != radiance)
    {
        this->hdri_light_destroy_per_environment_lighting_descriptor();

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

        assert(NULL != this->m_place_holder_asset_image);
        this->hdri_light_create_per_environment_lighting_descriptor((NULL != this->m_hdri_light_radiance) ? static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_image()->get_sampled_image() : this->m_place_holder_asset_image->get_sampled_image());

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

    this->m_hdri_light_layout_lock = false;
#endif
}

void brx_anari_pal_device::hdri_light_set_layout(BRX_ANARI_HDRI_LIGHT_LAYOUT layout)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_layout_lock);
    this->m_hdri_light_layout_lock = true;

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

    this->m_hdri_light_layout_lock = false;
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

DirectX::XMFLOAT4X4 brx_anari_pal_device::hdri_light_get_world_to_environment_map_transform()
{
    DirectX::XMFLOAT4X4 world_to_environment_map_transform;
    {
        DirectX::XMFLOAT4X4 environment_map_to_world_transform;
        {
            // Up 0 0 1
            // Forward 1 0 0
            // Left 0 1 0

            DirectX::XMFLOAT3 environment_map_left;
            DirectX::XMFLOAT3 environment_map_up;
            DirectX::XMFLOAT3 environment_map_direction;
            {
                DirectX::XMFLOAT3 const raw_environment_map_up(this->m_hdri_light_up.m_x, this->m_hdri_light_up.m_y, this->m_hdri_light_up.m_z);
                DirectX::XMFLOAT3 const raw_environment_map_direction(this->m_hdri_light_direction.m_x, this->m_hdri_light_direction.m_y, this->m_hdri_light_direction.m_z);

                DirectX::XMVECTOR simd_environment_map_up = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_environment_map_up));
                DirectX::XMVECTOR simd_environment_map_direction = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&raw_environment_map_direction));

                DirectX::XMStoreFloat3(&environment_map_left, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(simd_environment_map_up, simd_environment_map_direction)));

                DirectX::XMStoreFloat3(&environment_map_up, simd_environment_map_up);
                DirectX::XMStoreFloat3(&environment_map_direction, simd_environment_map_direction);
            }

            environment_map_to_world_transform.m[0][0] = this->m_hdri_light_direction.m_x;
            environment_map_to_world_transform.m[0][1] = this->m_hdri_light_direction.m_y;
            environment_map_to_world_transform.m[0][2] = this->m_hdri_light_direction.m_z;
            environment_map_to_world_transform.m[0][3] = 0.0F;
            environment_map_to_world_transform.m[1][0] = environment_map_left.x;
            environment_map_to_world_transform.m[1][1] = environment_map_left.y;
            environment_map_to_world_transform.m[1][2] = environment_map_left.z;
            environment_map_to_world_transform.m[1][3] = 0.0F;
            environment_map_to_world_transform.m[2][0] = this->m_hdri_light_up.m_x;
            environment_map_to_world_transform.m[2][1] = this->m_hdri_light_up.m_y;
            environment_map_to_world_transform.m[2][2] = this->m_hdri_light_up.m_z;
            environment_map_to_world_transform.m[2][3] = 0.0F;
            environment_map_to_world_transform.m[3][0] = 0.0F;
            environment_map_to_world_transform.m[3][1] = 0.0F;
            environment_map_to_world_transform.m[3][2] = 0.0F;
            environment_map_to_world_transform.m[3][3] = 1.0F;
        }

        {
            DirectX::XMVECTOR unused_determinant;
            DirectX::XMStoreFloat4x4(&world_to_environment_map_transform, DirectX::XMMatrixInverse(&unused_determinant, DirectX::XMLoadFloat4x4(&environment_map_to_world_transform)));
        }
    }

    return world_to_environment_map_transform;
}

void brx_anari_pal_device::hdri_light_update_uniform_buffer(uint32_t frame_throttling_index, DirectX::XMFLOAT4X4 const &inverse_view_transform, DirectX::XMFLOAT4X4 const &inverse_projection_transform, DirectX::XMFLOAT4X4 const &world_to_environment_map_transform)
{
    environment_lighting_none_update_set_uniform_buffer_binding *const environment_lighting_none_update_set_uniform_buffer_destination = this->helper_compute_uniform_buffer_memory_address<environment_lighting_none_update_set_uniform_buffer_binding>(frame_throttling_index, this->m_environment_lighting_none_update_set_uniform_buffer);
    environment_lighting_none_update_set_uniform_buffer_destination->g_inverse_view_transform = inverse_view_transform;
    environment_lighting_none_update_set_uniform_buffer_destination->g_inverse_projection_transform = inverse_projection_transform;
    environment_lighting_none_update_set_uniform_buffer_destination->g_world_to_environment_map_transform = world_to_environment_map_transform;
}

void brx_anari_pal_device::hdri_light_render_sh_projection(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, bool &inout_hdri_light_sh_dirty, BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_layout)
{
    if (inout_hdri_light_sh_dirty)
    {
        graphics_command_buffer->begin_debug_utils_label("Environment Lighting SH Projection");

        {
            brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_storage_buffer()};
            graphics_command_buffer->storage_resource_load_dont_care(sizeof(buffers) / sizeof(buffers[0]), buffers, 0U, NULL);
        }

        {
            graphics_command_buffer->bind_compute_pipeline(this->m_environment_lighting_sh_projection_clear_pipeline);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {
                    this->m_environment_lighting_descriptor_set_none_update,
                    this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<environment_lighting_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_environment_lighting_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            graphics_command_buffer->dispatch(1U, 1U, 1U);
        }

        if ((NULL != this->m_hdri_light_radiance) && ((BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR == hdri_light_layout) || (BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == hdri_light_layout)))
        {
            {
                brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_storage_buffer()};
                graphics_command_buffer->storage_resource_barrier(sizeof(buffers) / sizeof(buffers[0]), buffers, 0U, NULL);
            }

            {
                brx_pal_compute_pipeline *environment_lighting_sh_projection_pipeline;
                switch (hdri_light_layout)
                {
                case BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR:
                {
                    environment_lighting_sh_projection_pipeline = this->m_environment_lighting_sh_projection_equirectangular_map_pipeline;
                }
                break;
                default:
                {
                    assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == hdri_light_layout);
                    environment_lighting_sh_projection_pipeline = this->m_environment_lighting_sh_projection_octahedral_map_pipeline;
                }
                }

                graphics_command_buffer->bind_compute_pipeline(environment_lighting_sh_projection_pipeline);
            }

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {
                    this->m_environment_lighting_descriptor_set_none_update,
                    this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<environment_lighting_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_environment_lighting_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            {
                DirectX::XMUINT2 hdri_light_radiance_size(static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_width(), static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_height());
                DirectX::XMUINT3 sh_projection_environment_map_dispatch_extent = brx_sh_projection_environment_map_dispatch_extent(hdri_light_radiance_size);

                graphics_command_buffer->dispatch(sh_projection_environment_map_dispatch_extent.x, sh_projection_environment_map_dispatch_extent.y, sh_projection_environment_map_dispatch_extent.z);
            }
        }

        {
            brx_pal_storage_buffer const *buffers[] = {this->m_hdri_light_environment_map_sh_coefficients->get_storage_buffer()};
            graphics_command_buffer->storage_resource_store(sizeof(buffers) / sizeof(buffers[0]), buffers, 0U, NULL);
        }

        graphics_command_buffer->end_debug_utils_label();

        inout_hdri_light_sh_dirty = false;
    }
}

void brx_anari_pal_device::hdri_light_render_skybox(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer, BRX_ANARI_HDRI_LIGHT_LAYOUT hdri_light_layout)
{
    if ((BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR == hdri_light_layout) || (BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == hdri_light_layout))
    {
        {
            brx_pal_graphics_pipeline *environment_lighting_skybox_pipeline;
            switch (hdri_light_layout)
            {
            case BRX_ANARI_HDRI_LIGHT_LAYOUT_EQUIRECTANGULAR:
            {
                environment_lighting_skybox_pipeline = this->m_environment_lighting_skybox_equirectangular_map_pipeline;
            }
            break;
            default:
            {
                assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_OCTAHEDRAL == hdri_light_layout);
                environment_lighting_skybox_pipeline = this->m_environment_lighting_skybox_octahedral_map_pipeline;
            }
            }

            graphics_command_buffer->bind_graphics_pipeline(environment_lighting_skybox_pipeline);
        }

        {
            brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_environment_lighting_descriptor_set_none_update, this->m_hdri_light_environment_lighting_descriptor_set_per_environment_lighting_update};

            uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<environment_lighting_none_update_set_uniform_buffer_binding>(frame_throttling_index)};

            graphics_command_buffer->bind_graphics_descriptor_sets(this->m_environment_lighting_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
        }

        graphics_command_buffer->draw(3U, 1U, 0U, 0U);
    }
    else
    {
        assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED == hdri_light_layout);
    }
}
