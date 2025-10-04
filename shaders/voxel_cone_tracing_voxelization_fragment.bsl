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

#include "../../Spherical-Harmonic/include/brx_spherical_harmonic.h"
#define BRX_VCT_ENABLE_ILLUMINATION 1
#define BRX_VCT_VOXELIZATION_MAXIMUM_ENVIRONMENT_LIGHTING_ILLUMINATION_FLOAT_COUNT BRX_SH_COEFFICIENT_COUNT
#define BRX_VCT_VOXELIZATION_MAXIMUM_DIRECT_LIGHTING_COUNT 1
#include "../../Voxel-Cone-Tracing/shaders/brx_voxel_cone_tracing_voxelization_fragment.bsli"

#include "../../Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_brdf.bsli"
#include "../../Spherical-Harmonic/include/brx_spherical_harmonic_projection_transfer_function.h"
#include "../../Spherical-Harmonic/shaders/brx_spherical_harmonic_diffuse_radiance.bsli"
#include "../../Spherical-Harmonic/shaders/brx_spherical_harmonic_specular_radiance.bsli"

brx_bool brx_vct_application_bridge_get_surface(in brx_float3 in_normal, in brx_float4 in_tangent, in brx_float2 in_texcoord, out brx_float3 out_surface_shading_normal_world_space, out brx_float3 out_surface_diffuse_color, out brx_float3 out_surface_specular_color, out brx_float out_surface_roughness, out brx_float out_surface_opacity, out brx_float3 out_surface_emissive)
{
    brx_uint buffer_texture_flags;
    brx_float normal_scale;
    brx_float4 base_color_factor;
    brx_float roughness_factor;
    brx_float metallic_factor;
    brx_float3 emissive_factor;
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

    brx_float2 texcoord = in_texcoord;

    brx_float3 geometry_normal_world_space = brx_normalize(in_normal);

    brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_NORMAL))
    {
        brx_float3 tangent_world_space = brx_normalize(in_tangent.xyz);

        brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space) * ((in_tangent.w >= 0.0) ? 1.0 : -1.0);

        // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
        brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_NORMAL_TEXTURE_INDEX], s_sampler, texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_scale, normal_scale, 1.0));

        out_surface_shading_normal_world_space = brx_normalize(tangent_world_space * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);
    }
    else
    {
        out_surface_shading_normal_world_space = geometry_normal_world_space;
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

    brx_float3 surface_base_color = base_color_and_opacity.xyz;

    // usually the vertices within the same model is organized from back to front
    // we can simply use the over operation to render the result correctly (just like how we render the imgui)
    out_surface_opacity = base_color_and_opacity.w;

    brx_float surface_metallic;
    brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_METALLIC_ROUGHNESS))
    {
        brx_float2 roughness_metallic = brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_METALLIC_ROUGHNESS_TEXTURE_INDEX], s_sampler, texcoord).yz;
        out_surface_roughness = roughness_factor * roughness_metallic.x;
        surface_metallic = metallic_factor * roughness_metallic.y;
    }
    else
    {
        out_surface_roughness = roughness_factor;
        surface_metallic = metallic_factor;
    }

    {
        const brx_float dielectric_specular = 0.04;
        // UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
        out_surface_specular_color = brx_clamp((dielectric_specular - dielectric_specular * surface_metallic) + surface_base_color * surface_metallic, 0.0, 1.0);
        out_surface_diffuse_color = brx_clamp(surface_base_color - surface_base_color * surface_metallic, 0.0, 1.0);
    }

    brx_branch if (0u != (buffer_texture_flags & SURFACE_TEXTURE_FLAG_EMISSIVE))
    {
        out_surface_emissive = emissive_factor * brx_sample_2d(t_surface_images[FORWARD_SHADING_SURFACE_EMISSIVE_TEXTURE_INDEX], s_sampler, texcoord).xyz;
    }
    else
    {
        out_surface_emissive = emissive_factor;
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

        out_surface_roughness = brx_max(out_surface_roughness, geometric_aa_roughness);
    }

    // Prevent the roughness to be zero
    // https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/CapsuleLightIntegrate.ush#L94
    {
        const brx_float cvar_global_min_roughness_override = 0.02;
        out_surface_roughness = brx_max(out_surface_roughness, cvar_global_min_roughness_override);
    }

    return true;
}

void brx_vct_application_bridge_get_environment_lighting_illumination(in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness, out brx_float3 out_environment_lighting_illumination[BRX_VCT_VOXELIZATION_MAXIMUM_ENVIRONMENT_LIGHTING_ILLUMINATION_FLOAT_COUNT])
{
    brx_branch if ((ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR == g_environment_map_layout) || (ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL == g_environment_map_layout))
    {
        for (brx_int environment_map_sh_coefficient_index = 0; environment_map_sh_coefficient_index < BRX_SH_COEFFICIENT_COUNT; ++environment_map_sh_coefficient_index)
        {
            out_environment_lighting_illumination[environment_map_sh_coefficient_index] = brx_uint_as_float(brx_byte_address_buffer_load3(t_environment_map_sh_coefficients, (3 * (environment_map_sh_coefficient_index << 2))));
        }
    }
    else
    {
        for (brx_int environment_map_sh_coefficient_index = 0; environment_map_sh_coefficient_index < BRX_SH_COEFFICIENT_COUNT; ++environment_map_sh_coefficient_index)
        {
            out_environment_lighting_illumination[environment_map_sh_coefficient_index] = brx_float3(0.0, 0.0, 0.0);
        }
    }
}

brx_float3 brx_vct_application_bridge_get_environment_lighting_radiance(in brx_float3 in_environment_lighting_illumination[BRX_VCT_VOXELIZATION_MAXIMUM_ENVIRONMENT_LIGHTING_ILLUMINATION_FLOAT_COUNT], in brx_float3 in_outgoing_direction_world_space, in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    brx_float3 environment_lighting_radiance;
    brx_branch if ((ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR == g_environment_map_layout) || (ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL == g_environment_map_layout))
    {
        brx_float3 outgoing_direction_environment_map_space = brx_mul(g_world_to_environment_map_transform, brx_float4(in_outgoing_direction_world_space, 0.0)).xyz;

        brx_float3 shading_normal_environment_map_space = brx_mul(g_world_to_environment_map_transform, brx_float4(in_surface_shading_normal_world_space, 0.0)).xyz;

        brx_float3 surface_diffuse_albedo = in_surface_diffuse_color;

        brx_float3 surface_specular_albedo;
        brx_float non_rotation_transfer_function_lut_sh_coefficients[BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT];
        {
            brx_float surface_alpha = brx_max(brx_float(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM), in_surface_roughness * in_surface_roughness);
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
                brx_float3 f0 = in_surface_specular_color;
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

        brx_float3 environment_lighting_diffuse_radiance = brx_sh_diffuse_radiance(surface_diffuse_albedo, shading_normal_environment_map_space, in_environment_lighting_illumination);

        brx_float3 environment_lighting_specular_radiance = brx_sh_specular_radiance(surface_specular_albedo, outgoing_direction_environment_map_space, shading_normal_environment_map_space, non_rotation_transfer_function_lut_sh_coefficients, in_environment_lighting_illumination);

        environment_lighting_radiance = environment_lighting_diffuse_radiance + environment_lighting_specular_radiance;
    }
    else
    {
        environment_lighting_radiance = brx_float3(0.0, 0.0, 0.0);
    }

    return environment_lighting_radiance;
}

brx_int brx_vct_application_bridge_get_direct_lighting_count(in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    return 0;
}

brx_float3 brx_vct_application_bridge_get_direct_lighting_illumination(in brx_int in_direct_lighting_index, in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    return brx_float3(0.0, 0.0, 0.0);
}

brx_float3 brx_vct_application_bridge_get_direct_lighting_radiance(in brx_int in_direct_lighting_index, in brx_float3 in_direct_lighting_illumination, in brx_float3 in_outgoing_direction_world_space, in brx_float3 in_surface_position_world_space, in brx_float3 in_surface_shading_normal_world_space, in brx_float3 in_surface_diffuse_color, in brx_float3 in_surface_specular_color, in brx_float in_surface_roughness)
{
    return brx_float3(0.0, 0.0, 0.0);
}

brx_uint brx_vct_application_bridge_get_clipmap_mask(in brx_int3 in_mask_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_mask, in_mask_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_mask(in brx_int3 in_mask_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_mask, in_mask_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_opacity(in brx_int3 in_opacity_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_opacity_r32, in_opacity_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_opacity(in brx_int3 in_opacity_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_opacity_r32, in_opacity_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_red(in brx_int3 in_illumination_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_illumination_r32, in_illumination_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_illumination_red(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_illumination_r32, in_illumination_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_green(in brx_int3 in_illumination_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_illumination_g32, in_illumination_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_illumination_green(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_illumination_g32, in_illumination_texture_coordinates, in_old_value, in_new_value);
}

brx_uint brx_vct_application_bridge_get_clipmap_illumination_blue(in brx_int3 in_illumination_texture_coordinates)
{
    return brx_load_3d_uint(u_clipmap_texture_illumination_b32, in_illumination_texture_coordinates);
}

brx_uint brx_vct_application_bridge_compare_and_swap_clipmap_illumination_blue(in brx_int3 in_illumination_texture_coordinates, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_texture_3d_uint_interlocked_compare_exchange(u_clipmap_texture_illumination_b32, in_illumination_texture_coordinates, in_old_value, in_new_value);
}
