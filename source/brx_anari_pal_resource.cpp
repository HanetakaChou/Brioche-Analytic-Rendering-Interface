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
#include "../shaders/surface.bsli"
#include "../../Hemispherical-Directional-Reflectance/include/brx_hemispherical_directional_reflectance_look_up_table_norms.h"
#include "../../Linearly-Transformed-Cosine/include/brx_linearly_transformed_cosine_look_up_table_matrices.h"
#include "../../Spherical-Harmonic/include/brx_spherical_harmonic_look_up_table_transfer_functions.h"
#include <cstring>

static inline uint32_t internal_align_up(uint32_t value, uint32_t alignment);

void brx_anari_pal_device::helper_destroy_upload_buffer(brx_pal_uniform_upload_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_uniform_upload_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

void brx_anari_pal_device::helper_destroy_intermediate_buffer(brx_pal_storage_intermediate_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_storage_intermediate_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

void brx_anari_pal_device::helper_destroy_asset_buffer(brx_pal_storage_asset_buffer *const buffer)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != buffer);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_storage_asset_buffers[previous_frame_throttling_index].push_back(buffer);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

void brx_anari_pal_device::helper_destroy_asset_image(brx_pal_sampled_asset_image *const image)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != image);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_sampled_asset_images[previous_frame_throttling_index].push_back(image);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

void brx_anari_pal_device::helper_destroy_descriptor_set(brx_pal_descriptor_set *descriptor_set)
{
#ifndef NDEBUG
    assert(!this->m_frame_throttling_index_lock);
    this->m_frame_throttling_index_lock = true;
#endif

    assert(NULL != descriptor_set);

    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? this->m_frame_throttling_index : INTERNAL_FRAME_THROTTLING_COUNT) - 1U;

    this->m_pending_destroy_descriptor_sets[previous_frame_throttling_index].push_back(descriptor_set);

#ifndef NDEBUG
    this->m_frame_throttling_index_lock = false;
#endif
}

uint32_t brx_anari_pal_device::helper_compute_uniform_buffer_size(uint32_t buffer_size) const
{
    assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);
    return internal_align_up(buffer_size, this->m_uniform_upload_buffer_offset_alignment) * INTERNAL_FRAME_THROTTLING_COUNT;
}

void *brx_anari_pal_device::helper_compute_uniform_buffer_memory_address(uint32_t frame_throttling_index, brx_pal_uniform_upload_buffer const *uniform_upload_buffer, uint32_t buffer_size) const
{
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(uniform_upload_buffer->get_host_memory_range_base()) + helper_compute_uniform_buffer_dynamic_offset(frame_throttling_index, buffer_size));
}

uint32_t brx_anari_pal_device::helper_compute_uniform_buffer_dynamic_offset(uint32_t frame_throttling_index, uint32_t buffer_size) const
{
    assert(frame_throttling_index < INTERNAL_FRAME_THROTTLING_COUNT);
    assert(this->m_device->get_uniform_upload_buffer_offset_alignment() == this->m_uniform_upload_buffer_offset_alignment);
    return internal_align_up(buffer_size, this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index;
}

void brx_anari_pal_device::init_lut_resource()
{
    assert(0U == (g_brx_hdr_lut_width & (g_brx_hdr_lut_width - 1U)));
    assert(0U == (g_brx_hdr_lut_height & (g_brx_hdr_lut_height - 1U)));

    assert(0U == (g_brx_ltc_lut_width & (g_brx_ltc_lut_width - 1U)));
    assert(0U == (g_brx_ltc_lut_height & (g_brx_ltc_lut_height - 1U)));

    assert(0U == (g_brx_sh_lut_width & (g_brx_sh_lut_width - 1U)));
    assert(0U == (g_brx_sh_lut_height & (g_brx_sh_lut_height - 1U)));

    constexpr uint32_t const hdr_lut_mip_level_count = 1U;
    constexpr uint32_t const hdr_lut_array_layer_count = 1U;
    constexpr uint32_t const hdr_lut_subresource_count = hdr_lut_mip_level_count * hdr_lut_array_layer_count;

    constexpr uint32_t const ltc_lut_mip_level_count = 1U;
    constexpr uint32_t const ltc_lut_array_layer_count = 1U;
    constexpr uint32_t const ltc_lut_subresource_count = ltc_lut_mip_level_count * ltc_lut_array_layer_count;

    constexpr uint32_t const sh_lut_mip_level_count = 1U;
    constexpr uint32_t const sh_lut_array_layer_count = g_brx_sh_lut_array_size;
    constexpr uint32_t const sh_lut_subresource_count = sh_lut_mip_level_count * sh_lut_array_layer_count;

    BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST hdr_lut_subresource_memcpy_dests[hdr_lut_subresource_count];
    BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST ltc_lut_subresource_memcpy_dests[ltc_lut_subresource_count];
    BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST sh_lut_subresource_memcpy_dests[sh_lut_subresource_count];

    brx_pal_staging_upload_buffer *hdr_lut_image_staging_upload_buffer = NULL;
    brx_pal_staging_upload_buffer *ltc_lut_image_staging_upload_buffer = NULL;
    brx_pal_staging_upload_buffer *sh_lut_image_staging_upload_buffer = NULL;

    {
        {
            uint32_t const staging_upload_buffer_offset_alignment = this->m_device->get_staging_upload_buffer_offset_alignment();
            uint32_t const staging_upload_buffer_row_pitch_alignment = this->m_device->get_staging_upload_buffer_row_pitch_alignment();

            uint32_t const total_bytes = brx_pal_sampled_asset_image_import_calculate_subresource_memcpy_dests(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT, g_brx_hdr_lut_width, g_brx_hdr_lut_height, 1U, hdr_lut_mip_level_count, hdr_lut_array_layer_count, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, hdr_lut_subresource_count, hdr_lut_subresource_memcpy_dests);

            assert(NULL == hdr_lut_image_staging_upload_buffer);
            hdr_lut_image_staging_upload_buffer = this->m_device->create_staging_upload_buffer(total_bytes);
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < hdr_lut_array_layer_count; ++array_layer_index)
        {
            constexpr uint32_t const input_row_count = g_brx_hdr_lut_height;
            constexpr uint32_t const input_row_size = static_cast<uint32_t>(sizeof(uint16_t)) * 2U * g_brx_hdr_lut_height;
            static_assert(sizeof(g_brx_hdr_lut_norms) == (input_row_size * input_row_count), "");
            constexpr uint32_t const input_row_pitch = input_row_size;

            constexpr uint32_t const mip_level_index = 0U;

            uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, hdr_lut_mip_level_count, hdr_lut_array_layer_count);
            assert(input_row_size == hdr_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(input_row_count == hdr_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            uint32_t const memory_copy_count = std::min(input_row_size, hdr_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(1U == hdr_lut_subresource_memcpy_dests[subresource_index].output_slice_count);

            for (uint32_t row_index = 0U; row_index < input_row_count; ++row_index)
            {
                void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(hdr_lut_image_staging_upload_buffer->get_host_memory_range_base()) + (hdr_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + hdr_lut_subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(g_brx_hdr_lut_norms) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
            }
        }
    }

    {
        {
            uint32_t const staging_upload_buffer_offset_alignment = this->m_device->get_staging_upload_buffer_offset_alignment();
            uint32_t const staging_upload_buffer_row_pitch_alignment = this->m_device->get_staging_upload_buffer_row_pitch_alignment();

            uint32_t const total_bytes = brx_pal_sampled_asset_image_import_calculate_subresource_memcpy_dests(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT, g_brx_ltc_lut_width, g_brx_ltc_lut_height, 1U, ltc_lut_mip_level_count, ltc_lut_array_layer_count, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, ltc_lut_subresource_count, ltc_lut_subresource_memcpy_dests);

            assert(NULL == ltc_lut_image_staging_upload_buffer);
            ltc_lut_image_staging_upload_buffer = this->m_device->create_staging_upload_buffer(total_bytes);
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < ltc_lut_array_layer_count; ++array_layer_index)
        {
            constexpr uint32_t const input_row_count = g_brx_ltc_lut_height;
            constexpr uint32_t const input_row_size = static_cast<uint32_t>(sizeof(uint16_t)) * 4U * g_brx_ltc_lut_height;
            static_assert(sizeof(g_brx_ltc_lut_matrices) == (input_row_size * input_row_count), "");
            constexpr uint32_t const input_row_pitch = input_row_size;

            constexpr uint32_t const mip_level_index = 0U;

            uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, ltc_lut_mip_level_count, ltc_lut_array_layer_count);
            assert(input_row_size == ltc_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(input_row_count == ltc_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            uint32_t const memory_copy_count = std::min(input_row_size, ltc_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(1U == ltc_lut_subresource_memcpy_dests[subresource_index].output_slice_count);

            for (uint32_t row_index = 0U; row_index < input_row_count; ++row_index)
            {
                void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ltc_lut_image_staging_upload_buffer->get_host_memory_range_base()) + (ltc_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + ltc_lut_subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(g_brx_ltc_lut_matrices) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
            }
        }
    }

    {
        {
            uint32_t const staging_upload_buffer_offset_alignment = this->m_device->get_staging_upload_buffer_offset_alignment();
            uint32_t const staging_upload_buffer_row_pitch_alignment = this->m_device->get_staging_upload_buffer_row_pitch_alignment();

            uint32_t const total_bytes = brx_pal_sampled_asset_image_import_calculate_subresource_memcpy_dests(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT, g_brx_sh_lut_width, g_brx_sh_lut_height, 1U, sh_lut_mip_level_count, sh_lut_array_layer_count, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, sh_lut_subresource_count, sh_lut_subresource_memcpy_dests);

            assert(NULL == sh_lut_image_staging_upload_buffer);
            sh_lut_image_staging_upload_buffer = this->m_device->create_staging_upload_buffer(total_bytes);
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < sh_lut_array_layer_count; ++array_layer_index)
        {
            constexpr uint32_t const input_row_count = g_brx_sh_lut_width;
            constexpr uint32_t const input_row_size = static_cast<uint32_t>(sizeof(uint16_t)) * 1U * g_brx_sh_lut_height;
            static_assert(sizeof(g_brx_sh_lut_transfer_functions) == (input_row_size * input_row_count * sh_lut_array_layer_count), "");
            constexpr uint32_t const input_row_pitch = input_row_size;

            constexpr uint32_t const mip_level_index = 0U;

            uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, sh_lut_mip_level_count, sh_lut_array_layer_count);
            assert(input_row_size == sh_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(input_row_count == sh_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            uint32_t const memory_copy_count = std::min(input_row_size, sh_lut_subresource_memcpy_dests[subresource_index].output_row_size);
            assert(1U == sh_lut_subresource_memcpy_dests[subresource_index].output_slice_count);

            for (uint32_t row_index = 0U; row_index < input_row_count; ++row_index)
            {
                void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(sh_lut_image_staging_upload_buffer->get_host_memory_range_base()) + (sh_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + sh_lut_subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(g_brx_sh_lut_transfer_functions) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
            }
        }
    }

    assert(NULL == this->m_lut_specular_hdr_fresnel_factor_asset_image);
    this->m_lut_specular_hdr_fresnel_factor_asset_image = this->m_device->create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT, g_brx_hdr_lut_width, g_brx_hdr_lut_height, false, hdr_lut_array_layer_count, 1U);

    assert(NULL == this->m_lut_specular_ltc_matrix_asset_image);
    this->m_lut_specular_ltc_matrix_asset_image = this->m_device->create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT, g_brx_ltc_lut_width, g_brx_ltc_lut_height, false, ltc_lut_array_layer_count, 1U);

    assert(NULL == this->m_lut_specular_transfer_function_sh_coefficient_asset_image);
    this->m_lut_specular_transfer_function_sh_coefficient_asset_image = this->m_device->create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT, g_brx_sh_lut_width, g_brx_sh_lut_height, true, sh_lut_array_layer_count, 1U);

    {
        brx_pal_upload_command_buffer *const upload_command_buffer = this->m_device->create_upload_command_buffer();

        brx_pal_graphics_command_buffer *const graphics_command_buffer = this->m_device->create_graphics_command_buffer();

        brx_pal_upload_queue *const upload_queue = this->m_device->create_upload_queue();

        brx_pal_graphics_queue *const graphics_queue = this->m_device->create_graphics_queue();

        brx_pal_fence *const fence = this->m_device->create_fence(true);

        this->m_device->reset_upload_command_buffer(upload_command_buffer);

        this->m_device->reset_graphics_command_buffer(graphics_command_buffer);

        upload_command_buffer->begin();

        graphics_command_buffer->begin();

        BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE uploaded_asset_image_subresources[hdr_lut_subresource_count + ltc_lut_subresource_count + sh_lut_subresource_count];

        constexpr uint32_t const hdr_lut_subresource_index_offset = 0U;
        constexpr uint32_t const ltc_lut_subresource_index_offset = hdr_lut_subresource_index_offset + hdr_lut_subresource_count;
        constexpr uint32_t const sh_lut_subresource_index_offset = ltc_lut_subresource_index_offset + ltc_lut_subresource_count;

        for (uint32_t array_layer_index = 0U; array_layer_index < hdr_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < hdr_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, hdr_lut_mip_level_count, hdr_lut_array_layer_count);

                uploaded_asset_image_subresources[hdr_lut_subresource_index_offset + subresource_index] = BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE{this->m_lut_specular_hdr_fresnel_factor_asset_image, mip_level_index, array_layer_index};
            }
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < ltc_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < ltc_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, ltc_lut_mip_level_count, ltc_lut_array_layer_count);

                uploaded_asset_image_subresources[ltc_lut_subresource_index_offset + subresource_index] = BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE{this->m_lut_specular_ltc_matrix_asset_image, mip_level_index, array_layer_index};
            }
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < sh_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < sh_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, sh_lut_mip_level_count, sh_lut_array_layer_count);

                uploaded_asset_image_subresources[sh_lut_subresource_index_offset + subresource_index] = BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE{this->m_lut_specular_transfer_function_sh_coefficient_asset_image, mip_level_index, array_layer_index};
            }
        }

        upload_command_buffer->asset_resource_load_dont_care(0U, NULL, static_cast<uint32_t>(sizeof(uploaded_asset_image_subresources) / sizeof(uploaded_asset_image_subresources[0])), uploaded_asset_image_subresources);

        for (uint32_t array_layer_index = 0U; array_layer_index < hdr_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < hdr_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, hdr_lut_mip_level_count, hdr_lut_array_layer_count);

                upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(&uploaded_asset_image_subresources[hdr_lut_subresource_index_offset + subresource_index], BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT, g_brx_hdr_lut_width, g_brx_hdr_lut_height, hdr_lut_image_staging_upload_buffer, hdr_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset, hdr_lut_subresource_memcpy_dests[subresource_index].output_row_pitch, hdr_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            }
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < ltc_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < ltc_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, ltc_lut_mip_level_count, ltc_lut_array_layer_count);

                upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(&uploaded_asset_image_subresources[ltc_lut_subresource_index_offset + subresource_index], BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT, g_brx_ltc_lut_width, g_brx_ltc_lut_height, ltc_lut_image_staging_upload_buffer, ltc_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset, ltc_lut_subresource_memcpy_dests[subresource_index].output_row_pitch, ltc_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            }
        }

        for (uint32_t array_layer_index = 0U; array_layer_index < sh_lut_array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < sh_lut_mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, sh_lut_mip_level_count, sh_lut_array_layer_count);

                upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(&uploaded_asset_image_subresources[sh_lut_subresource_index_offset + subresource_index], BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT, g_brx_sh_lut_width, g_brx_sh_lut_height, sh_lut_image_staging_upload_buffer, sh_lut_subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset, sh_lut_subresource_memcpy_dests[subresource_index].output_row_pitch, sh_lut_subresource_memcpy_dests[subresource_index].output_row_count);
            }
        }

        upload_command_buffer->asset_resource_store(0U, NULL, static_cast<uint32_t>(sizeof(uploaded_asset_image_subresources) / sizeof(uploaded_asset_image_subresources[0])), uploaded_asset_image_subresources);

        upload_command_buffer->release(0U, NULL, static_cast<uint32_t>(sizeof(uploaded_asset_image_subresources) / sizeof(uploaded_asset_image_subresources[0])), uploaded_asset_image_subresources, 0U, NULL);

        graphics_command_buffer->acquire(0U, NULL, static_cast<uint32_t>(sizeof(uploaded_asset_image_subresources) / sizeof(uploaded_asset_image_subresources[0])), uploaded_asset_image_subresources, 0U, NULL);

        upload_command_buffer->end();

        graphics_command_buffer->end();

        upload_queue->submit_and_signal(upload_command_buffer);

        this->m_device->reset_fence(fence);

        graphics_queue->wait_and_submit(upload_command_buffer, graphics_command_buffer, fence);

        // TODO: async load
        this->m_device->wait_for_fence(fence);

        this->m_device->destroy_fence(fence);

        this->m_device->destroy_upload_command_buffer(upload_command_buffer);

        this->m_device->destroy_graphics_command_buffer(graphics_command_buffer);

        this->m_device->destroy_upload_queue(upload_queue);

        this->m_device->destroy_graphics_queue(graphics_queue);
    }

    this->m_device->destroy_staging_upload_buffer(hdr_lut_image_staging_upload_buffer);
    hdr_lut_image_staging_upload_buffer = NULL;

    this->m_device->destroy_staging_upload_buffer(ltc_lut_image_staging_upload_buffer);
    ltc_lut_image_staging_upload_buffer = NULL;

    this->m_device->destroy_staging_upload_buffer(sh_lut_image_staging_upload_buffer);
    sh_lut_image_staging_upload_buffer = NULL;
}

void brx_anari_pal_device::init_place_holder_resource()
{
    constexpr uint32_t const place_holder_asset_buffer_size = 1U;
    static_assert(place_holder_asset_buffer_size < SURFACE_VERTEX_POSITION_BUFFER_STRIDE, "");
    static_assert(place_holder_asset_buffer_size < SURFACE_VERTEX_VARYING_BUFFER_STRIDE, "");
    static_assert(place_holder_asset_buffer_size < SURFACE_VERTEX_BLENDING_BUFFER_STRIDE, "");

    assert(NULL == this->m_place_holder_asset_buffer);
    this->m_place_holder_asset_buffer = this->m_device->create_storage_asset_buffer(place_holder_asset_buffer_size);

    constexpr BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT const place_holder_asset_image_format = BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM;
    constexpr uint32_t const place_holder_asset_image_width = 1U;
    constexpr uint32_t const place_holder_asset_image_height = 1U;
    constexpr uint32_t const place_holder_asset_image_mip_level_count = 1U;
    constexpr uint32_t const place_holder_asset_image_mip_level_index = 0U;
    constexpr uint32_t const place_holder_asset_image_array_layer_count = 1U;
    constexpr uint32_t const place_holder_asset_image_array_layer_index = 0U;

    assert(NULL == this->m_place_holder_asset_image);
    this->m_place_holder_asset_image = this->m_device->create_sampled_asset_image(place_holder_asset_image_format, place_holder_asset_image_width, place_holder_asset_image_height, false, place_holder_asset_image_array_layer_count, place_holder_asset_image_mip_level_count);

    brx_pal_upload_command_buffer *const upload_command_buffer = this->m_device->create_upload_command_buffer();

    brx_pal_graphics_command_buffer *const graphics_command_buffer = this->m_device->create_graphics_command_buffer();

    brx_pal_upload_queue *const upload_queue = this->m_device->create_upload_queue();

    brx_pal_graphics_queue *const graphics_queue = this->m_device->create_graphics_queue();

    brx_pal_fence *const fence = this->m_device->create_fence(true);

    this->m_device->reset_upload_command_buffer(upload_command_buffer);

    this->m_device->reset_graphics_command_buffer(graphics_command_buffer);

    upload_command_buffer->begin();

    graphics_command_buffer->begin();

    BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const uploaded_asset_image_subresource{this->m_place_holder_asset_image, place_holder_asset_image_mip_level_index, place_holder_asset_image_array_layer_index};

    upload_command_buffer->asset_resource_load_dont_care(1U, &this->m_place_holder_asset_buffer, 1U, &uploaded_asset_image_subresource);

    upload_command_buffer->asset_resource_store(1U, &this->m_place_holder_asset_buffer, 1U, &uploaded_asset_image_subresource);

    upload_command_buffer->release(1U, &this->m_place_holder_asset_buffer, 1U, &uploaded_asset_image_subresource, 0U, NULL);

    graphics_command_buffer->acquire(1U, &this->m_place_holder_asset_buffer, 1U, &uploaded_asset_image_subresource, 0U, NULL);

    upload_command_buffer->end();

    graphics_command_buffer->end();

    upload_queue->submit_and_signal(upload_command_buffer);

    this->m_device->reset_fence(fence);

    graphics_queue->wait_and_submit(upload_command_buffer, graphics_command_buffer, fence);

    // TODO: async load
    this->m_device->wait_for_fence(fence);

    this->m_device->destroy_fence(fence);

    this->m_device->destroy_upload_command_buffer(upload_command_buffer);

    this->m_device->destroy_graphics_command_buffer(graphics_command_buffer);

    this->m_device->destroy_upload_queue(upload_queue);

    this->m_device->destroy_graphics_queue(graphics_queue);

    constexpr BRX_PAL_STORAGE_IMAGE_FORMAT const place_holder_storage_image_format = BRX_PAL_STORAGE_IMAGE_FORMAT_R32_UINT;
    constexpr uint32_t const place_holder_storage_image_width = 1U;
    constexpr uint32_t const place_holder_storage_image_height = 1U;
    constexpr bool const place_holder_storage_image_volume = false;
    constexpr uint32_t const place_holder_storage_image_depth = 1U;
    constexpr bool const place_holder_storage_image_allow_sampled_image = false;

    assert(NULL == this->m_place_holder_storage_image);
    this->m_place_holder_storage_image = this->m_device->create_storage_image(place_holder_storage_image_format, place_holder_storage_image_width, place_holder_storage_image_height, place_holder_storage_image_volume, place_holder_storage_image_depth, place_holder_storage_image_allow_sampled_image);
}

static inline uint32_t internal_align_up(uint32_t value, uint32_t alignment)
{
    //
    //  Copyright (c) 2005-2019 Intel Corporation
    //
    //  Licensed under the Apache License, Version 2.0 (the "License");
    //  you may not use this file except in compliance with the License.
    //  You may obtain a copy of the License at
    //
    //      http://www.apache.org/licenses/LICENSE-2.0
    //
    //  Unless required by applicable law or agreed to in writing, software
    //  distributed under the License is distributed on an "AS IS" BASIS,
    //  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    //  See the License for the specific language governing permissions and
    //  limitations under the License.
    //

    // [alignUp](https://github.com/oneapi-src/oneTBB/blob/tbb_2019/src/tbbmalloc/shared_utils.h#L42)

    assert(alignment != static_cast<uint32_t>(0U));

    // power-of-2 alignment
    assert((alignment & (alignment - static_cast<uint32_t>(1U))) == static_cast<uint32_t>(0U));

    return (((value - static_cast<uint32_t>(1U)) | (alignment - static_cast<uint32_t>(1U))) + static_cast<uint32_t>(1U));
}
