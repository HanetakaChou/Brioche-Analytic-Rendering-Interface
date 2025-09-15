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
#include "../../Environment-Lighting/shaders/brx_environment_lighting_sh_projection_clear.bsli"

void brx_vct_application_bridge_set_sh_irradiance_coefficient(in brx_int in_sh_irradiance_coefficient_monochromatic_index, in brx_uint in_sh_coefficient)
{
    brx_byte_address_buffer_store(u_environment_lighting_sh_irradiance_coefficients, (in_sh_irradiance_coefficient_monochromatic_index << 2), in_sh_coefficient);
}