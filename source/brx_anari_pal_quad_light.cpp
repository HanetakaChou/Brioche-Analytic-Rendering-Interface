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
#include <cstring>

void brx_anari_pal_device::set_quad_lights(uint32_t quad_light_count, BRX_ANARI_QUAD const *quad_lights)
{
    this->m_quad_lights.resize(quad_light_count);
    std::memcpy(this->m_quad_lights.data(), quad_lights, sizeof(BRX_ANARI_QUAD) * quad_light_count);
}

void brx_anari_pal_device::quad_light_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_binding *none_update_set_uniform_buffer_destination)
{
    uint32_t const quad_light_count = this->m_quad_lights.size();
    assert(quad_light_count <= NONE_UPDATE_MAX_AREA_LIGHTING_COUNT);
    BRX_ANARI_QUAD const *const quad_lights = this->m_quad_lights.data();
    none_update_set_uniform_buffer_destination->g_area_lighting_count = quad_light_count;
    for (uint32_t quad_light_index = 0U; quad_light_index < quad_light_count; ++quad_light_index)
    {
        BRX_ANARI_QUAD const quad_light = quad_lights[quad_light_index];

        none_update_set_uniform_buffer_destination->g_area_lightings[3U * quad_light_index + 0U] = DirectX::XMFLOAT4(quad_light.m_position.m_x, quad_light.m_position.m_y, quad_light.m_position.m_z, quad_light.m_radiance.m_x);
        none_update_set_uniform_buffer_destination->g_area_lightings[3U * quad_light_index + 1U] = DirectX::XMFLOAT4(quad_light.m_edge1.m_x, quad_light.m_edge1.m_y, quad_light.m_edge1.m_z, quad_light.m_radiance.m_y);
        none_update_set_uniform_buffer_destination->g_area_lightings[3U * quad_light_index + 2U] = DirectX::XMFLOAT4(quad_light.m_edge2.m_x, quad_light.m_edge2.m_y, quad_light.m_edge2.m_z, quad_light.m_radiance.m_z);
    }
}

void brx_anari_pal_device::quad_light_create_pipeline()
{
    assert(NULL != this->m_forward_shading_render_pass);

    {
        assert(NULL == this->m_area_lighting_emissive_pipeline);
#if defined(__GNUC__)
#if defined(__linux__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/area_lighting_emissive_vertex.inl"
#include "../shaders/spirv/area_lighting_emissive_fragment.inl"
            this->m_area_lighting_emissive_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(area_lighting_emissive_vertex_shader_module_code), area_lighting_emissive_vertex_shader_module_code, sizeof(area_lighting_emissive_fragment_shader_module_code), area_lighting_emissive_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#elif defined(__MACH__)
        assert(BRX_PAL_BACKEND_NAME_VK == this->m_device->get_backend_name());
        {
#include "../shaders/spirv/area_lighting_emissive_vertex.inl"
#include "../shaders/spirv/area_lighting_emissive_fragment.inl"
            this->m_area_lighting_emissive_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(area_lighting_emissive_vertex_shader_module_code), area_lighting_emissive_vertex_shader_module_code, sizeof(area_lighting_emissive_fragment_shader_module_code), area_lighting_emissive_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        switch (this->m_device->get_backend_name())
        {
        case BRX_PAL_BACKEND_NAME_D3D12:
        {
#include "../shaders/dxil/area_lighting_emissive_vertex.inl"
#include "../shaders/dxil/area_lighting_emissive_fragment.inl"
            this->m_area_lighting_emissive_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(area_lighting_emissive_vertex_shader_module_code), area_lighting_emissive_vertex_shader_module_code, sizeof(area_lighting_emissive_fragment_shader_module_code), area_lighting_emissive_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
        }
        break;
        case BRX_PAL_BACKEND_NAME_VK:
        {
#include "../shaders/spirv/area_lighting_emissive_vertex.inl"
#include "../shaders/spirv/area_lighting_emissive_fragment.inl"
            this->m_area_lighting_emissive_pipeline = this->m_device->create_graphics_pipeline(this->m_forward_shading_render_pass, this->m_none_update_pipeline_layout, sizeof(area_lighting_emissive_vertex_shader_module_code), area_lighting_emissive_vertex_shader_module_code, sizeof(area_lighting_emissive_fragment_shader_module_code), area_lighting_emissive_fragment_shader_module_code, false, true, true, 1U, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER_EQUAL, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE);
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

void brx_anari_pal_device::quad_light_destroy_pipeline()
{
    assert(NULL != this->m_area_lighting_emissive_pipeline);
    this->m_device->destroy_graphics_pipeline(this->m_area_lighting_emissive_pipeline);
    this->m_area_lighting_emissive_pipeline = NULL;
}

void brx_anari_pal_device::quad_light_render_emissive(uint32_t frame_throttling_index, brx_pal_graphics_command_buffer *graphics_command_buffer)
{
    if (this->m_quad_lights_enable_debug_renderer)
    {
        uint32_t const quad_light_count = this->m_quad_lights.size();
        assert(quad_light_count <= NONE_UPDATE_MAX_AREA_LIGHTING_COUNT);

        if (quad_light_count > 0U)
        {
            graphics_command_buffer->bind_graphics_pipeline(this->m_area_lighting_emissive_pipeline);

            {
                brx_pal_descriptor_set const *const descriptor_sets[] = {this->m_none_update_descriptor_set};

                uint32_t const dynamic_offsets[] = {this->helper_compute_uniform_buffer_dynamic_offset<none_update_set_uniform_buffer_binding>(frame_throttling_index)};

                graphics_command_buffer->bind_graphics_descriptor_sets(this->m_none_update_pipeline_layout, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);
            }

            graphics_command_buffer->draw(6U * quad_light_count, 1U, 0U, 0U);
        }
    }
}

void brx_anari_pal_device::set_quad_lights_enable_debug_renderer(bool quad_lights_enable_debug_renderer)
{
    this->m_quad_lights_enable_debug_renderer = quad_lights_enable_debug_renderer;
}

bool brx_anari_pal_device::get_quad_lights_enable_debug_renderer() const
{
    return this->m_quad_lights_enable_debug_renderer;
}
