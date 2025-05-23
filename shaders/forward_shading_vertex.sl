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

#include "forward_shading_resource_binding.sli"
#include "../../Packed-Vector/shaders/brx_packed_vector.bsli"
#include "../../Environment-Lighting/shaders/brx_octahedral_mapping.bsli"
#include "surface.sli"

brx_root_signature(forward_shading_root_signature_macro, forward_shading_root_signature_name)
brx_vertex_shader_parameter_begin(main)
brx_vertex_shader_parameter_in_vertex_index brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out_position brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_vertex_position_world_space, 0) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_vertex_normal, 1) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float4, out_vertex_tagent, 2) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float2, out_vertex_texcoord, 3) 
brx_vertex_shader_parameter_end(main)
{
    brx_uint buffer_texture_flags = brx_byte_address_buffer_load(g_surface_buffers[3], 0);

    brx_uint vertex_index;
    brx_branch
    if(0u != (buffer_texture_flags & Buffer_Flag_Index_Type_UInt16))
    {
        brx_uint index_buffer_offset = (0u == ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_index)) & 3u)) ? (g_index_uint16_buffer_stride * brx_uint(brx_vertex_index)) : ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_index)) - 2u);
        brx_uint packed_vector_index_buffer = brx_byte_address_buffer_load(g_surface_buffers[2], index_buffer_offset);
        brx_uint2 unpacked_vector_index_buffer = brx_R16G16_UINT_to_UINT2(packed_vector_index_buffer);
        vertex_index = (0u == ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_index)) & 3u)) ? unpacked_vector_index_buffer.x : unpacked_vector_index_buffer.y;
    }
    else
    {
        brx_uint index_buffer_offset = g_index_uint32_buffer_stride * brx_uint(brx_vertex_index);
        vertex_index = brx_byte_address_buffer_load(g_surface_buffers[2], index_buffer_offset);
    }

    brx_float3 vertex_position_model_space;
    {
        brx_uint vertex_position_buffer_offset = g_vertex_position_buffer_stride * vertex_index;
        brx_uint3 packed_vector_vertex_position_binding = brx_byte_address_buffer_load3(g_surface_buffers[0], vertex_position_buffer_offset);
        vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_binding);
    }

    brx_float3 vertex_normal_model_space;
    brx_float4 vertex_tangent_model_space;
    brx_float2 vertex_texcoord;
    {
        brx_uint vertex_varying_buffer_offset = g_vertex_varying_buffer_stride * vertex_index;
        brx_uint3 packed_vector_vertex_varying_binding = brx_byte_address_buffer_load3(g_surface_buffers[1], vertex_varying_buffer_offset);
        vertex_normal_model_space = brx_octahedral_unmap(brx_R16G16_SNORM_to_FLOAT2(packed_vector_vertex_varying_binding.x));
        brx_float3 vertex_mapped_tangent_model_space = brx_R15G15B2_SNORM_to_FLOAT3(packed_vector_vertex_varying_binding.y);
        vertex_tangent_model_space = brx_float4(brx_octahedral_unmap(vertex_mapped_tangent_model_space.xy), vertex_mapped_tangent_model_space.z);
        vertex_texcoord = brx_R16G16_UNORM_to_FLOAT2(packed_vector_vertex_varying_binding.z);
    }

    brx_float3 vertex_position_world_space = brx_mul(g_model_transform, brx_float4(vertex_position_model_space, 1.0)).xyz;
    brx_float3 vertex_position_view_space = brx_mul(g_view_transform, brx_float4(vertex_position_world_space, 1.0)).xyz;
    brx_float4 vertex_position_clip_space = brx_mul(g_projection_transform, brx_float4(vertex_position_view_space, 1.0));

    brx_float3 vertex_normal_world_space = brx_mul(g_model_transform, brx_float4(vertex_normal_model_space, 0.0)).xyz;
    
    brx_float4 vertex_tangent_world_space = brx_float4(brx_mul(g_model_transform, brx_float4(vertex_tangent_model_space.xyz, 0.0)).xyz, vertex_tangent_model_space.w);

    brx_position = vertex_position_clip_space;
    out_vertex_position_world_space = vertex_position_world_space;
    out_vertex_normal = vertex_normal_world_space;
    out_vertex_tagent = vertex_tangent_world_space;
    out_vertex_texcoord = vertex_texcoord;
}
