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

#include "full_screen_transfer_resource_binding.bsli"

brx_root_signature(full_screen_transfer_root_signature_macro, full_screen_transfer_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_display_color, 0)
brx_pixel_shader_parameter_end(main)
{
	out_display_color = brx_fetch_2d(g_display_color_image, brx_int3(brx_frag_coord.xy, 0));
}
