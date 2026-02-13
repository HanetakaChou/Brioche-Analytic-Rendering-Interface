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
#include "../../Brioche-Shader-Language/include/brx_reversed_z.h"

void brx_anari_pal_device::camera_set(brx_anari_vec3 position, brx_anari_vec3 direction, brx_anari_vec3 up, float fovy, float near, float far)
{
    this->m_camera_position = position;
    {
        DirectX::XMFLOAT3 const camera_direction(direction.m_x, direction.m_y, direction.m_z);
        DirectX::XMFLOAT3 normalized_camera_direction;
        DirectX::XMStoreFloat3(&normalized_camera_direction, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&camera_direction)));
        this->m_camera_direction = brx_anari_vec3{normalized_camera_direction.x, normalized_camera_direction.y, normalized_camera_direction.z};
    }
    {
        DirectX::XMFLOAT3 const camera_up(up.m_x, up.m_y, up.m_z);
        DirectX::XMFLOAT3 normalized_camera_up;
        DirectX::XMStoreFloat3(&normalized_camera_up, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&camera_up)));
        this->m_camera_up = brx_anari_vec3{normalized_camera_up.x, normalized_camera_up.y, normalized_camera_up.z};
    }
    this->m_camera_fovy = std::min(std::max(0.01745329251994329576923690768489F, fovy), 3.1241393610698499426934064755946F);
    this->m_camera_near = std::max(0.001F, near);
    this->m_camera_far = std::max(std::max(0.001F, near) + 0.001F, far);
}

void brx_anari_pal_device::camera_upload_none_update_set_uniform_buffer(none_update_set_uniform_buffer_binding *none_update_set_uniform_buffer_destination)
{
    DirectX::XMFLOAT4X4 camera_view_transform;
    DirectX::XMFLOAT4X4 camera_projection_transform;
    DirectX::XMFLOAT4X4 camera_inverse_view_transform;
    DirectX::XMFLOAT4X4 camera_inverse_projection_transform;
    {
        {
            DirectX::XMFLOAT3 const camera_position(this->m_camera_position.m_x, this->m_camera_position.m_y, this->m_camera_position.m_z);
            DirectX::XMFLOAT3 const camera_direction(this->m_camera_direction.m_x, this->m_camera_direction.m_y, this->m_camera_direction.m_z);
            DirectX::XMFLOAT3 const camera_up(this->m_camera_up.m_x, this->m_camera_up.m_y, this->m_camera_up.m_z);

            DirectX::XMMATRIX simd_camera_view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&camera_position), DirectX::XMLoadFloat3(&camera_direction), DirectX::XMLoadFloat3(&camera_up));
            DirectX::XMStoreFloat4x4(&camera_view_transform, simd_camera_view_transform);

            DirectX::XMVECTOR unused_determinant;
            DirectX::XMMATRIX simd_inverse_view_transform = DirectX::XMMatrixInverse(&unused_determinant, simd_camera_view_transform);
            DirectX::XMStoreFloat4x4(&camera_inverse_view_transform, simd_inverse_view_transform);
        }

        {
            float const aspect = static_cast<float>(this->m_intermediate_width) / static_cast<float>(this->m_intermediate_height);

            DirectX::XMMATRIX simd_camera_projection_transform = brx_DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(this->m_camera_fovy, aspect, this->m_camera_near, this->m_camera_far);
            DirectX::XMStoreFloat4x4(&camera_projection_transform, simd_camera_projection_transform);

            DirectX::XMVECTOR unused_determinant;
            DirectX::XMMATRIX simd_camera_inverse_projection_transform = DirectX::XMMatrixInverse(&unused_determinant, simd_camera_projection_transform);
            DirectX::XMStoreFloat4x4(&camera_inverse_projection_transform, simd_camera_inverse_projection_transform);
        }
    }

    none_update_set_uniform_buffer_destination->g_view_transform = camera_view_transform;
    none_update_set_uniform_buffer_destination->g_projection_transform = camera_projection_transform;
    none_update_set_uniform_buffer_destination->g_inverse_view_transform = camera_inverse_view_transform;
    none_update_set_uniform_buffer_destination->g_inverse_projection_transform = camera_inverse_projection_transform;
}
