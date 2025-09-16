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
#include "../../Brioche-Shader-Language/shaders/brx_brdf.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"
#include "../../Spherical-Harmonic/shaders/brx_spherical_harmonic_diffuse_radiance.bsli"
#include "../../Spherical-Harmonic/shaders/brx_spherical_harmonic_specular_radiance.bsli"

brx_root_signature(forward_shading_root_signature_macro, forward_shading_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_position_world_space, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float4, in_interpolated_tangent, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_direct_radiance, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_ambient_radiance, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_normal, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_base_color, 3) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_gbuffer_roughness_metallic, 4)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 camera_ray_direction;
    {
        brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;
        brx_float3 surface_position_world_space = in_interpolated_position_world_space;
        camera_ray_direction = brx_normalize(surface_position_world_space - camera_ray_origin);
    }

    brx_float3 surface_shading_normal_world_space;
    brx_float3 surface_diffuse_color;
    brx_float3 surface_specular_color;
    brx_float3 surface_base_color;
    brx_float surface_metallic;
    brx_float surface_roughness;
    brx_float surface_opacity;
    brx_float3 surface_emissive;
    {
        brx_uint buffer_texture_flags;
        brx_float3 emissive_factor;
        brx_float4 base_color_factor;
        brx_float normal_scale;
        brx_float metallic_factor;
        brx_float roughness_factor;
        {
            brx_uint4 auxiliary_packed_vectors[3];
            auxiliary_packed_vectors[0] = brx_byte_address_buffer_load4(t_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 0);
            auxiliary_packed_vectors[1] = brx_byte_address_buffer_load4(t_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 4 * 4);
            auxiliary_packed_vectors[2].xyz = brx_byte_address_buffer_load3(t_surface_buffers[FORWARD_SHADING_SURFACE_AUXILIARY_BUFFER_INDEX], 4 * 4 + 4 * 4);
            buffer_texture_flags = auxiliary_packed_vectors[0].x;
            emissive_factor = brx_uint_as_float(auxiliary_packed_vectors[0].yzw);
            base_color_factor = brx_uint_as_float(auxiliary_packed_vectors[1].xyzw);
            normal_scale = brx_uint_as_float(auxiliary_packed_vectors[2].x);
            metallic_factor = brx_uint_as_float(auxiliary_packed_vectors[2].y);
            roughness_factor = brx_uint_as_float(auxiliary_packed_vectors[2].z);
        }

        brx_float2 texcoord = in_interpolated_texcoord;

        brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);

        brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_NORMAL))
        {
            brx_float3 tangent_world_space = brx_normalize(in_interpolated_tangent.xyz);

            brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space) * ((in_interpolated_tangent.w >= 0.0) ? 1.0 : -1.0);

            // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
            brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_NORMAL_TEXTURE_INDEX], s_sampler, texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_scale, normal_scale, 1.0));

            surface_shading_normal_world_space = brx_normalize(tangent_world_space * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);
        }
        else
        {
            surface_shading_normal_world_space = geometry_normal_world_space;
        }

        brx_float4 base_color_and_opacity;
        brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_BASE_COLOR))
        {
            base_color_and_opacity = base_color_factor * brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_BASE_COLOR_TEXTURE_INDEX], s_sampler, texcoord);
        }
        else
        {
            base_color_and_opacity = base_color_factor;
        }

        surface_base_color = base_color_and_opacity.xyz;

        // usually the vertices within the same model is organized from back to front
        // we can simply use the over operation to render the result correctly (just like how we render the imgui)
        surface_opacity = base_color_and_opacity.w;

        brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_METALLIC_ROUGHNESS))
        {
            brx_float2 roughness_metallic = brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_METALLIC_ROUGHNESS_TEXTURE_INDEX], s_sampler, texcoord).yz;
            surface_roughness = roughness_factor * roughness_metallic.x;
            surface_metallic = metallic_factor * roughness_metallic.y;
        }
        else
        {
            surface_roughness = roughness_factor;
            surface_metallic = metallic_factor;
        }

        {
            const brx_float dielectric_specular = 0.04;
            // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
            surface_specular_color = brx_clamp((dielectric_specular - dielectric_specular * surface_metallic) + surface_base_color * surface_metallic, 0.0, 1.0);
            surface_diffuse_color = brx_clamp(surface_base_color - surface_base_color * surface_metallic, 0.0, 1.0);
        }

        brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_EMISSIVE))
        {
            surface_emissive = emissive_factor * brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_EMISSIVE_TEXTURE_INDEX], s_sampler, texcoord).xyz;
        }
        else
        {
            surface_emissive = emissive_factor;
        }

#if 0
        // flip back side normal
        {
            brx_branch if (brx_dot(geometry_normal_world_space, camera_ray_direction) > 0.0)
            {
                geometry_normal_world_space = -geometry_normal_world_space;
                surface_shading_normal_world_space = -surface_shading_normal_world_space;
            }
        }

        // [donut: getBentNormal](https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/shaders/utils.hlsli#L139)
        {
            brx_float3 reflected_ray_direction = brx_reflect(camera_ray_direction, surface_shading_normal_world_space);

            brx_float a = brx_dot(geometry_normal_world_space, reflected_ray_direction);

            brx_branch if (a < 0.0)
            {
                brx_float b = brx_max(0.001, brx_dot(geometry_normal_world_space, surface_shading_normal_world_space));

                reflected_ray_direction = brx_normalize(reflected_ray_direction + surface_shading_normal_world_space * ((-a) / b));

                surface_shading_normal_world_space = brx_normalize((-camera_ray_direction) + reflected_ray_direction);
            }
        }
#endif

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

            surface_roughness = brx_max(surface_roughness, geometric_aa_roughness);
        }

        // Prevent the roughness to be zero
        // https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/CapsuleLightIntegrate.ush#L94
        {
            const brx_float cvar_global_min_roughness_override = 0.02;
            surface_roughness = brx_max(surface_roughness, cvar_global_min_roughness_override);
        }
    }

    // TODO: shadow
    brx_float3 direct_radiance = brx_float3(0.0, 0.0, 0.0);

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
        brx_float3(1.0, 1.0, 1.0) brx_array_constructor_split
        brx_float3(1.0, 1.0, -1.0) brx_array_constructor_split
        brx_float3(-1.0, 1.0, 1.0) brx_array_constructor_split
        brx_float3(-1.0, 1.0, -1.0) brx_array_constructor_split
        brx_float3(1.0, -1.0, 1.0) brx_array_constructor_split
        brx_float3(1.0, -1.0, -1.0) brx_array_constructor_split
        brx_float3(-1.0, -1.0, 1.0) brx_array_constructor_split
        brx_float3(-1.0, -1.0, -1.0)
        brx_array_constructor_end;

    brx_unroll for (brx_int incident_light_index = 0; incident_light_index < incident_light_count; ++incident_light_index)
    {
        brx_float3 N = surface_shading_normal_world_space;
        brx_float3 L = brx_normalize(Ls[incident_light_index]);
        brx_float NdotL = brx_dot(N, L);

        brx_branch if (NdotL > 0.0)
        {
            brx_float3 incident_illuminance = incident_illuminances[incident_light_index];

            brx_float3 V = -camera_ray_direction;
            brx_float NdotV = brx_dot(N, V);

            // brx_branch if (NdotV > 0.0)
            {
                brx_float3 diffuse_brdf = brx_normalized_clamped_cosine_brdf(surface_diffuse_color);

                brx_float3 specular_brdf;
                {
                    brx_float VdotL = brx_dot(V, L);

                    // Real-Time Rendering Fourth Edition / 9.8 BRDF Models for Surface Reflection / [Hammon 2017]
                    // UE4: [Init](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L31)
                    // U3D: [GetBSDFAngle](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L361)
                    brx_float invLenH = brx_rsqrt(2.0 + 2.0 * VdotL);
                    brx_float NdotH = brx_clamp((NdotL + NdotV) * invLenH, 0.0, 1.0);
                    brx_float LdotH = brx_clamp(invLenH * VdotL + invLenH, 0.0, 1.0);

                    brx_float VdotH = LdotH;

                    // Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the surface_roughness control to users as g = r2, where r is the user-interface surface_roughness parameter value between 0 and 1."
                    brx_float alpha = surface_roughness * surface_roughness;

                    brx_float3 f0 = surface_specular_color;
                    const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

                    specular_brdf = brx_trowbridge_reitz_brdf(alpha, NdotH, NdotV, NdotL, f0, f90, VdotH);
                }

                direct_radiance += (diffuse_brdf + specular_brdf) * (NdotL * incident_illuminance);
            }
        }
    }
#endif

#if 0
    const brx_int outgoing_direction_count = 6;

    const brx_float3 outgoing_directions[outgoing_direction_count] =
        brx_array_constructor_begin(brx_float3, outgoing_direction_count)
        // 0
        brx_float3(1.0, 0.0, 0.0) brx_array_constructor_split
        // 1
        brx_float3(0.0, 1.0, 0.0) brx_array_constructor_split
        // 2
        brx_float3(0.0, 0.0, 1.0) brx_array_constructor_split
        // 3
        brx_float3(-1.0, 0.0, 0.0) brx_array_constructor_split
        // 4
        brx_float3(0.0, -1.0, 0.0) brx_array_constructor_split
        // 5
        brx_float3(0.0, 0.0, -1.0)
        brx_array_constructor_end;

    brx_unroll for (brx_int outgoing_direction_index = 0; outgoing_direction_index < outgoing_direction_count; ++outgoing_direction_index)
    {
        brx_float3 L = surface_shading_normal_world_space;

        brx_float3 V = outgoing_directions[outgoing_direction_index];
        brx_float3 N = surface_shading_normal_world_space;
        brx_float NdotL = brx_dot(N, L);
        brx_float NdotV = brx_dot(N, V);

        brx_branch if ((NdotL > 0.0) && (NdotV > 0.0))
        {
            brx_float3 specular_brdf;
            {
                brx_float VdotL = brx_dot(V, L);

                // Real-Time Rendering Fourth Edition / 9.8 BRDF Models for Surface Reflection / [Hammon 2017]
                // UE4: [Init](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L31)
                // U3D: [GetBSDFAngle](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L361)
                brx_float invLenH = brx_rsqrt(2.0 + 2.0 * VdotL);
                brx_float NdotH = brx_clamp((NdotL + NdotV) * invLenH, 0.0, 1.0);
                brx_float LdotH = brx_clamp(invLenH * VdotL + invLenH, 0.0, 1.0);

                brx_float VdotH = LdotH;

                // Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the surface_roughness control to users as g = r2, where r is the user-interface surface_roughness parameter value between 0 and 1."
                brx_float alpha = surface_roughness * surface_roughness;

                brx_float3 f0 = surface_specular_color;
                const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

                specular_brdf = brx_trowbridge_reitz_brdf(alpha, NdotH, NdotV, NdotL, f0, f90, VdotH);
            }

            direct_radiance += specular_brdf;
        }
    }
#endif

    direct_radiance += surface_emissive;

    // Environment Lighting
    brx_float3 ambient_radiance;
    brx_branch if ((ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR == g_environment_map_layout) || (ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL == g_environment_map_layout))
    {
        brx_float3 environment_map_sh_coefficients[BRX_SH_COEFFICIENT_COUNT];
        for (brx_int environment_map_sh_coefficient_index = 0; environment_map_sh_coefficient_index < BRX_SH_COEFFICIENT_COUNT; ++environment_map_sh_coefficient_index)
        {
            environment_map_sh_coefficients[environment_map_sh_coefficient_index] = brx_uint_as_float(brx_byte_address_buffer_load3(t_environment_map_sh_coefficients, (3 * (environment_map_sh_coefficient_index << 2))));
        }

        brx_float3 outgoing_direction_environment_map_space = brx_mul(g_world_to_environment_map_transform, brx_float4(-camera_ray_direction, 0.0)).xyz;

        brx_float3 shading_normal_environment_map_space = brx_mul(g_world_to_environment_map_transform, brx_float4(surface_shading_normal_world_space, 0.0)).xyz;

        brx_float3 surface_diffuse_albedo = surface_diffuse_color;

        brx_float3 surface_specular_albedo;
        brx_float non_rotation_transfer_function_lut_sh_coefficients[BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT];
        {
            brx_float surface_alpha = brx_max(brx_float(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM), surface_roughness * surface_roughness);
            brx_float NdotV = brx_max(brx_float(BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM), brx_dot(shading_normal_environment_map_space, outgoing_direction_environment_map_space));

            brx_float2 raw_lut_uv = brx_float2(brx_max(0.0, 1.0 - NdotV), brx_max(0.0, 1.0 - surface_alpha));

            {
                // Remap: [0, 1] -> [0.5/size, 1.0 - 0.5/size]
                // U3D: [Remap01ToHalfTexelCoord](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl#L661)
                // UE4: [N/A](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/RectLight.ush#L450)
                brx_int2 specular_hdr_fresnel_factors_lut_dimension = brx_texture_2d_get_dimension(t_lut_specular_hdr_fresnel_factors, 0);
                brx_float2 specular_hdr_fresnel_factors_lut_uv = (brx_float2(0.5, 0.5) + brx_float2(specular_hdr_fresnel_factors_lut_dimension.x - 1, specular_hdr_fresnel_factors_lut_dimension.y - 1) * raw_lut_uv) / brx_float2(specular_hdr_fresnel_factors_lut_dimension.x, specular_hdr_fresnel_factors_lut_dimension.y);

                brx_float2 fresnel_factor = brx_sample_level_2d(t_lut_specular_hdr_fresnel_factors, s_sampler, specular_hdr_fresnel_factors_lut_uv, 0.0).xy;
                brx_float f0_factor = fresnel_factor.x;
                brx_float f90_factor = fresnel_factor.y;

                // UE4: [EnvBRDF](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L476)
                brx_float3 f0 = surface_specular_color;
                brx_float f90 = brx_clamp(50.0 * f0.g, 0.0, 1.0);

                // UE4: [EnvBRDF](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L471)
                // U3D: [GetPreIntegratedFGDGGXAndDisneyDiffuse](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/PreIntegratedFGD/PreIntegratedFGD.hlsl#L8)
                surface_specular_albedo = f0 * f0_factor + brx_float3(f90, f90, f90) * f90_factor;
            }

            {
                // Remap: [0, 1] -> [0.5/size, 1.0 - 0.5/size]
                // U3D: [Remap01ToHalfTexelCoord](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl#L661)
                // UE4: [N/A](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/RectLight.ush#L450)
                brx_int3 specular_transfer_function_sh_coefficients_lut_dimension = brx_texture_2d_array_get_dimension(t_lut_specular_transfer_function_sh_coefficients, 0);
                brx_float2 specular_transfer_function_sh_coefficients_lut_uv = (brx_float2(0.5, 0.5) + brx_float2(specular_transfer_function_sh_coefficients_lut_dimension.x - 1, specular_transfer_function_sh_coefficients_lut_dimension.y - 1) * raw_lut_uv) / brx_float2(specular_transfer_function_sh_coefficients_lut_dimension.x, specular_transfer_function_sh_coefficients_lut_dimension.y);

                brx_unroll for (brx_int transfer_function_lut_sh_coefficient_index = 0; transfer_function_lut_sh_coefficient_index < BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT; ++transfer_function_lut_sh_coefficient_index)
                {
                    non_rotation_transfer_function_lut_sh_coefficients[transfer_function_lut_sh_coefficient_index] = brx_sample_level_2d_array(t_lut_specular_transfer_function_sh_coefficients, s_sampler, brx_float3(specular_transfer_function_sh_coefficients_lut_uv, brx_float(transfer_function_lut_sh_coefficient_index)), 0.0).x;
                }
            }
        }

        brx_float3 environment_lighting_diffuse_radiance = brx_sh_diffuse_radiance(surface_diffuse_albedo, shading_normal_environment_map_space, environment_map_sh_coefficients);

        brx_float3 environment_lighting_specular_radiance = brx_sh_specular_radiance(surface_specular_albedo, outgoing_direction_environment_map_space, shading_normal_environment_map_space, non_rotation_transfer_function_lut_sh_coefficients, environment_map_sh_coefficients);

        ambient_radiance = environment_lighting_diffuse_radiance + environment_lighting_specular_radiance;
    }
    else
    {
        ambient_radiance = brx_float3(0.0, 0.0, 0.0);
    }

    out_direct_radiance = brx_float4(direct_radiance, surface_opacity);

    out_ambient_radiance = brx_float4(ambient_radiance, 1.0);

    out_gbuffer_normal = brx_float4(brx_octahedral_map(surface_shading_normal_world_space), 0.0, 0.0);
    out_gbuffer_base_color = brx_float4(surface_base_color, 1.0);
    out_gbuffer_roughness_metallic = brx_float4(surface_roughness, surface_metallic, 0.0, 0.0);
}
