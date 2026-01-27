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

#include "none_update_resource_binding.bsli"

brx_root_signature(area_lighting_emissive_root_signature_macro, area_lighting_emissive_root_signature_name)
brx_vertex_shader_parameter_begin(main)
brx_vertex_shader_parameter_in_vertex_index brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out_position brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_vertex_position_world_space, 0) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_normal, 1) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float2, out_vertex_texcoord, 2) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_emissive, 3) 
brx_vertex_shader_parameter_end(main)
{
    brx_int area_lighting_index = brx_int(brx_uint(brx_vertex_index) / 6u);
    // assert area_lighting_index < g_area_lighting_count

    brx_float4 area_lighting_auxiliary_packed_vectors[3] = brx_array_constructor_begin(brx_float4, 3)
        g_area_lightings[3 * area_lighting_index + 0] brx_array_constructor_split
        g_area_lightings[3 * area_lighting_index + 1] brx_array_constructor_split
        g_area_lightings[3 * area_lighting_index + 2] brx_array_constructor_end;

    brx_float3 area_lighting_position = area_lighting_auxiliary_packed_vectors[0].xyz;
    brx_float3 area_lighting_edge1 = area_lighting_auxiliary_packed_vectors[1].xyz;
    brx_float3 area_lighting_edge2 = area_lighting_auxiliary_packed_vectors[2].xyz;

    // https://github.com/AcademySoftwareFoundation/Imath/blob/main/src/Imath/half.h
    // HALF_MAX 65504.0
    brx_float3 area_lighting_radiance = brx_clamp(brx_float3(area_lighting_auxiliary_packed_vectors[0].w, area_lighting_auxiliary_packed_vectors[1].w, area_lighting_auxiliary_packed_vectors[2].w), brx_float3(0.0, 0.0, 0.0), brx_float3(65504.0, 65504.0, 65504.0));

    brx_float3 quad_vertices_world_space[4] = brx_array_constructor_begin(brx_float3, 4)
        area_lighting_position brx_array_constructor_split
        (area_lighting_position + area_lighting_edge2) brx_array_constructor_split
        (area_lighting_position + area_lighting_edge1 + area_lighting_edge2) brx_array_constructor_split
        (area_lighting_position + area_lighting_edge1) brx_array_constructor_end;

    brx_float3 quad_normal_world_space = brx_normalize(brx_cross(quad_vertices_world_space[1] - quad_vertices_world_space[0], quad_vertices_world_space[3] - quad_vertices_world_space[0]));

    const brx_float2 quad_vertices_texcoords[4] = brx_array_constructor_begin(brx_float2, 4)
        brx_float2(0.0, 1.0) brx_array_constructor_split
        brx_float2(0.0, 0.0) brx_array_constructor_split
        brx_float2(1.0, 0.0) brx_array_constructor_split
        brx_float2(1.0, 1.0) brx_array_constructor_end;

    brx_int quad_indices[6] = brx_array_constructor_begin(brx_int, 6)
        0 brx_array_constructor_split
        1 brx_array_constructor_split
        3 brx_array_constructor_split
        3 brx_array_constructor_split
        1 brx_array_constructor_split
        2 brx_array_constructor_end;

    brx_int quad_index_index = brx_int(brx_uint(brx_vertex_index) % 6u);;
    // assert quad_index_index < 6

    brx_int quad_vertex_index = quad_indices[quad_index_index];
    
    brx_float3 vertex_position_world_space = quad_vertices_world_space[quad_vertex_index];
    brx_float3 vertex_position_view_space = brx_mul(g_view_transform, brx_float4(vertex_position_world_space, 1.0)).xyz;
    brx_float4 vertex_position_clip_space = brx_mul(g_projection_transform, brx_float4(vertex_position_view_space, 1.0));

    brx_float2 vertex_texcoord = quad_vertices_texcoords[quad_vertex_index];

	brx_position = vertex_position_clip_space;
    out_vertex_position_world_space = vertex_position_world_space;
    out_normal = quad_normal_world_space;
    out_vertex_texcoord = vertex_texcoord;
    out_emissive = area_lighting_radiance;
}
