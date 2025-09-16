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

#include "forward_shading_resource_binding.bsli"
#include "../../Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_voxelization_vertex.bsli"

#include "../../Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_packed_vector.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

void brx_vct_application_bridge_get_triangle_vertices(in brx_uint in_triangle_index, out brx_float3 out_triangle_vertex_position_world_space_a, out brx_float3 out_triangle_vertex_position_world_space_b, out brx_float3 out_triangle_vertex_position_world_space_c)
{
    brx_uint buffer_texture_flags = brx_byte_address_buffer_load(t_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 0);

    brx_uint3 triangle_vertex_indices;
    brx_branch if (0u != (buffer_texture_flags & SURFACE_BUFFER_FLAG_UINT16_INDEX))
    {
        brx_uint index_buffer_offset = (0u == (((SURFACE_UINT16_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index) & 3u)) ? ((SURFACE_UINT16_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index) : (((SURFACE_UINT16_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index) - 2u);

        brx_uint2 packed_vector_index_buffer = brx_byte_address_buffer_load2(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], index_buffer_offset);

        brx_uint4 unpacked_vector_index_buffer = brx_R16G16B16A16_UINT_to_UINT4(packed_vector_index_buffer);

        triangle_vertex_indices = (0u == (((SURFACE_UINT16_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index) & 3u)) ? unpacked_vector_index_buffer.xyz : unpacked_vector_index_buffer.yzw;
    }
    else
    {
        brx_uint index_buffer_offset = (SURFACE_UINT32_INDEX_BUFFER_STRIDE * 3u) * in_triangle_index;

        triangle_vertex_indices = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], index_buffer_offset);
    }

    brx_float3 triangle_vertices_position_model_space[3];
    {
        brx_uint3 vertex_position_buffer_offset = SURFACE_VERTEX_POSITION_BUFFER_STRIDE * triangle_vertex_indices;

        brx_uint3 packed_vector_vertices_position_binding[3];
        packed_vector_vertices_position_binding[0] = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], vertex_position_buffer_offset.x);
        packed_vector_vertices_position_binding[1] = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], vertex_position_buffer_offset.y);
        packed_vector_vertices_position_binding[2] = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], vertex_position_buffer_offset.z);

        triangle_vertices_position_model_space[0] = brx_uint_as_float(packed_vector_vertices_position_binding[0]);
        triangle_vertices_position_model_space[1] = brx_uint_as_float(packed_vector_vertices_position_binding[1]);
        triangle_vertices_position_model_space[2] = brx_uint_as_float(packed_vector_vertices_position_binding[2]);
    }

    {
        out_triangle_vertex_position_world_space_a = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[0], 1.0)).xyz;
        out_triangle_vertex_position_world_space_b = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[1], 1.0)).xyz;
        out_triangle_vertex_position_world_space_c = brx_mul(g_model_transform, brx_float4(triangle_vertices_position_model_space[2], 1.0)).xyz;
    }
}

void brx_vct_application_bridge_get_vertex(in brx_uint in_vertex_index, out brx_float3 out_vertex_position_world_space, out brx_float3 out_vertex_normal_world_space, out brx_float4 out_vertex_tangent_world_space, out brx_float2 out_vertex_texcoord)
{
    brx_uint buffer_texture_flags = brx_byte_address_buffer_load(t_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 0);

    brx_uint vertex_index;
    brx_branch if (0u != (buffer_texture_flags & SURFACE_BUFFER_FLAG_UINT16_INDEX))
    {
        brx_uint index_buffer_offset = (0u == ((SURFACE_UINT16_INDEX_BUFFER_STRIDE * brx_uint(in_vertex_index)) & 3u)) ? (SURFACE_UINT16_INDEX_BUFFER_STRIDE * brx_uint(in_vertex_index)) : ((SURFACE_UINT16_INDEX_BUFFER_STRIDE * brx_uint(in_vertex_index)) - 2u);
        brx_uint packed_vector_index_buffer = brx_byte_address_buffer_load(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], index_buffer_offset);
        brx_uint2 unpacked_vector_index_buffer = brx_R16G16_UINT_to_UINT2(packed_vector_index_buffer);
        vertex_index = (0u == ((SURFACE_UINT16_INDEX_BUFFER_STRIDE * brx_uint(in_vertex_index)) & 3u)) ? unpacked_vector_index_buffer.x : unpacked_vector_index_buffer.y;
    }
    else
    {
        brx_uint index_buffer_offset = SURFACE_UINT32_INDEX_BUFFER_STRIDE * brx_uint(in_vertex_index);
        vertex_index = brx_byte_address_buffer_load(t_surface_buffers[FORWARD_SHADING_SURFACE_INDEX_BUFFER_INDEX], index_buffer_offset);
    }

    brx_float3 vertex_position_model_space;
    {
        brx_uint vertex_position_buffer_offset = SURFACE_VERTEX_POSITION_BUFFER_STRIDE * vertex_index;
        brx_uint3 packed_vector_vertex_position_binding = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_VERTEX_POSITION_BUFFER_INDEX], vertex_position_buffer_offset);
        vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_binding);
    }

    brx_float3 vertex_normal_model_space;
    brx_float4 vertex_tangent_model_space;
    brx_float2 vertex_texcoord;
    {
        brx_uint vertex_varying_buffer_offset = SURFACE_VERTEX_VARYING_BUFFER_STRIDE * vertex_index;
        brx_uint3 packed_vector_vertex_varying_binding = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_VERTEX_VARYING_BUFFER_INDEX], vertex_varying_buffer_offset);
        vertex_normal_model_space = brx_octahedral_unmap(brx_R16G16_SNORM_to_FLOAT2(packed_vector_vertex_varying_binding.x));
        brx_float3 vertex_mapped_tangent_model_space = brx_R15G15B2_SNORM_to_FLOAT3(packed_vector_vertex_varying_binding.y);
        vertex_tangent_model_space = brx_float4(brx_octahedral_unmap(vertex_mapped_tangent_model_space.xy), vertex_mapped_tangent_model_space.z);
        // https://github.com/AcademySoftwareFoundation/Imath/blob/main/src/Imath/half.h
        // HALF_MAX 65504.0
        vertex_texcoord = brx_clamp(brx_unpack_half2(packed_vector_vertex_varying_binding.z), brx_float2(-65504.0, -65504.0), brx_float2(65504.0, 65504.0));
    }

    out_vertex_position_world_space = brx_mul(g_model_transform, brx_float4(vertex_position_model_space, 1.0)).xyz;

    out_vertex_normal_world_space = brx_mul(g_model_transform, brx_float4(vertex_normal_model_space, 0.0)).xyz;
    out_vertex_tangent_world_space = brx_float4(brx_mul(g_model_transform, brx_float4(vertex_tangent_model_space.xyz, 0.0)).xyz, vertex_tangent_model_space.w);
    out_vertex_texcoord = vertex_texcoord;
}

brx_float4x4 brx_vct_application_bridge_get_viewport_depth_direction_view_matrix(in brx_int in_viewport_depth_direction_index)
{
    return g_viewport_depth_direction_view_matrices[in_viewport_depth_direction_index];
}

brx_float4x4 brx_vct_application_bridge_get_clipmap_stack_level_projection_matrix(in brx_int in_clipmap_stack_level_index)
{
    return g_clipmap_stack_level_projection_matrices[in_clipmap_stack_level_index];
}
