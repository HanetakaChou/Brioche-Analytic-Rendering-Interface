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

#include "deforming_resource_binding.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_packed_vector.bsli"
#include "../../Brioche-Shader-Language/shaders/brx_octahedral_mapping.bsli"
#if defined(GL_SPIRV) || defined(VULKAN)
#define brx_dual_quaternion mat2x4
#include "../../DLB/DLB.glsli"
#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)
#define brx_dual_quaternion float2x4
#include "../../DLB/DLB.hlsli"
#else
#error Unknown Compiler
#endif

#define THREAD_GROUP_X 1
#define THREAD_GROUP_Y 1
#define THREAD_GROUP_Z 1

#define INTERNAL_WEIGHT_EPSILON 1E-6f

brx_root_signature(deforming_root_signature_macro, deforming_root_signature_name)
brx_num_threads(THREAD_GROUP_X, THREAD_GROUP_Y, THREAD_GROUP_Z)
brx_compute_shader_parameter_begin(main)
brx_compute_shader_parameter_in_group_id
brx_pixel_shader_parameter_end(main)
{
    brx_uint max_vertex_position_count = brx_byte_address_buffer_get_dimension(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_VERTEX_POSITION_BUFFER_INDEX]) / SURFACE_VERTEX_POSITION_BUFFER_STRIDE;

    brx_uint max_vertex_varying_count = brx_byte_address_buffer_get_dimension(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_VERTEX_VARYING_BUFFER_INDEX]) / SURFACE_VERTEX_VARYING_BUFFER_STRIDE;

    brx_branch
    if (max_vertex_position_count != max_vertex_varying_count)
    {
        return;
    }

    brx_uint vertex_index = brx_group_id.x + DEFORMING_MAX_COMPUTE_DISPATCH_THREAD_GROUPS_PER_DIMENSION * brx_group_id.y;

    brx_branch
    if ((vertex_index >= max_vertex_position_count) || (vertex_index >= max_vertex_varying_count))
    {
        return;
    }

    brx_uint vertex_position_buffer_offset = SURFACE_VERTEX_POSITION_BUFFER_STRIDE * vertex_index;

    brx_uint vertex_varying_buffer_offset = SURFACE_VERTEX_VARYING_BUFFER_STRIDE * vertex_index;

    brx_float3 vertex_position_model_space;
    {
        brx_uint3 packed_vector_vertex_position_binding = brx_byte_address_buffer_load3(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_VERTEX_POSITION_BUFFER_INDEX], vertex_position_buffer_offset);
        vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_binding);
    }

    brx_float3 vertex_normal_model_space;
    brx_float4 vertex_tangent_model_space;
    brx_float2 vertex_texcoord;
    {
        brx_uint3 packed_vector_vertex_varying_binding = brx_byte_address_buffer_load3(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_VERTEX_VARYING_BUFFER_INDEX], vertex_varying_buffer_offset);
        vertex_normal_model_space = brx_octahedral_unmap(brx_R16G16_SNORM_to_FLOAT2(packed_vector_vertex_varying_binding.x));
        brx_float3 vertex_mapped_tangent_model_space = brx_R15G15B2_SNORM_to_FLOAT3(packed_vector_vertex_varying_binding.y);
        vertex_tangent_model_space = brx_float4(brx_octahedral_unmap(vertex_mapped_tangent_model_space.xy), vertex_mapped_tangent_model_space.z);
        vertex_texcoord = brx_R16G16_UNORM_to_FLOAT2(packed_vector_vertex_varying_binding.z);
    }

    // morph target
    brx_float3 morphed_vertex_position_model_space = vertex_position_model_space;
    brx_float3 morphed_vertex_normal_model_space = vertex_normal_model_space;
    brx_float4 morphed_vertex_tangent_model_space = vertex_tangent_model_space;
    brx_float2 morphed_vertex_texcoord = vertex_texcoord;

    brx_unroll
    for (brx_uint morph_target_weight_index = 0u; morph_target_weight_index < MORPH_TARGET_WEIGHT_COUNT; ++morph_target_weight_index)
    {
        float morph_target_weight;
        {
            brx_uint packed_vector_index = morph_target_weight_index / 4u;
            brx_uint component_index = morph_target_weight_index % 4u;

            switch (component_index)
            {
            case 0u:
            {
                morph_target_weight = g_packed_vector_morph_target_weights[packed_vector_index].x;
            }
            break;
            case 1u:
            {
                morph_target_weight = g_packed_vector_morph_target_weights[packed_vector_index].y;
            }
            break;
            case 2u:
            {
                morph_target_weight = g_packed_vector_morph_target_weights[packed_vector_index].z;
            }
            break;
            default:
            {
                morph_target_weight = g_packed_vector_morph_target_weights[packed_vector_index].w;
            }
            }
        }

        brx_uint max_morph_target_vertex_position_count = brx_byte_address_buffer_get_dimension(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_weight_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_POSITION_BUFFER_INDEX]) / SURFACE_VERTEX_POSITION_BUFFER_STRIDE;

        brx_branch
        if (max_morph_target_vertex_position_count == max_vertex_position_count)
        {
            brx_branch
            if (morph_target_weight > INTERNAL_WEIGHT_EPSILON)
            {
                brx_float3 morph_target_vertex_position_model_space;
                {
                    brx_uint3 packed_vector_morph_target_vertex_position_binding = brx_byte_address_buffer_load3(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_weight_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_POSITION_BUFFER_INDEX], vertex_position_buffer_offset);
                    morph_target_vertex_position_model_space = brx_uint_as_float(packed_vector_morph_target_vertex_position_binding);
                }

                morphed_vertex_position_model_space += morph_target_vertex_position_model_space * morph_target_weight;
            }
        }

        brx_uint max_morph_target_vertex_varying_count = brx_byte_address_buffer_get_dimension(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_weight_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_VARYING_BUFFER_INDEX]) / SURFACE_VERTEX_VARYING_BUFFER_STRIDE;

        brx_branch
        if (max_morph_target_vertex_varying_count == max_vertex_varying_count)
        {
            brx_branch
            if (morph_target_weight > INTERNAL_WEIGHT_EPSILON)
            {
                brx_float3 morph_target_vertex_normal_model_space;
                brx_float4 morph_target_vertex_tangent_model_space;
                brx_float2 morph_target_vertex_texcoord;
                {
                    brx_uint3 packed_vector_morph_target_vertex_varying_binding = brx_byte_address_buffer_load3(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_BUFFER_COUNT + DEFORMING_SURFACE_INPUT_MORPH_TARGET_BUFFER_COUNT * morph_target_weight_index + DEFORMING_SURFACE_INPUT_MORPH_TARGET_VERTEX_VARYING_BUFFER_INDEX], vertex_varying_buffer_offset);
                    morph_target_vertex_normal_model_space = brx_octahedral_unmap(brx_R16G16_SNORM_to_FLOAT2(packed_vector_morph_target_vertex_varying_binding.x));
                    brx_float3 morph_target_vertex_mapped_tangent_model_space = brx_R15G15B2_SNORM_to_FLOAT3(packed_vector_morph_target_vertex_varying_binding.y);
                    morph_target_vertex_tangent_model_space = brx_float4(brx_octahedral_unmap(morph_target_vertex_mapped_tangent_model_space.xy), morph_target_vertex_mapped_tangent_model_space.z);
                    morph_target_vertex_texcoord = brx_R16G16_UNORM_to_FLOAT2(packed_vector_morph_target_vertex_varying_binding.z);
                }

                morphed_vertex_normal_model_space += (morph_target_vertex_normal_model_space - vertex_normal_model_space) * morph_target_weight;
                morphed_vertex_tangent_model_space += (morph_target_vertex_tangent_model_space - vertex_tangent_model_space) * morph_target_weight;
                morphed_vertex_texcoord += morph_target_vertex_texcoord * morph_target_weight;
            }
        }
    }

    morphed_vertex_normal_model_space = brx_normalize(morphed_vertex_normal_model_space);

    morphed_vertex_tangent_model_space.xyz = brx_normalize(morphed_vertex_tangent_model_space.xyz);

    // skin

    brx_uint4 joint_indices;
    brx_float4 joint_weights;
    {
        brx_uint vertex_joint_buffer_offset = SURFACE_VERTEX_BLENDING_BUFFER_STRIDE * vertex_index;
        brx_uint3 packed_vector_vertex_joint_buffer = brx_byte_address_buffer_load3(g_surface_input_buffers[DEFORMING_SURFACE_INPUT_VERTEX_BLENDING_BUFFER_INDEX], vertex_joint_buffer_offset);
        joint_indices = brx_R16G16B16A16_UINT_to_UINT4(packed_vector_vertex_joint_buffer.xy);
        joint_weights = brx_R8G8B8A8_UNORM_to_FLOAT4(packed_vector_vertex_joint_buffer.z);
    }

    brx_dual_quaternion blend_dual_quaternion;
    {
        brx_dual_quaternion dual_quaternion_indices_x = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.x], g_dual_quaternions[2u * joint_indices.x + 1u]);
        brx_dual_quaternion dual_quaternion_indices_y = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.y], g_dual_quaternions[2u * joint_indices.y + 1u]);
        brx_dual_quaternion dual_quaternion_indices_z = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.z], g_dual_quaternions[2u * joint_indices.z + 1u]);
        brx_dual_quaternion dual_quaternion_indices_w = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.w], g_dual_quaternions[2u * joint_indices.w + 1u]);
        blend_dual_quaternion = dual_quaternion_linear_blending(dual_quaternion_indices_x, dual_quaternion_indices_y, dual_quaternion_indices_z, dual_quaternion_indices_w, joint_weights);
    }

    brx_float4 blend_quaternion;
    brx_float3 blend_translation;
    unit_dual_quaternion_to_rigid_transform(blend_dual_quaternion, blend_quaternion, blend_translation);

    brx_float3 skined_vertex_position_model_space = unit_quaternion_to_rotation_transform(blend_quaternion, morphed_vertex_position_model_space) + blend_translation;

    brx_float3 skined_vertex_normal_model_space = unit_quaternion_to_rotation_transform(blend_quaternion, morphed_vertex_normal_model_space);
    brx_float4 skined_vertex_tangent_model_space = brx_float4(unit_quaternion_to_rotation_transform(blend_quaternion, morphed_vertex_tangent_model_space.xyz), morphed_vertex_tangent_model_space.w);

    brx_uint3 packed_vector_skined_vertex_position_binding = brx_float_as_uint(skined_vertex_position_model_space);
    brx_uint3 packed_vector_skined_vertex_varying_binding = brx_uint3(brx_FLOAT2_to_R16G16_SNORM(brx_octahedral_map(skined_vertex_normal_model_space)), brx_FLOAT3_to_R15G15B2_SNORM(brx_float3(brx_octahedral_map(skined_vertex_tangent_model_space.xyz), skined_vertex_tangent_model_space.w)), brx_FLOAT2_to_R16G16_UNORM(morphed_vertex_texcoord));

    brx_byte_address_buffer_store3(g_surface_output_buffers[DEFORMING_SURFACE_OUTPUT_VERTEX_POSITION_BUFFER_INDEX], vertex_position_buffer_offset, packed_vector_skined_vertex_position_binding);
    brx_byte_address_buffer_store3(g_surface_output_buffers[DEFORMING_SURFACE_OUTPUT_VERTEX_VARYING_BUFFER_INDEX], vertex_varying_buffer_offset, packed_vector_skined_vertex_varying_binding);
}
