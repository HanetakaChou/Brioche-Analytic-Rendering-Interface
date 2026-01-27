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

brx_root_signature(post_processing_root_signature_macro, post_processing_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_display_color, 0)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 direct_radiance = brx_fetch_2d(t_direct_radiance, brx_int3(brx_frag_coord.xy, 0)).xyz;

    brx_float3 ambient_radiance;
    brx_branch if ((ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR == g_environment_map_layout) || (ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL == g_environment_map_layout))
    {
        ambient_radiance = brx_fetch_2d(t_ambient_radiance, brx_int3(brx_frag_coord.xy, 0)).xyz;
    }
    else
    {
        ambient_radiance = brx_float3(0.0, 0.0, 0.0);
    }

    brx_float3 indirect_radiance;
    brx_float ambient_occlusion;
    brx_branch if ((RENDERER_GI_QUALITY_LOW == g_renderer_gi_quality) || (RENDERER_GI_QUALITY_MEDIUM == g_renderer_gi_quality) || (RENDERER_GI_QUALITY_HIGH == g_renderer_gi_quality))
    {
        brx_float4 indirect_radiance_and_ambient_occlusion = brx_sample_2d(t_indirect_radiance_and_ambient_occlusion, s_clamp_sampler, in_interpolated_texcoord);
        indirect_radiance = indirect_radiance_and_ambient_occlusion.xyz;
        ambient_occlusion = indirect_radiance_and_ambient_occlusion.w;
    }
    else
    {
        indirect_radiance = brx_float3(0.0, 0.0, 0.0);
        ambient_occlusion = 1.0;
    }

    // TODO: HDR swapchain
    //
    // DXGI_COLOR_SPACE_TYPE: https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range#setting-up-your-directx-swap-chain
    // Direct3D 12 HDR sample: https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hdr-sample-win32/
    // VK_EXT_swapchain_colorspace: https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#TRANSFER_CONVERSION
    // https://developer.nvidia.com/high-dynamic-range-display-development
    // https://gpuopen.com/learn/using-amd-freesync-2-hdr-color-spaces/
    //
    // Options       | hardware OETF input | hardware OETF output | Direct3D12                                                                  | Vulkan
    // sRGB          | sRGB                | Bt709                | DXGI_FORMAT_R8G8B8A8_UNORM     + DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709    | VK_FORMAT_B8G8R8A8_UNORM           + VK_COLOR_SPACE_BT709_NONLINEAR_EXT
    // sRGB          | sRGB                | Bt709                | DXGI_FORMAT_R10G10B10A2_UNORM  + DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709    | VK_FORMAT_A2B10G10R10_UNORM_PACK32 + VK_COLOR_SPACE_BT709_NONLINEAR_EXT
    // HDR10         | ST2084              | Bt2020               | DXGI_FORMAT_R10G10B10A2_UNORM  + DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 | VK_FORMAT_A2B10G10R10_UNORM_PACK32 + VK_COLOR_SPACE_HDR10_ST2084_EXT
    // scRGB         | scRGB               | Bt709                | N/A                                                                         | VK_FORMAT_R16G16B16A16_SFLOAT      + VK_COLOR_SPACE_BT709_LINEAR_EXT
    // Linear BT2020 | Linear BT2020       | Bt2020               | DXGI_FORMAT_R16G16B16A16_FLOAT + DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709    | VK_FORMAT_R16G16B16A16_SFLOAT      + VK_COLOR_SPACE_BT2020_LINEAR_EXT

    brx_float3 color_linear = direct_radiance + indirect_radiance + ambient_radiance * ambient_occlusion;
    brx_float4 color_srgb = brx_float4(brx_pow(brx_clamp(color_linear, 0.0, 1.0), brx_float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)), 1.0);

    out_display_color = color_srgb;
}
