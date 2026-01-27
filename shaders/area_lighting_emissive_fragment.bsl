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
#include "../../Brioche-Shader-Language/shaders/brx_math_consts.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"

brx_root_signature(area_lighting_emissive_root_signature_macro, area_lighting_emissive_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_position_world_space, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_emissive, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_direct_radiance, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_ambient_radiance, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_normal, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_base_color, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_roughness_metallic, 4)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 surface_position_world_space = in_interpolated_position_world_space;

    brx_float3 camera_ray_direction;
    {
        brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
        camera_ray_direction = brx_normalize(surface_position_world_space - camera_ray_origin);
    }

    brx_float3 surface_shading_normal_world_space;
    brx_float3 surface_emissive;
    {
        brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);

        brx_branch if (brx_dot(geometry_normal_world_space, camera_ray_direction) > 0.0)
        {
            // back face

            surface_shading_normal_world_space = -geometry_normal_world_space;

            brx_float2 texcoord = in_interpolated_texcoord;

            brx_float t = brx_smoothstep(0.25, 0.75, brx_sin(BRX_M_PI * 8.0 * texcoord.x) * brx_sin(BRX_M_PI * 8.0 * texcoord.y) * 0.5 + 0.5);

            surface_emissive = brx_lerp(brx_float3(0.0, 0.0, 0.0), in_interpolated_emissive, t);
        }
        else
        {
            surface_shading_normal_world_space = geometry_normal_world_space;

            surface_emissive = in_interpolated_emissive;
        }
    }

    out_direct_radiance = brx_float4(surface_emissive, 1.0);
    out_ambient_radiance = brx_float4(0.0, 0.0, 0.0, 1.0);
    out_gbuffer_normal = brx_float4(brx_octahedral_map(surface_shading_normal_world_space), 0.0, 0.0);
    out_gbuffer_base_color = brx_float4(0.0, 0.0, 0.0, 1.0);
    out_gbuffer_roughness_metallic = brx_float4(0.0, 0.0, 0.0, 0.0);
}
