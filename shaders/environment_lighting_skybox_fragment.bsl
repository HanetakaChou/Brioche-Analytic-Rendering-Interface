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

#include "environment_lighting_skybox_resource_binding.bsli"
#include "../../Environment-Lighting/shaders/brx_equirectangular_mapping.bsli"
#include "../../Environment-Lighting/shaders/brx_octahedral_mapping.bsli"

brx_root_signature(environment_lighting_skybox_root_signature_macro, environment_lighting_skybox_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_uv, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_scene_color, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_uint4, out_gbuffer, 1)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 camera_ray_origin;
    {
        camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
    }

    brx_float3 camera_ray_direction;
    {
        brx_float2 uv = in_interpolated_uv;

        brx_float position_target_depth = 1.0 / 256.0;

        brx_float3 position_target_ndc_space = brx_float3(uv * brx_float2(2.0, -2.0) + brx_float2(-1.0, 1.0), position_target_depth);

        brx_float4 position_target_view_space_with_w = brx_mul(g_inverse_projection_transform, brx_float4(position_target_ndc_space, 1.0));

        brx_float3 position_target_view_space = position_target_view_space_with_w.xyz / position_target_view_space_with_w.w;

        brx_float3 position_target_world_space = brx_mul(g_inverse_view_transform, brx_float4(position_target_view_space, 1.0)).xyz;

        camera_ray_direction = brx_normalize(position_target_world_space - camera_ray_origin);
    }

    brx_float3 environment_map_radiance;
    {
        brx_float3 environment_map_omega = brx_mul(g_world_to_environment_map_transform, brx_float4(camera_ray_direction, 0.0)).xyz;

        brx_float3 environment_map_raw_radiance = brx_float3(0.0, 0.0, 0.0);
        brx_branch
        if (ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR == g_environment_map_layout)
        {
            brx_float2 environment_map_uv = brx_equirectangular_map(environment_map_omega);

            environment_map_raw_radiance = brx_sample_2d(g_environment_map, g_shared_none_update_sampler, environment_map_uv).rgb;
        }
        else
        brx_branch
        if (ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL == g_environment_map_layout)
        {
            brx_float2 environment_map_ndc_flip_y = brx_octahedral_map(environment_map_omega);

            brx_float2 environment_map_uv = (environment_map_ndc_flip_y + brx_float2(1.0, 1.0)) * brx_float2(0.5, 0.5);

            environment_map_raw_radiance = brx_sample_2d(g_environment_map, g_shared_none_update_sampler, environment_map_uv).rgb;
        }
        else
        {
            environment_map_raw_radiance = brx_float3(0.0, 0.0, 0.0);
        }

        // https://github.com/AcademySoftwareFoundation/Imath/blob/main/src/Imath/half.h
        // HALF_MAX 65504.0
        environment_map_radiance = brx_clamp(environment_map_raw_radiance, brx_float3(0.0, 0.0, 0.0), brx_float3(65504.0, 65504.0, 65504.0));
    }

    brx_float3 outgoing_radiance = environment_map_radiance;

    out_scene_color = brx_float4(outgoing_radiance, 1.0);

    out_gbuffer = brx_uint4(0, 0, 0, 0);
}
