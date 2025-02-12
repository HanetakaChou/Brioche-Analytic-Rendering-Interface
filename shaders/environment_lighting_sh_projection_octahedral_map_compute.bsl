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

#include "environment_lighting_resource_binding.bsli"
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT_OCTAHEDRAL_MAP
#include "../../Environment-Lighting/shaders/brx_environment_lighting_sh_projection_reduce.bsli"

brx_int2 brx_vct_application_bridge_get_environment_map_dimension()
{
    return brx_texture_2d_get_dimension(t_environment_map, 0);
}

brx_float3 brx_vct_application_bridge_get_environment_map_lighting(in brx_int2 texture_coordinates)
{
    return brx_fetch_2d(t_environment_map, brx_int3(texture_coordinates, 0)).rgb;
}

brx_uint brx_vct_application_bridge_get_sh_irradiance_coefficient(in brx_int in_sh_irradiance_coefficient_monochromatic_index)
{
    return brx_byte_address_buffer_load(u_environment_lighting_sh_irradiance_coefficients, (in_sh_irradiance_coefficient_monochromatic_index << 2));
}

brx_uint brx_vct_application_bridge_compare_and_swap_sh_irradiance_coefficient(in brx_int in_sh_irradiance_coefficient_monochromatic_index, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_byte_address_buffer_interlocked_compare_exchange(u_environment_lighting_sh_irradiance_coefficients, (in_sh_irradiance_coefficient_monochromatic_index << 2), in_old_value, in_new_value);
}
