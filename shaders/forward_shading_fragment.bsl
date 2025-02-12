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
#include "../../Brioche-Shader-Language/shaders/brx_math_consts.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_packed_vector.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_brdf.bsli"
#include "../../Environment-Lighting/shaders/brx_octahedral_mapping.bsli"
#include "../../Environment-Lighting/shaders/brx_environment_lighting_diffuse_radiance.bsli"

brx_root_signature(forward_shading_root_signature_macro, forward_shading_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_position_world_space, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float4, in_interpolated_tangent, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_scene_color, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_uint4, out_gbuffer, 1)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 camera_ray_direction;
    brx_float3 shading_normal_world_space;
    brx_float3 diffuse_color;
    brx_float3 specular_color;
    brx_float roughness;
    brx_float3 emissive;
    brx_float base_color_alpha;
    {
        brx_uint buffer_texture_flags;
        brx_float3 emissive_factor;
        brx_float4 base_color_factor;
        brx_float normal_scale;
        brx_float metallic_factor;
        brx_float roughness_factor;
        {
            brx_uint4 auxiliary_packed_vectors[3];
            auxiliary_packed_vectors[0] = brx_byte_address_buffer_load4(g_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 0);
            auxiliary_packed_vectors[1] = brx_byte_address_buffer_load4(g_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 4 * 4);
            auxiliary_packed_vectors[2].xyz = brx_byte_address_buffer_load3(g_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 4 * 4 + 4 * 4);
            buffer_texture_flags = auxiliary_packed_vectors[0].x;
            emissive_factor = brx_uint_as_float(auxiliary_packed_vectors[0].yzw);
            base_color_factor = brx_uint_as_float(auxiliary_packed_vectors[1].xyzw);
            normal_scale = brx_uint_as_float(auxiliary_packed_vectors[2].x);
            metallic_factor = brx_uint_as_float(auxiliary_packed_vectors[2].y);
            roughness_factor = brx_uint_as_float(auxiliary_packed_vectors[2].z);
        }

        {
            brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
            brx_float3 surface_position_world_space = in_interpolated_position_world_space;
            camera_ray_direction = brx_normalize(surface_position_world_space - camera_ray_origin);
        }

        brx_float2 texcoord = in_interpolated_texcoord;

        brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);
        
        brx_branch
        if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_NORMAL))
        {
            brx_float4 tangent_world_space = brx_float4(brx_normalize(in_interpolated_tangent.xyz), in_interpolated_tangent.w);

            // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
            brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(g_surface_images[FORWARD_SHADING_SURFACE_NORMAL_TEXTURE_INDEX], g_shared_none_update_sampler, texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_scale, normal_scale, 1.0));
            brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space.xyz) * tangent_world_space.w;
            shading_normal_world_space = brx_normalize(tangent_world_space.xyz * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);
        }
        else
        {
            shading_normal_world_space = geometry_normal_world_space;
        }

        // flip back side normal
        {
            brx_branch
            if (brx_dot(geometry_normal_world_space, camera_ray_direction) > 0.0)
            {
                geometry_normal_world_space = -geometry_normal_world_space;
                shading_normal_world_space = -shading_normal_world_space;
            }
        }

        // [donut: getBentNormal](https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/shaders/utils.hlsli#L139)
        {
            brx_float3 reflected_ray_direction = brx_reflect(camera_ray_direction, shading_normal_world_space);

            brx_float a = brx_dot(geometry_normal_world_space, reflected_ray_direction);

            brx_branch
            if (a < 0.0)
            {
                brx_float b = brx_max(0.001, brx_dot(geometry_normal_world_space, shading_normal_world_space));

                reflected_ray_direction = brx_normalize(reflected_ray_direction + shading_normal_world_space * ((-a) / b));

                shading_normal_world_space = brx_normalize((-camera_ray_direction) + reflected_ray_direction);
            }
        }

        brx_float4 base_color;
        brx_branch
        if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_BASE_COLOR))
        {
            base_color = base_color_factor * brx_sample_2d(g_surface_images[FORWARD_SHADING_SURFACE_BASE_COLOR_TEXTURE_INDEX], g_shared_none_update_sampler, texcoord);

        }
        else
        {
            base_color = base_color_factor;
        }

        // usually the vertices within the same model is organized from back to front 
        // we can simply use the over operation to render the result correctly (just like how we render the imgui)
        base_color_alpha = base_color.w;

        brx_float metallic;
        brx_branch
        if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_METALLIC_ROUGHNESS))
        {
            brx_float2 metallic_roughness = brx_sample_2d(g_surface_images[FORWARD_SHADING_SURFACE_METALLIC_ROUGHNESS_TEXTURE_INDEX], g_shared_none_update_sampler, texcoord).bg;
            metallic = metallic_factor * metallic_roughness.x;
            roughness = roughness_factor * metallic_roughness.y;
        }
        else
        {
            metallic = metallic_factor;
            roughness = roughness_factor;
        }

        {
            const brx_float dielectric_specular = 0.04;
            // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
            specular_color = brx_clamp((dielectric_specular - dielectric_specular * metallic) + base_color.xyz * metallic, 0.0, 1.0);
            diffuse_color = brx_clamp(base_color.xyz - base_color.xyz * metallic, 0.0, 1.0);
        }

        // Specular Anti-Aliasing
        // "7.8.1 Mipmapping BRDF and Normal Maps" of "Real-Time Rendering Third Edition"
        // "9.13.1 Filtering Normals and Normal Distributions" of "Real-Time Rendering Fourth Edition"
        // UE4: [NormalCurvatureToRoughness](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L67)
        // U3D: [TextureNormalVariance ](https://github.com/Unity-Technologies/Graphics/blob/v10.8.1/com.unity.render-pipelines.core/ShaderLibrary/CommonMaterial.hlsl#L214)
        {
            const brx_float cvar_normal_curvature_to_roughness_scale = 1.0;
            const brx_float cvar_normal_curvature_to_roughness_bias = 0.0;
            const brx_float cvar_normal_curvature_to_roughness_exponent = 0.3333333;

            brx_float3 dn_dx = brx_ddx(geometry_normal_world_space);
            brx_float3 dn_dy = brx_ddy(geometry_normal_world_space);
            brx_float x = brx_dot(dn_dx, dn_dx);
            brx_float y = brx_dot(dn_dy, dn_dy);

            brx_float curvature_approx = brx_pow(brx_max(x, y), cvar_normal_curvature_to_roughness_exponent);
            brx_float geometric_aa_roughness = brx_clamp(curvature_approx * cvar_normal_curvature_to_roughness_scale + cvar_normal_curvature_to_roughness_bias, 0.0, 1.0);

            roughness = brx_max(roughness, geometric_aa_roughness);
        }

        // Prevent the roughness to be zero
        // https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/CapsuleLightIntegrate.ush#L94
        {
            const brx_float cvar_global_min_roughness_override = 0.02;
            roughness = brx_max(roughness, cvar_global_min_roughness_override);
        }

        brx_branch
        if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_EMISSIVE))
        {
            emissive = emissive_factor * brx_sample_2d(g_surface_images[FORWARD_SHADING_SURFACE_EMISSIVE_TEXTURE_INDEX], g_shared_none_update_sampler, texcoord).xyz;
        }
        else
        {
            emissive = emissive_factor;
        }
    }

    // TODO: shadow
    // TODO: specular environment lighting
    brx_float3 outgoing_radiance = brx_float3(0.0, 0.0, 0.0);

#if 0
	const brx_int incident_light_count = 8;

	const brx_float3 incident_illuminances[incident_light_count] = brx_array_constructor_begin(brx_float3, incident_light_count) 
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) 
        brx_array_constructor_end;

	const brx_float3 Ls[incident_light_count] = brx_array_constructor_begin(brx_float3, incident_light_count) 
		brx_float3( 1.0,  1.0,  1.0) brx_array_constructor_split
		brx_float3( 1.0,  1.0, -1.0) brx_array_constructor_split
		brx_float3(-1.0,  1.0,  1.0) brx_array_constructor_split
		brx_float3(-1.0,  1.0, -1.0) brx_array_constructor_split
		brx_float3( 1.0, -1.0,  1.0) brx_array_constructor_split
		brx_float3( 1.0, -1.0, -1.0) brx_array_constructor_split
		brx_float3(-1.0, -1.0,  1.0) brx_array_constructor_split
		brx_float3(-1.0, -1.0, -1.0)
        brx_array_constructor_end;

	brx_unroll
	for(brx_int incident_light_index = 0; incident_light_index < incident_light_count; ++incident_light_index)
	{
		brx_float3 incident_illuminance = incident_illuminances[incident_light_index];
		brx_float3 L = brx_normalize(Ls[incident_light_index]);

        brx_float3 V = -camera_ray_direction;
        brx_float3 N = shading_normal_world_space;
        brx_float NdotL = brx_clamp(brx_dot(N, L), 0.0, 1.0);
        brx_float NdotV = brx_clamp(brx_dot(N, V), 0.0, 1.0);
        brx_float VdotL = brx_dot(V, L);

        // Real-Time Rendering Fourth Edition / 9.8 BRDF Models for Surface Reflection / [Hammon 2017]
        //
        // |L + V|^2 = L^2 + V^2 + 2L·V = 1 + 1 + 2L·V
        // N·H = N·((L + V)/(|L + V|)) = (N·L + N·V)/(|L + V|)
        // L·H = L·((L + V)/(|L + V|)) = (L^2 + L·V)/(|L + V|)
        // V·H = V·((L + V)/(|L + V|)) = (L·V + V^2)/(|L + V|)
        // 2L·H = 2V·H = L·H + V·H = (L^2 + L·V + L·V + V^2)/(|L + V|) = (|L + V|^2)/(|L + V|) = |L + V| = 1 + 1 + 2L·V
        // ⇒ L·H = 0.5 * |L + V| = (0.5 * |L + V|^2)/(|L + V|) =(0.5 * (1 + 1 + 2L·V))/(|L + V|) = 1/(|L + V|) + (L·V)/(|L + V|)
        //
        // UE4: [Init](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L31)
        // U3D: [GetBSDFAngle](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L361)
        brx_float invLenH = brx_rsqrt(2.0 + 2.0 * VdotL);
        brx_float NdotH = brx_clamp((NdotL + NdotV) * invLenH, 0.0, 1.0);
        brx_float LdotH = brx_clamp(invLenH * VdotL + invLenH, 0.0, 1.0);

        brx_float VdotH = LdotH;

        brx_float3 brdf_diffuse = brx_brdf_diffuse_lambert(diffuse_color);

        brx_float3 brdf_specular;
        {
			// Trowbridge Reitz

			// Prevent the NdotV to be zero
			// https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L34
			NdotV = brx_max(NdotV, 1e-5);

			// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as g = r2, where r is the user-interface roughness parameter value between 0 and 1."
			brx_float alpha = roughness * roughness;

			brx_float D = brx_brdf_specular_trowbridge_reitz_ndf(alpha, NdotH);

			brx_float V = brx_brdf_specular_trowbridge_reitz_visibility(alpha, NdotV, NdotL);

			// glTF Sample Renderer: [F_Schlick](https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/e5646a2bf87b0871ba3f826fc2335fe117a11411/source/Renderer/shaders/brdf.glsl#L24)
            brx_float3 f0 = specular_color;
			const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

			brx_float x = brx_clamp(1.0 - VdotH, 0.0, 1.0);
			brx_float x2 = x * x;
			brx_float x5 = x * x2 * x2;
			brx_float3 F = f0 + (f90 - f0) * x5;

			brdf_specular = D * V * F;
		}

		 outgoing_radiance += (brdf_diffuse + brdf_specular) * (NdotL * incident_illuminance);
	}
#endif

    // Environment Lighting
    {
        brx_float3 irradiance_coefficients[9];
        for (brx_int irradiance_coefficient_index = 0; irradiance_coefficient_index < 9; ++irradiance_coefficient_index)
        {
            irradiance_coefficients[irradiance_coefficient_index] = brx_uint_as_float(brx_byte_address_buffer_load3(g_irradiance_coefficients, (3 * (irradiance_coefficient_index << 2))));
        }

        brx_float3 shading_normal_environment_map_space = brx_mul(g_world_to_environment_map_transform, brx_float4(shading_normal_world_space, 0.0)).xyz;

        brx_float3 environment_lighting_diffuse_radiance = brx_environment_lighting_diffuse_radiance(diffuse_color, specular_color, shading_normal_environment_map_space, irradiance_coefficients);
        outgoing_radiance += environment_lighting_diffuse_radiance;
    }

    outgoing_radiance += emissive;

    out_scene_color = brx_float4(outgoing_radiance, base_color_alpha);

    brx_uint packed_shading_normal_world_space = brx_FLOAT2_to_R16G16_SNORM(brx_octahedral_map(shading_normal_world_space));
    brx_uint packed_roughness = brx_FLOAT2_to_R16G16_UNORM(brx_float2(roughness, 0.0F));
    out_gbuffer = brx_uint4(packed_shading_normal_world_space, packed_roughness, 0, 0);
}
