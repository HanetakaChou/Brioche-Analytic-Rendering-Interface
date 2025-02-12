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
#include "../../Brioche-Shader-Language/shaders/brx_math_consts.bsli"

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
    brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);
    brx_float4 tangent_world_space = brx_float4(brx_normalize(in_interpolated_tangent.xyz), in_interpolated_tangent.w);
    brx_float2 texcoord = in_interpolated_texcoord;
    
    brx_uint buffer_texture_flags;
	brx_float3 emissive_factor;
    brx_float normal_scale;
    brx_float3 base_color_factor;
    brx_float metallic_factor;
    brx_float roughness_factor;
    {
        brx_uint4 auxiliary_packed_vectors[3];
        auxiliary_packed_vectors[0] = brx_byte_address_buffer_load4(g_surface_buffers[Forward_Shading_Surface_Buffer_Auxiliary_Index], 0);
        auxiliary_packed_vectors[1] = brx_byte_address_buffer_load4(g_surface_buffers[Forward_Shading_Surface_Buffer_Auxiliary_Index], 4 * 4);
        auxiliary_packed_vectors[2].xy = brx_byte_address_buffer_load2(g_surface_buffers[Forward_Shading_Surface_Buffer_Auxiliary_Index], 4 * 4 + 4 * 4);
        buffer_texture_flags = auxiliary_packed_vectors[0].x;
        emissive_factor = brx_uint_as_float(auxiliary_packed_vectors[0].yzw);
        normal_scale = brx_uint_as_float(auxiliary_packed_vectors[1].x);
	    base_color_factor = brx_uint_as_float(auxiliary_packed_vectors[1].yzw);
        metallic_factor = brx_uint_as_float(auxiliary_packed_vectors[2].x);
        roughness_factor = brx_uint_as_float(auxiliary_packed_vectors[2].y);
    }

    brx_float3 emissive;
    brx_branch
    if(0u != (buffer_texture_flags & Surface_Texture_Flag_Emissive))
    {
        emissive = emissive_factor * brx_sample_2d(g_surface_images[Forward_Shading_Surface_Texture_Emissive_Index], g_shared_none_update_set_sampler, texcoord).xyz;
    }
	else
	{
		emissive = brx_float3(0.0, 0.0, 0.0);
	}

    brx_float3 shading_normal_world_space;
    brx_branch
    if(0u != (buffer_texture_flags & Surface_Texture_Flag_Normal))
    {
        // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
        brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(g_surface_images[Forward_Shading_Surface_Texture_Normal_Index], g_shared_none_update_set_sampler, texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_scale, normal_scale, 1.0));
        brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space.xyz) * tangent_world_space.w;
        shading_normal_world_space = brx_normalize(tangent_world_space.xyz * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);  
    }
    else
    {
        shading_normal_world_space = geometry_normal_world_space;
    }
  
    brx_float3 base_color;
    brx_branch
    if(0u != (buffer_texture_flags & Surface_Texture_Flag_Base_Color))
    {
        base_color = base_color_factor * brx_sample_2d(g_surface_images[Forward_Shading_Surface_Texture_Base_Color_Index], g_shared_none_update_set_sampler, texcoord).xyz;
    }
	else
	{
		base_color = base_color_factor;
	}

    brx_float metallic;
    brx_float roughness;
    brx_branch
    if(0u != (buffer_texture_flags & Surface_Texture_Flag_Metallic_Roughness))
    {
        brx_float2 metallic_roughness = brx_sample_2d(g_surface_images[Forward_Shading_Surface_Texture_Metallic_Roughness_Index], g_shared_none_update_set_sampler, texcoord).bg;
	    metallic = metallic_factor * metallic_roughness.x;
        roughness = roughness_factor * metallic_roughness.y;
    }
	else
	{
		metallic = metallic_factor;
		roughness = roughness_factor;
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

    const brx_float dielectric_specular = 0.04;
	// UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
	brx_float3 f0 = brx_clamp((dielectric_specular - dielectric_specular * metallic) + base_color * metallic, 0.0, 1.0);
	brx_float3 albedo = brx_clamp(base_color - base_color * metallic, 0.0, 1.0);

    brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;

    brx_float3 surface_position_world_space = in_interpolated_position_world_space;

	// TODO: shadow
	// TODO: environment lighting
	brx_float3 outgoing_radiance = brx_float3(0.0, 0.0, 0.0);

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

		brx_float3 V = brx_normalize(camera_ray_origin - surface_position_world_space);
		brx_float3 N = shading_normal_world_space;
		brx_float3 H = normalize(L + V);
		brx_float NdotL = brx_clamp(dot(N, L), 0.0, 1.0);
		brx_float NdotH = brx_clamp(dot(N, H), 0.0, 1.0);
		brx_float NdotV = brx_clamp(dot(N, V), 0.0, 1.0);
		brx_float VdotH = brx_clamp(dot(V, H), 0.0, 1.0);

		brx_float3 brdf_diffuse;
		{
			// Lambert

			brdf_diffuse = (1.0 / BRX_M_PI) * albedo;
		}

		brx_float3 brdf_specular;
		{
			// Trowbridge Reitz		
		
			// Prevent the roughness to be zero
			// https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/CapsuleLightIntegrate.ush#L94
			const brx_float cvar_global_min_roughness_override = 0.02;
			roughness = brx_max(roughness, cvar_global_min_roughness_override);

			// Prevent the NdotV to be zero
			// https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L34
			NdotV = brx_max(NdotV, 1e-5);

			// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as g = r2, where r is the user-interface roughness parameter value between 0 and 1."
			brx_float alpha = roughness * roughness;
	
			// Equation 9.41 of Real-Time Rendering Fourth Edition: "Although **Trowbridge-Reitz distribution** is technically the correct name"
			// Equation 8.11 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MicrofacetDistributionFunctions
			brx_float alpha2 = alpha * alpha;
			brx_float denominator = 1.0 + NdotH * (NdotH * alpha2 - NdotH);
			brx_float D = (1.0 / BRX_M_PI) * (alpha2 / (denominator * denominator));

			// Lambda:
			// Equation 8.13 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
			// Equation 9.42 of Real-Time Rendering Fourth Edition
			// Figure 8.18 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
			// Lambda(V) = 0.5*(-1.0 + (1.0/NoV)*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2))
			// Lambda(L) = 0.5*(-1.0 + (1.0/NoL)*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// G2
			// Equation 9.31 of Real-Time Rendering Fourth Edition
			// PBR Book / 8.4.3 Masking and Shadowing: "A more accurate model can be derived assuming that microfacet visibility is more likely the higher up a given point on a microface"
			// G2 = 1.0/(1.0 + Lambda(V) + Lambda(L)) = (2.0*NoV*NoL)/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// V = G2/(4.0*NoV*NoL) = 0.5/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// float alpha2 = alpha * alpha;
			// float term_v = NdotL * sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
			// float term_l = NdotV * sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
			// UE: [Vis_SmithJointApprox](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L380)
			brx_float term_v = NdotL * (alpha + (1.0 - alpha) * NdotV);
			brx_float term_l = NdotV * (alpha + (1.0 - alpha) * NdotL);
			brx_float V = (0.5 / (term_v + term_l));

			// glTF Sample Renderer: [F_Schlick](https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/e5646a2bf87b0871ba3f826fc2335fe117a11411/source/Renderer/shaders/brdf.glsl#L24)
			const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

			brx_float x = brx_clamp(1.0 - VdotH, 0.0, 1.0);
			brx_float x2 = x * x;
			brx_float x5 = x * x2 * x2;
			brx_float3 F = f0 + (f90 - f0) * x5;

			brdf_specular = D * V * F;
		}
	
		 outgoing_radiance += (brdf_diffuse + brdf_specular) * (NdotL * incident_illuminance);
	}

    outgoing_radiance += emissive;
    out_scene_color = brx_float4(outgoing_radiance, 1.0);

    brx_uint packed_shading_normal_world_space = brx_FLOAT2_to_R16G16_SNORM(brx_octahedral_map(shading_normal_world_space));
    brx_uint packed_roughness_metallic = brx_FLOAT2_to_R16G16_UNORM(brx_float2(roughness, metallic));
    out_gbuffer = brx_uint4(packed_shading_normal_world_space, packed_roughness_metallic, 0, 0);
}
