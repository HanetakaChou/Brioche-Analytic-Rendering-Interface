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

#include "brx_anari_pal_device.h"

void brx_anari_pal_device::directional_light_set(bool visible, brx_anari_vec3 irradiance, brx_anari_vec3 direction)
{
    this->m_directional_light_visible = visible;
    this->m_directional_light_irradiance = irradiance;
    this->m_directional_light_direction = direction;
}

void brx_anari_pal_device::directional_light_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_binding *none_update_set_uniform_buffer_destination)
{
    none_update_set_uniform_buffer_destination->g_directional_lighting_visible = this->m_directional_light_visible ? 1U : 0U;
    none_update_set_uniform_buffer_destination->g_directional_lighting_irradiance = DirectX::XMFLOAT3(this->m_directional_light_irradiance.m_x, this->m_directional_light_irradiance.m_y, this->m_directional_light_irradiance.m_z);
    none_update_set_uniform_buffer_destination->g_directional_lighting_direction = DirectX::XMFLOAT3(this->m_directional_light_direction.m_x, this->m_directional_light_direction.m_y, this->m_directional_light_direction.m_z);
}
