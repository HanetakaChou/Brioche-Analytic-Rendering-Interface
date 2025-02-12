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

#include "brx_anari_brx_device.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <tuple>
#include <new>

static inline brx_anari_image *wrap(brx_sampled_asset_image *unwrapped_image)
{
    return reinterpret_cast<brx_anari_image *>(unwrapped_image);
}

static inline brx_sampled_asset_image *unwrap(brx_anari_image *wrapped_image)
{
    return reinterpret_cast<brx_sampled_asset_image *>(wrapped_image);
}

extern "C" brx_anari_device *brx_anari_new_device(void *device)
{
    void *new_unwrapped_device_base = mcrt_malloc(sizeof(brx_anari_brx_device), alignof(brx_anari_brx_device));
    assert(NULL != new_unwrapped_device_base);

    brx_anari_brx_device *new_unwrapped_device = new (new_unwrapped_device_base) brx_anari_brx_device{};
    new_unwrapped_device->init(static_cast<brx_device *>(device));
    return new_unwrapped_device;
}

extern void brx_destroy_vk_device(brx_anari_device *wrapped_device)
{
    assert(NULL != wrapped_device);
    brx_anari_brx_device *delete_unwrapped_device = static_cast<brx_anari_brx_device *>(wrapped_device);

    delete_unwrapped_device->uninit();

    delete_unwrapped_device->~brx_anari_brx_device();
    mcrt_free(delete_unwrapped_device);
}

brx_anari_brx_device::brx_anari_brx_device()
    : m_device(NULL),
      m_upload_queue(NULL),
      m_graphics_queue(NULL),
      m_asset_import_upload_command_buffer(NULL),
      m_asset_import_graphics_command_buffer(NULL),
      m_asset_import_fence(NULL),
      m_staging_upload_buffer_offset_alignment(static_cast<uint32_t>(~0U)),
      m_staging_upload_buffer_row_pitch_alignment(static_cast<uint32_t>(~0U)),
      m_frame_throttling_index(static_cast<uint32_t>(~0U))
{
}

brx_anari_brx_device::~brx_anari_brx_device()
{
    // At most time, these pending tasks have been completed when we release the device
    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < BRX_ANARI_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(this->m_pending_release_storage_asset_buffers[frame_throttling_index].empty());
        assert(this->m_pending_release_sampled_asset_images[frame_throttling_index].empty());
    }
    assert(static_cast<uint32_t>(~0U) == this->m_frame_throttling_index);

    assert(this->m_pending_commit_staging_upload_buffers.empty());
    assert(this->m_pending_commit_storage_asset_buffers.empty());
    assert(this->m_pending_commit_sampled_asset_image_subresources.empty());
    assert(static_cast<uint32_t>(~0U) == this->m_staging_upload_buffer_row_pitch_alignment);
    assert(static_cast<uint32_t>(~0U) == this->m_staging_upload_buffer_offset_alignment);
    assert(NULL == this->m_asset_import_fence);
    assert(NULL == this->m_asset_import_upload_command_buffer);
    assert(NULL == this->m_asset_import_graphics_command_buffer);

    assert(NULL == this->m_upload_queue);
    assert(NULL == this->m_graphics_queue);
    assert(NULL == this->m_device);
}

void brx_anari_brx_device::init(brx_device *device)
{
    assert(NULL == this->m_device);
    this->m_device = device;

    assert(NULL == this->m_upload_queue);
    this->m_upload_queue = device->create_upload_queue();

    assert(NULL == this->m_graphics_queue);
    this->m_graphics_queue = device->create_graphics_queue();

    assert(NULL == this->m_asset_import_upload_command_buffer);
    this->m_asset_import_upload_command_buffer = device->create_upload_command_buffer();
    this->m_asset_import_upload_command_buffer->begin();

    assert(NULL == this->m_asset_import_graphics_command_buffer);
    this->m_asset_import_graphics_command_buffer = device->create_graphics_command_buffer();
    this->m_asset_import_graphics_command_buffer->begin();

    assert(NULL == this->m_asset_import_fence);
    this->m_asset_import_fence = device->create_fence(true);

    assert(static_cast<uint32_t>(~0U) == this->m_staging_upload_buffer_offset_alignment);
    this->m_staging_upload_buffer_offset_alignment = device->get_staging_upload_buffer_offset_alignment();

    assert(static_cast<uint32_t>(~0U) == this->m_staging_upload_buffer_row_pitch_alignment);
    this->m_staging_upload_buffer_row_pitch_alignment = device->get_staging_upload_buffer_row_pitch_alignment();

    assert(this->m_pending_commit_storage_asset_buffers.empty());
    assert(this->m_pending_commit_sampled_asset_image_subresources.empty());
    assert(this->m_pending_commit_staging_upload_buffers.empty());

    assert(static_cast<uint32_t>(~0U) == this->m_frame_throttling_index);
    this->m_frame_throttling_index = 0U;

    for (uint32_t frame_throttling_index = 0U; frame_throttling_index < BRX_ANARI_FRAME_THROTTLING_COUNT; ++frame_throttling_index)
    {
        assert(this->m_pending_release_storage_asset_buffers[frame_throttling_index].empty());
        assert(this->m_pending_release_sampled_asset_images[frame_throttling_index].empty());
    }
}

void brx_anari_brx_device::uninit()
{
    assert(static_cast<uint32_t>(~0U) != this->m_frame_throttling_index);
    this->m_frame_throttling_index = static_cast<uint32_t>(~0U);

    assert(static_cast<uint32_t>(~0U) != this->m_staging_upload_buffer_row_pitch_alignment);
    this->m_staging_upload_buffer_row_pitch_alignment = static_cast<uint32_t>(~0U);

    assert(static_cast<uint32_t>(~0U) != this->m_staging_upload_buffer_offset_alignment);
    this->m_staging_upload_buffer_offset_alignment = static_cast<uint32_t>(~0U);

    this->m_device->wait_for_fence(this->m_asset_import_fence);

    assert(NULL != this->m_asset_import_upload_command_buffer);
    this->m_asset_import_upload_command_buffer->end();
    this->m_device->destroy_upload_command_buffer(this->m_asset_import_upload_command_buffer);
    this->m_asset_import_upload_command_buffer = NULL;

    assert(NULL != this->m_asset_import_graphics_command_buffer);
    this->m_asset_import_graphics_command_buffer->end();
    this->m_device->destroy_graphics_command_buffer(this->m_asset_import_graphics_command_buffer);
    this->m_asset_import_graphics_command_buffer = NULL;

    assert(NULL != this->m_asset_import_fence);
    this->m_device->destroy_fence(this->m_asset_import_fence);
    this->m_asset_import_fence = NULL;

    assert(NULL != this->m_upload_queue);
    this->m_device->destroy_upload_queue(this->m_upload_queue);
    this->m_upload_queue = NULL;

    assert(NULL != this->m_graphics_queue);
    this->m_device->destroy_graphics_queue(this->m_graphics_queue);
    this->m_graphics_queue = NULL;

    assert(NULL != this->m_device);
    this->m_device = NULL;
}

brx_anari_image *brx_anari_brx_device::new_image(brx_asset_import_image *asset_import_image, bool force_srgb)
{
    assert(NULL != asset_import_image);

    BRX_SAMPLED_ASSET_IMAGE_FORMAT const format = asset_import_image->get_format(force_srgb);

    uint32_t const mip_level_count = asset_import_image->get_mip_level_count();

    mcrt_vector<BRX_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST> subresource_memcpy_dests;
    uint32_t const subresource_count = mip_level_count;
    subresource_memcpy_dests.resize(subresource_count);

    uint32_t const total_bytes = brx_sampled_asset_image_import_calculate_subresource_memcpy_dests(format, asset_import_image->get_width(0U), asset_import_image->get_height(0U), 1U, mip_level_count, 1U, 0U, this->m_staging_upload_buffer_offset_alignment, this->m_staging_upload_buffer_row_pitch_alignment, subresource_count, &subresource_memcpy_dests[0]);
    brx_staging_upload_buffer *new_staging_upload_buffer = this->m_device->create_staging_upload_buffer(static_cast<uint32_t>(total_bytes));
    this->m_pending_commit_staging_upload_buffers.push_back(new_staging_upload_buffer);

    for (uint32_t mip_level_index = 0U; mip_level_index < mip_level_count; ++mip_level_index)
    {
        uint32_t const subresource_index = brx_sampled_asset_image_import_calculate_subresource_index(mip_level_index, 0U, 0U, mip_level_count, 1U);
        assert(1U == subresource_memcpy_dests[mip_level_index].output_slice_count);

        assert(asset_import_image->get_row_count(mip_level_index) == subresource_memcpy_dests[mip_level_index].output_row_count);
        assert(asset_import_image->get_row_size(mip_level_index) == subresource_memcpy_dests[mip_level_index].output_row_size);
        size_t const memory_copy_count = std::min(asset_import_image->get_row_size(mip_level_index), subresource_memcpy_dests[mip_level_index].output_row_size);

        for (uint32_t row_index = 0U; (row_index < asset_import_image->get_row_count(mip_level_index)) && (row_index < subresource_memcpy_dests[mip_level_index].output_row_count); ++row_index)
        {
            void *const memory_copy_destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(new_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_row_pitch * row_index));
            void *const memory_copy_source = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(asset_import_image->get_pixel_data(mip_level_index)) + asset_import_image->get_row_pitch(mip_level_index) * row_index);
            std::memcpy(memory_copy_destination, memory_copy_source, memory_copy_count);
        }
    }

    brx_sampled_asset_image *new_imported_sample_asset_texture = this->m_device->create_sampled_asset_image(format, asset_import_image->get_width(0U), asset_import_image->get_height(0U), mip_level_count);
    for (uint32_t mip_level_index = 0U; mip_level_index < mip_level_count; ++mip_level_index)
    {
        this->m_asset_import_upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(new_imported_sample_asset_texture, format, asset_import_image->get_width(0U), asset_import_image->get_height(0U), mip_level_index, new_staging_upload_buffer, subresource_memcpy_dests[mip_level_index].staging_upload_buffer_offset, subresource_memcpy_dests[mip_level_index].output_row_pitch, subresource_memcpy_dests[mip_level_index].output_row_count);
        this->m_pending_commit_sampled_asset_image_subresources.push_back({new_imported_sample_asset_texture, mip_level_index});
    }
}

void brx_anari_brx_device::release_image(brx_anari_image *image)
{
    uint32_t const previous_frame_throttling_index = ((this->m_frame_throttling_index >= 1U) ? (this->m_frame_throttling_index - 1U) : BRX_ANARI_FRAME_THROTTLING_COUNT);

    this->m_pending_release_sampled_asset_images[previous_frame_throttling_index].push_back(unwrap(image));
}

void brx_anari_brx_device::commit_buffer_and_image()
{
    this->m_asset_import_upload_command_buffer->release(static_cast<uint32_t>(this->m_pending_commit_storage_asset_buffers.size()), this->m_pending_commit_storage_asset_buffers.data(), static_cast<uint32_t>(this->m_pending_commit_sampled_asset_image_subresources.size()), this->m_pending_commit_sampled_asset_image_subresources.data(), 0U, NULL);

    this->m_asset_import_graphics_command_buffer->acquire(static_cast<uint32_t>(this->m_pending_commit_storage_asset_buffers.size()), this->m_pending_commit_storage_asset_buffers.data(), static_cast<uint32_t>(this->m_pending_commit_sampled_asset_image_subresources.size()), this->m_pending_commit_sampled_asset_image_subresources.data(), 0U, NULL);

    this->m_pending_commit_storage_asset_buffers.clear();

    this->m_pending_commit_sampled_asset_image_subresources.clear();

    this->m_asset_import_upload_command_buffer->end();

    this->m_asset_import_graphics_command_buffer->end();

    this->m_upload_queue->submit_and_signal(this->m_asset_import_upload_command_buffer);

    this->m_device->reset_fence(this->m_asset_import_fence);

    this->m_graphics_queue->wait_and_submit(this->m_asset_import_upload_command_buffer, this->m_asset_import_graphics_command_buffer, this->m_asset_import_fence);

    this->m_device->wait_for_fence(this->m_asset_import_fence);

    for (brx_staging_upload_buffer *const staging_upload_buffer : this->m_pending_commit_staging_upload_buffers)
    {
        this->m_device->destroy_staging_upload_buffer(staging_upload_buffer);
    }
    this->m_pending_commit_staging_upload_buffers.clear();

    this->m_device->reset_upload_command_buffer(this->m_asset_import_upload_command_buffer);

    this->m_device->reset_graphics_command_buffer(this->m_asset_import_graphics_command_buffer);

    this->m_asset_import_upload_command_buffer->begin();

    this->m_asset_import_graphics_command_buffer->begin();
}
