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
#include <cassert>
#include <algorithm>

// #define CV_IGNORE_DEBUG_BUILD_GUARD 1
#if defined(__GNUC__)
// GCC or CLANG
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#elif defined(_MSC_VER)
#if defined(__clang__)
// CLANG-CL
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-noreturn"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#pragma GCC diagnostic pop
#else
// MSVC
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#endif
#else
#error Unknown Compiler
#endif

static inline BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT wrap(BRX_ANARI_IMAGE_FORMAT format)
{
    switch (format)
    {
    case BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_UNORM:
    {
        return BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM;
    }
    case BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_SRGB:
    {
        return BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB;
    }
    case BRX_ANARI_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
    {
        return BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT;
    }
    default:
    {
        assert(false);
        return BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_UNDEFINED;
    }
    }
}

// https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-limits
// D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION 16384
// D3D11_REQ_MIP_LEVELS 15
static constexpr size_t const k_max_image_width_or_height = 16384U;
static constexpr size_t const k_max_image_mip_levels = 15U;

static constexpr int const k_albedo_image_channel_size = sizeof(uint8_t);
static constexpr int const k_albedo_image_num_channels = 4U;

static constexpr int const k_illumiant_image_channel_size = sizeof(uint16_t);
static constexpr int const k_illumiant_image_num_channels = 4U;

static inline bool internal_is_power_of_2(uint32_t width_or_height);
static inline void internal_fit_power_of_2(uint32_t origin_width, uint32_t origin_height, uint32_t &target_width, uint32_t &target_height);
static inline uint32_t internal_count_mips(uint32_t width, uint32_t height);

void brx_anari_pal_device::init_image(BRX_ANARI_IMAGE_FORMAT const origin_format, void const *const origin_pixel_data, uint32_t const origin_width, uint32_t const origin_height, brx_pal_sampled_asset_image **const out_asset_image)
{
    // TODO: merge submit and barrier

    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format;
    uint32_t zeroth_width;
    uint32_t zeroth_height;
    uint32_t mip_level_count;
    constexpr uint32_t const array_layer_count = 1U;
    uint32_t subresource_count;
    mcrt_vector<BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST> subresource_memcpy_dests;
    brx_pal_staging_upload_buffer *image_staging_upload_buffer = NULL;
    {
        format = wrap(origin_format);

        internal_fit_power_of_2(origin_width, origin_height, zeroth_width, zeroth_height);
        assert((zeroth_width >= 1U) && (zeroth_width <= k_max_image_width_or_height) && internal_is_power_of_2(zeroth_width) && (zeroth_height >= 1U) && (zeroth_height <= k_max_image_width_or_height) && internal_is_power_of_2(zeroth_height));

        mip_level_count = internal_count_mips(zeroth_width, zeroth_height);
        assert((mip_level_count >= 1U) && (mip_level_count <= k_max_image_mip_levels));

        subresource_count = mip_level_count * array_layer_count;

        subresource_memcpy_dests.resize(subresource_count);

        uint32_t const staging_upload_buffer_offset_alignment = this->m_device->get_staging_upload_buffer_offset_alignment();
        uint32_t const staging_upload_buffer_row_pitch_alignment = this->m_device->get_staging_upload_buffer_row_pitch_alignment();

        uint32_t const total_bytes = brx_pal_sampled_asset_image_import_calculate_subresource_memcpy_dests(format, zeroth_width, zeroth_height, 1U, mip_level_count, array_layer_count, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, subresource_count, subresource_memcpy_dests.data());

        assert(NULL == image_staging_upload_buffer);
        image_staging_upload_buffer = this->m_device->create_staging_upload_buffer(total_bytes);
    }

    if (BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_UNORM == origin_format || BRX_ANARI_IMAGE_FORMAT_R8G8B8A8_SRGB == origin_format)
    {
        mcrt_vector<uint32_t> pixel_data;
        static_assert(sizeof(pixel_data[0]) == (static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels)), "");
        // zeroth
        {
            uint32_t const width = zeroth_width;
            uint32_t const height = zeroth_height;

            if ((origin_width == width) && (origin_height == height))
            {
                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels)) * pixel_data.size();
                std::memcpy(pixel_data.data(), origin_pixel_data, total_size);
            }
            else
            {
                // mediapipe/framework/formats/image_frame_opencv.cc
                int const dims = 2;
                int const origin_sizes[dims] = {static_cast<int>(origin_height), static_cast<int>(origin_width)};
                int const type = CV_MAKETYPE(CV_8U, k_albedo_image_num_channels);
                size_t const origin_stride = static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels) * static_cast<uint64_t>(origin_width);
                size_t const origin_steps[dims] = {origin_stride, k_albedo_image_channel_size};

                cv::Mat const origin_image(dims, origin_sizes, type, const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(origin_pixel_data)), origin_steps);
                assert(origin_image.isContinuous());

                cv::Mat target_image;
                int const interpolation = ((width <= origin_width) && (height <= origin_height)) ? cv::INTER_AREA : cv::INTER_LANCZOS4;
                cv::resize(origin_image, target_image, cv::Size(static_cast<int>(width), static_cast<int>(height)), 0.0, 0.0, interpolation);

                size_t const target_stride = static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels) * static_cast<uint64_t>(width);
                assert(target_image.type() == type);
                assert(target_image.step[0] == target_stride);
                assert(width == target_image.cols);
                assert(height == target_image.rows);
                assert(target_image.isContinuous());

                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels)) * pixel_data.size();
                assert((target_image.step[0] * target_image.rows) == total_size);
                std::memcpy(pixel_data.data(), target_image.data, total_size);
            }

            {
                constexpr uint32_t const mip_level_index = 0U;
                constexpr uint32_t const array_layer_index = 0U;

                uint32_t const input_row_count = height;
                uint32_t const input_row_size = static_cast<uint32_t>(k_albedo_image_channel_size) * static_cast<uint32_t>(k_albedo_image_num_channels) * width;
                uint32_t const input_row_pitch = input_row_size;

                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);
                assert(input_row_count == subresource_memcpy_dests[subresource_index].output_row_count);
                assert(input_row_size == subresource_memcpy_dests[subresource_index].output_row_size);
                size_t const memory_copy_count = std::min(input_row_size, subresource_memcpy_dests[subresource_index].output_row_size);
                assert(1U == subresource_memcpy_dests[subresource_index].output_slice_count);

                for (uint32_t row_index = 0U; (row_index < input_row_count) && (row_index < subresource_memcpy_dests[subresource_index].output_row_count); ++row_index)
                {
                    void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(image_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                    void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pixel_data.data()) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                    std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
                }
            }
        }

        mcrt_vector<uint32_t> previous_pixel_data;
        static_assert(sizeof(previous_pixel_data[0]) == (static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels)), "");
        for (uint32_t mip_level_index = 1U; mip_level_index < mip_level_count; ++mip_level_index)
        {
            previous_pixel_data = std::move(pixel_data);

            uint32_t const previous_width = std::max(1U, zeroth_width >> (mip_level_index - 1U));
            uint32_t const previous_height = std::max(1U, zeroth_height >> (mip_level_index - 1U));

            uint32_t const width = std::max(1U, zeroth_width >> mip_level_index);
            uint32_t const height = std::max(1U, zeroth_height >> mip_level_index);

            {
                int const dims = 2;
                int const source_sizes[dims] = {static_cast<int>(previous_height), static_cast<int>(previous_width)};
                int const type = CV_MAKETYPE(CV_8U, k_albedo_image_num_channels);
                size_t const source_stride = static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels) * static_cast<uint64_t>(previous_width);
                size_t const source_steps[dims] = {source_stride, k_albedo_image_channel_size};
                cv::Mat source_image(dims, source_sizes, type, reinterpret_cast<uint8_t *>(previous_pixel_data.data()), source_steps);
                assert(source_image.isContinuous());

                cv::Mat destination_image;
                cv::resize(source_image, destination_image, cv::Size(static_cast<int>(width), static_cast<int>(height)), 0.0, 0.0, cv::INTER_AREA);

                size_t const destination_stride = static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels) * static_cast<uint64_t>(width);
                assert(destination_image.type() == type);
                assert(destination_image.step[0] == destination_stride);
                assert(width == destination_image.cols);
                assert(height == destination_image.rows);
                assert(destination_image.isContinuous());

                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_albedo_image_channel_size) * static_cast<size_t>(k_albedo_image_num_channels)) * pixel_data.size();
                assert((destination_image.step[0] * destination_image.rows) == total_size);
                std::memcpy(pixel_data.data(), destination_image.data, total_size);
            }

            {
                constexpr uint32_t const array_layer_index = 0U;

                uint32_t const input_row_count = height;
                uint32_t const input_row_size = static_cast<uint32_t>(k_albedo_image_channel_size) * static_cast<uint32_t>(k_albedo_image_num_channels) * width;
                uint32_t const input_row_pitch = input_row_size;

                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);
                assert(input_row_count == subresource_memcpy_dests[subresource_index].output_row_count);
                assert(input_row_size == subresource_memcpy_dests[subresource_index].output_row_size);
                size_t const memory_copy_count = std::min(input_row_size, subresource_memcpy_dests[subresource_index].output_row_size);
                assert(1U == subresource_memcpy_dests[subresource_index].output_slice_count);

                for (uint32_t row_index = 0U; (row_index < input_row_count) && (row_index < subresource_memcpy_dests[subresource_index].output_row_count); ++row_index)
                {
                    void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(image_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                    void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pixel_data.data()) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                    std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
                }
            }
        }
    }
    else if (BRX_ANARI_IMAGE_FORMAT_R16G16B16A16_SFLOAT == origin_format)
    {
        // TODO: we treat half as int16_t

        mcrt_vector<uint64_t> pixel_data;
        static_assert(sizeof(pixel_data[0]) == (static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels)), "");
        // zeroth
        {
            uint32_t const width = zeroth_width;
            uint32_t const height = zeroth_height;

            if ((origin_width == width) && (origin_height == height))
            {
                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels)) * pixel_data.size();
                std::memcpy(pixel_data.data(), origin_pixel_data, total_size);
            }
            else
            {
                // mediapipe/framework/formats/image_frame_opencv.cc
                int const dims = 2;
                int const origin_sizes[dims] = {static_cast<int>(origin_height), static_cast<int>(origin_width)};
                int const type = CV_MAKETYPE(CV_16S, k_illumiant_image_num_channels);
                size_t const origin_stride = static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels) * static_cast<uint64_t>(origin_width);
                size_t const origin_steps[dims] = {origin_stride, k_illumiant_image_channel_size};

                cv::Mat const origin_image(dims, origin_sizes, type, const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(origin_pixel_data)), origin_steps);
                assert(origin_image.isContinuous());

                cv::Mat target_image;
                int const interpolation = ((width <= origin_width) && (height <= origin_height)) ? cv::INTER_AREA : cv::INTER_LANCZOS4;
                cv::resize(origin_image, target_image, cv::Size(static_cast<int>(width), static_cast<int>(height)), 0.0, 0.0, interpolation);

                size_t const target_stride = static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels) * static_cast<uint64_t>(width);
                assert(target_image.type() == type);
                assert(target_image.step[0] == target_stride);
                assert(width == target_image.cols);
                assert(height == target_image.rows);
                assert(target_image.isContinuous());

                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels)) * pixel_data.size();
                assert((target_image.step[0] * target_image.rows) == total_size);
                std::memcpy(pixel_data.data(), target_image.data, total_size);
            }

            {
                constexpr uint32_t const mip_level_index = 0U;
                constexpr uint32_t const array_layer_index = 0U;

                uint32_t const input_row_count = height;
                uint32_t const input_row_size = static_cast<uint32_t>(k_illumiant_image_channel_size) * static_cast<uint32_t>(k_illumiant_image_num_channels) * width;
                uint32_t const input_row_pitch = input_row_size;

                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);
                assert(input_row_count == subresource_memcpy_dests[subresource_index].output_row_count);
                assert(input_row_size == subresource_memcpy_dests[subresource_index].output_row_size);
                size_t const memory_copy_count = std::min(input_row_size, subresource_memcpy_dests[subresource_index].output_row_size);
                assert(1U == subresource_memcpy_dests[subresource_index].output_slice_count);

                for (uint32_t row_index = 0U; (row_index < input_row_count) && (row_index < subresource_memcpy_dests[subresource_index].output_row_count); ++row_index)
                {
                    void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(image_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                    void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pixel_data.data()) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                    std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
                }
            }
        }

        mcrt_vector<uint64_t> previous_pixel_data;
        static_assert(sizeof(previous_pixel_data[0]) == (static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels)), "");
        for (uint32_t mip_level_index = 1U; mip_level_index < mip_level_count; ++mip_level_index)
        {
            previous_pixel_data = std::move(pixel_data);

            uint32_t const previous_width = std::max(1U, zeroth_width >> (mip_level_index - 1U));
            uint32_t const previous_height = std::max(1U, zeroth_height >> (mip_level_index - 1U));

            uint32_t const width = std::max(1U, zeroth_width >> mip_level_index);
            uint32_t const height = std::max(1U, zeroth_height >> mip_level_index);

            {
                int const dims = 2;
                int const source_sizes[dims] = {static_cast<int>(previous_height), static_cast<int>(previous_width)};
                int const type = CV_MAKETYPE(CV_16S, k_illumiant_image_num_channels);
                size_t const source_stride = static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels) * static_cast<uint64_t>(previous_width);
                size_t const source_steps[dims] = {source_stride, k_illumiant_image_channel_size};
                cv::Mat source_image(dims, source_sizes, type, reinterpret_cast<uint8_t *>(previous_pixel_data.data()), source_steps);
                assert(source_image.isContinuous());

                cv::Mat destination_image;
                cv::resize(source_image, destination_image, cv::Size(static_cast<int>(width), static_cast<int>(height)), 0.0, 0.0, cv::INTER_AREA);

                size_t const destination_stride = static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels) * static_cast<uint64_t>(width);
                assert(destination_image.type() == type);
                assert(destination_image.step[0] == destination_stride);
                assert(width == destination_image.cols);
                assert(height == destination_image.rows);
                assert(destination_image.isContinuous());

                pixel_data.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
                size_t const total_size = (static_cast<size_t>(k_illumiant_image_channel_size) * static_cast<size_t>(k_illumiant_image_num_channels)) * pixel_data.size();
                assert((destination_image.step[0] * destination_image.rows) == total_size);
                std::memcpy(pixel_data.data(), destination_image.data, total_size);
            }

            {
                constexpr uint32_t const array_layer_index = 0U;

                uint32_t const input_row_count = height;
                uint32_t const input_row_size = static_cast<uint32_t>(k_illumiant_image_channel_size) * static_cast<uint32_t>(k_illumiant_image_num_channels) * width;
                uint32_t const input_row_pitch = input_row_size;

                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);
                assert(input_row_count == subresource_memcpy_dests[subresource_index].output_row_count);
                assert(input_row_size == subresource_memcpy_dests[subresource_index].output_row_size);
                size_t const memory_copy_count = std::min(input_row_size, subresource_memcpy_dests[subresource_index].output_row_size);
                assert(1U == subresource_memcpy_dests[subresource_index].output_slice_count);

                for (uint32_t row_index = 0U; (row_index < input_row_count) && (row_index < subresource_memcpy_dests[subresource_index].output_row_count); ++row_index)
                {
                    void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(image_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
                    void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(pixel_data.data()) + (input_row_pitch * input_row_count) * array_layer_index + input_row_pitch * row_index);
                    std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
                }
            }
        }
    }
    else
    {
        assert(false);
    }

    brx_pal_sampled_asset_image *const destination_asset_image = this->m_device->create_sampled_asset_image(format, zeroth_width, zeroth_height, false, array_layer_count, mip_level_count);
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

        mcrt_vector<BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE> uploaded_asset_image_subresources(static_cast<size_t>(subresource_count));

        for (uint32_t array_layer_index = 0U; array_layer_index < array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);

                uploaded_asset_image_subresources[subresource_index] = BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE{destination_asset_image, mip_level_index, array_layer_index};
            }
        }

        upload_command_buffer->asset_resource_load_dont_care(0U, NULL, static_cast<uint32_t>(uploaded_asset_image_subresources.size()), uploaded_asset_image_subresources.data());

        for (uint32_t array_layer_index = 0U; array_layer_index < array_layer_count; ++array_layer_index)
        {
            for (uint32_t mip_level_index = 0U; mip_level_index < mip_level_count; ++mip_level_index)
            {
                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mip_level_index, array_layer_index, 0U, mip_level_count, array_layer_count);

                upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(&uploaded_asset_image_subresources[subresource_index], format, zeroth_width, zeroth_height, image_staging_upload_buffer, subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset, subresource_memcpy_dests[subresource_index].output_row_pitch, subresource_memcpy_dests[subresource_index].output_row_count);
            }
        }

        upload_command_buffer->asset_resource_store(0U, NULL, static_cast<uint32_t>(uploaded_asset_image_subresources.size()), uploaded_asset_image_subresources.data());

        upload_command_buffer->release(0U, NULL, static_cast<uint32_t>(uploaded_asset_image_subresources.size()), uploaded_asset_image_subresources.data(), 0U, NULL);

        graphics_command_buffer->acquire(0U, NULL, static_cast<uint32_t>(uploaded_asset_image_subresources.size()), uploaded_asset_image_subresources.data(), 0U, NULL);

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

    this->m_device->destroy_staging_upload_buffer(image_staging_upload_buffer);
    image_staging_upload_buffer = NULL;

    (*out_asset_image) = destination_asset_image;
}

brx_anari_image *brx_anari_pal_device::new_image(BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height)
{
    void *new_unwrapped_image_base = mcrt_malloc(sizeof(brx_anari_pal_image), alignof(brx_anari_pal_image));
    assert(NULL != new_unwrapped_image_base);

    brx_anari_pal_image *new_unwrapped_image = new (new_unwrapped_image_base) brx_anari_pal_image{};
    new_unwrapped_image->init(this, format, pixel_data, width, height);
    return new_unwrapped_image;
}

void brx_anari_pal_device::release_image(brx_anari_pal_image *const release_unwrapped_image)
{
    if (0U == release_unwrapped_image->internal_release())
    {
        brx_anari_pal_image *const delete_unwrapped_image = release_unwrapped_image;

        delete_unwrapped_image->uninit(this);

        delete_unwrapped_image->~brx_anari_pal_image();
        mcrt_free(delete_unwrapped_image);
    }
}

void brx_anari_pal_device::release_image(brx_anari_image *wrapped_image)
{
    assert(NULL != wrapped_image);
    brx_anari_pal_image *const release_unwrapped_image = static_cast<brx_anari_pal_image *>(wrapped_image);

    this->release_image(release_unwrapped_image);
}

inline brx_anari_pal_image::brx_anari_pal_image() : m_ref_count(0U), m_image(NULL), m_width(0U), m_height(0U)
{
}

inline brx_anari_pal_image::~brx_anari_pal_image()
{
    assert(0U == this->m_ref_count);
    assert(NULL == this->m_image);
}

inline void brx_anari_pal_image::init(brx_anari_pal_device *device, BRX_ANARI_IMAGE_FORMAT format, void const *pixel_data, uint32_t width, uint32_t height)
{
    assert(0U == this->m_ref_count);
    this->m_ref_count = 1U;

    assert(NULL == this->m_image);
    device->init_image(format, pixel_data, width, height, &this->m_image);

    assert(0U == this->m_width);
    this->m_width = width;

    assert(0U == this->m_height);
    this->m_height = height;
}

inline void brx_anari_pal_image::uninit(brx_anari_pal_device *device)
{
    assert(0U == this->m_ref_count);

    assert(NULL != this->m_image);
    device->helper_destroy_asset_image(this->m_image);
    this->m_image = NULL;
}

void brx_anari_pal_image::retain()
{
    assert(this->m_ref_count > 0U);
    assert(this->m_ref_count < static_cast<uint32_t>(UINT32_MAX));
    ++this->m_ref_count;
}

inline uint32_t brx_anari_pal_image::internal_release()
{
    assert(this->m_ref_count > 0U);
    --this->m_ref_count;
    return this->m_ref_count;
}

brx_pal_sampled_asset_image const *brx_anari_pal_image::get_image() const
{
    assert(this->m_ref_count > 0U);
    return this->m_image;
}

uint32_t brx_anari_pal_image::get_width() const
{
    assert(this->m_ref_count > 0U);
    return this->m_width;
}

uint32_t brx_anari_pal_image::get_height() const
{
    assert(this->m_ref_count > 0U);
    return this->m_height;
}

static inline bool internal_is_power_of_2(uint32_t width_or_height)
{
    assert(width_or_height >= 1U);
    return (0U == (width_or_height & (width_or_height - 1U)));
}

static inline void internal_fit_power_of_2(uint32_t origin_width, uint32_t origin_height, uint32_t &target_width, uint32_t &target_height)
{
    assert((origin_width >= 1U) && (origin_width <= k_max_image_width_or_height) && (origin_height >= 1U) && (origin_height <= k_max_image_width_or_height));

    // DirectXTex/texconv.cpp: FitPowerOf2

    if (internal_is_power_of_2(origin_width) && internal_is_power_of_2(origin_height))
    {
        target_width = origin_width;
        target_height = origin_height;
    }
    else
    {
        if (origin_width > origin_height)
        {
            if (internal_is_power_of_2(origin_width))
            {
                target_width = origin_width;
            }
            else
            {
                uint32_t w;
                for (w = k_max_image_width_or_height; w > 1; w >>= 1)
                {
                    if (w <= origin_width)
                    {
                        break;
                    }
                }
                target_width = w;
            }

            {
                float const origin_aspect_ratio = static_cast<float>(origin_width) / static_cast<float>(origin_height);
                float best_score = FLT_MAX;
                for (uint32_t h = k_max_image_width_or_height; h > 0; h >>= 1)
                {
                    float const score = std::abs((static_cast<float>(target_width) / static_cast<float>(h)) - origin_aspect_ratio);
                    if (score < best_score)
                    {
                        best_score = score;
                        target_height = h;
                    }
                }
            }
        }
        else
        {
            if (internal_is_power_of_2(origin_height))
            {
                target_height = origin_height;
            }
            else
            {
                uint32_t h;
                for (h = k_max_image_width_or_height; h > 1; h >>= 1)
                {
                    if (h <= origin_height)
                    {
                        break;
                    }
                }
                target_height = h;
            }

            {
                float const rcp_origin_aspect_ratio = static_cast<float>(origin_height) / static_cast<float>(origin_width);
                float best_score = FLT_MAX;
                for (uint32_t w = k_max_image_width_or_height; w > 0; w >>= 1)
                {
                    float const score = std::abs((static_cast<float>(target_height) / static_cast<float>((w))) - rcp_origin_aspect_ratio);
                    if (score < best_score)
                    {
                        best_score = score;
                        target_width = w;
                    }
                }
            }
        }
    }
}

static inline uint32_t internal_count_mips(uint32_t width, uint32_t height)
{
    assert(width >= 1U && height >= 1U);

    // DirectXTex/DirectXTexMipmaps.cpp CountMips

    uint32_t mip_level_count = 1U;

    while (width > 1U || height > 1U)
    {
        width = std::max(1U, width >> 1U);

        height = std::max(1U, height >> 1U);

        ++mip_level_count;
    }

    return mip_level_count;
}
