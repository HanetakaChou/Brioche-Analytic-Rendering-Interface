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
#include "../shaders/none_update_resource_binding.bsli"
#include "../shaders/surface_resource_binding.bsli"

static constexpr float const INTERNAL_ROTATION_EPSILON = 1E-6F;

void brx_anari_pal_device::hdri_light_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_binding *none_update_set_uniform_buffer_destination)
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

    none_update_set_uniform_buffer_destination->g_world_to_environment_map_transform = world_to_environment_map_transform;

    none_update_set_uniform_buffer_destination->g_environment_map_layout = this->m_hdri_light_layout;
}

void brx_anari_pal_device::hdri_light_write_place_holder_none_update_descriptor()
{
    assert(NULL != this->m_place_holder_asset_image);

    // Write None Update Descriptor

    {
        brx_pal_sampled_image const *const sampled_images[] = {this->m_place_holder_asset_image->get_sampled_image()};
        this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 13U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
    }
}

void brx_anari_pal_device::hdri_light_create_none_update_binding_resource()
{
    assert(NULL == this->m_hdri_light_environment_map_sh_coefficients);
    this->m_hdri_light_environment_map_sh_coefficients = this->m_device->create_storage_intermediate_buffer(sizeof(float) * (3U * BRX_SH_COEFFICIENT_COUNT));
}

void brx_anari_pal_device::hdri_light_destroy_none_update_binding_resource()
{
    assert(NULL != this->m_hdri_light_environment_map_sh_coefficients);
    this->helper_destroy_intermediate_buffer(this->m_hdri_light_environment_map_sh_coefficients);
    this->m_hdri_light_environment_map_sh_coefficients = NULL;
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
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_environment_map_clear_compute.inl"
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
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
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_environment_map_clear_compute.inl"
            this->m_environment_lighting_sh_projection_clear_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code), environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code);
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
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
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
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_equirectangular_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_equirectangular_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code), environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code);
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
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_sh_projection_octahedral_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
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
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_sh_projection_octahedral_environment_map_compute.inl"
            this->m_environment_lighting_sh_projection_octahedral_map_pipeline = this->m_device->create_compute_pipeline(this->m_none_update_pipeline_layout, sizeof(environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code), environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code);
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
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_equirectangular_map_fragment.inl"
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_equirectangular_map_fragment.inl"
            this->m_environment_lighting_skybox_equirectangular_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_equirectangular_map_fragment_shader_module_code), environment_lighting_skybox_equirectangular_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_octahedral_map_fragment.inl"
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/environment_lighting_skybox_vertex.inl"
#include "../shaders/spirv/environment_lighting_skybox_octahedral_map_fragment.inl"
            this->m_environment_lighting_skybox_octahedral_map_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(environment_lighting_skybox_vertex_shader_module_code), environment_lighting_skybox_vertex_shader_module_code, sizeof(environment_lighting_skybox_octahedral_map_fragment_shader_module_code), environment_lighting_skybox_octahedral_map_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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

        for (uint32_t frame_throttling_index = 0U; frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
        {
            this->m_device->wait_for_fence(this->m_fences[frame_throttling_index]);
        }

        if (NULL == this->m_hdri_light_radiance)
        {
            assert(NULL != this->m_place_holder_asset_image);

            // Write None Update Descriptor

            {
                brx_pal_sampled_image const *const sampled_images[] = {this->m_place_holder_asset_image->get_sampled_image()};
                this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 13U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
            }
        }
        else
        {
            // Write None Update Descriptor

            {
                brx_pal_sampled_image const *const sampled_images[] = {static_cast<brx_anari_pal_image *>(this->m_hdri_light_radiance)->get_image()->get_sampled_image()};
                this->m_device->write_descriptor_set(this->m_none_update_descriptor_set, 13U, BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
            }
        }

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
#ifndef NDEBUG
    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    bool hdri_light_direction_dirty;
    {
        DirectX::XMFLOAT3 const new_hdri_light_direction(direction.m_x, direction.m_y, direction.m_z);

        DirectX::XMFLOAT3 const old_hdri_light_direction(this->m_hdri_light_direction.m_x, this->m_hdri_light_direction.m_y, this->m_hdri_light_direction.m_z);

        bool hdri_light_direction_not_dirty = DirectX::XMVector3Less(DirectX::XMVectorAbs(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&new_hdri_light_direction), DirectX::XMLoadFloat3(&old_hdri_light_direction))), DirectX::XMVectorReplicate(INTERNAL_ROTATION_EPSILON));

        hdri_light_direction_dirty = (!hdri_light_direction_not_dirty);
    }

    if (hdri_light_direction_dirty)
    {
        this->m_hdri_light_direction = direction;
        this->m_hdri_light_dirty = true;
    }

#ifndef NDEBUG
    this->m_hdri_light_dirty_lock = false;
#endif
}

void brx_anari_pal_device::hdri_light_set_up(brx_anari_vec3 up)
{
#ifndef NDEBUG
    assert(!this->m_hdri_light_dirty_lock);
    this->m_hdri_light_dirty_lock = true;
#endif

    bool hdri_light_up_dirty;
    {
        DirectX::XMFLOAT3 const new_hdri_light_up(up.m_x, up.m_y, up.m_z);

        DirectX::XMFLOAT3 const old_hdri_light_up(this->m_hdri_light_up.m_x, this->m_hdri_light_up.m_y, this->m_hdri_light_up.m_z);

        bool hdri_light_up_not_dirty = DirectX::XMVector3Less(DirectX::XMVectorAbs(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&new_hdri_light_up), DirectX::XMLoadFloat3(&old_hdri_light_up))), DirectX::XMVectorReplicate(INTERNAL_ROTATION_EPSILON));

        hdri_light_up_dirty = (!hdri_light_up_not_dirty);
    }

    if (hdri_light_up_dirty)
    {
        this->m_hdri_light_up = up;
        this->m_hdri_light_dirty = true;
    }

#ifndef NDEBUG
    this->m_hdri_light_dirty_lock = false;
#endif
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

void brx_anari_pal_device::hdri_light_set_enable_skybox_renderer(bool hdri_light_enable_skybox_renderer)
{
    this->m_hdri_light_enable_skybox_renderer = hdri_light_enable_skybox_renderer;
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
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_none_update_descriptor_set};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_none_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
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
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_none_update_descriptor_set};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_compute_descriptor_sets(this->m_none_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
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
    if (this->m_hdri_light_enable_skybox_renderer)
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
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_none_update_descriptor_set};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_graphics_descriptor_sets(this->m_none_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            graphics_command_buffer->draw(3U, 1U, 0U, 0U);
        }
        else
        {
            assert(BRX_ANARI_HDRI_LIGHT_LAYOUT_UNDEFINED == hdri_light_layout);
        }
    }
}
