/*
 * Portions of this file are from reshade:
 *     Copyright (C) 2021 Patrick Mours
 *     SPDX-License-Identifier: BSD-3-Clause OR MIT
 * Other portions Copyright (C) 2022 Jason Bunk
 */
#include <reshade.hpp>
#include "copy_texture_into_packedbuf.h"
#include "tex_buffer_utils.h"

using namespace reshade::api;

// https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
// returns integer ceil(x,y)
template<typename T>
T ceil_int(T x, T y) {
	return (x + y - static_cast<T>(1)) / y;
}

void depth_gray_bytesLE_to_f32(simple_packed_buf &dstBuf, const resource_desc &desc, const subresource_data &data,
							size_t byteskeephint, const depth_tex_settings &settings) {
	if (dstBuf.pixfmt != BUF_PIX_FMT_GRAYF32) {
		reshade::log_message(1, std::string(std::string("depth_gray_bytesLE_to_f32: dstBuf.pixfmt ") + std::to_string(static_cast<int64_t>(dstBuf.pixfmt))).c_str());
		return;
	}
	const size_t settings_depthbyteskeep = static_cast<size_t>(std::max(1, settings.depthbyteskeep));
	const size_t settings_depthbytes = static_cast<size_t>(std::max(1, settings.depthbytes));
	const size_t srcbytesfromformatrowptch = format_row_pitch(desc.texture.format, desc.texture.width) / desc.texture.width;
	const size_t srcpixbytes = (settings.depthbytes <= 0) ? srcbytesfromformatrowptch : settings_depthbytes;
	const size_t depthbytes2keep = settings.depthbyteskeep <= 0 ? std::min(byteskeephint, srcpixbytes) : std::min(srcpixbytes, settings_depthbyteskeep);
	const size_t rowpitch = (settings.adjustpitchhack == 0) ? (data.row_pitch) : ((settings.adjustpitchhack > 0) ? (data.row_pitch << static_cast<uint32_t>(settings.adjustpitchhack))
																												 : (data.row_pitch >> static_cast<uint32_t>(-settings.adjustpitchhack)));
	//const uint64_t subfromme = (1ull << (8ull * depthbytes2keep));
	const uint64_t subfromme = 1ull << (settings.depthnormalization <= 0 ? (8ull * depthbytes2keep) : static_cast<uint64_t>(settings.depthnormalization));
	const double subfrmflt = static_cast<double>(subfromme);
	if (settings.debug_mode || settings.more_verbose) {
		reshade::log_message(3, std::string(std::string("depthgray: srcpixbytes ") + std::to_string(srcpixbytes)
			+ std::string(", srcbytesfromformatrowptch ") + std::to_string(srcbytesfromformatrowptch)
			+ std::string(", data.row_pitch ") + std::to_string(data.row_pitch)
			+ std::string(", rowpitch ") + std::to_string(rowpitch)
			+ std::string(", settingsBppx ") + std::to_string(settings_depthbyteskeep)
			+ std::string(", depthbytes2keep ") + std::to_string(depthbytes2keep)
			+ std::string(", byteskeephint ") + std::to_string(byteskeephint)
			+ std::string(", settings.depthbyteskeep ") + std::to_string(settings.depthbyteskeep)
			+ std::string(", desc.texture.width ") + std::to_string(desc.texture.width)
			+ std::string(", subfromme ") + std::to_string(subfromme)
			+ std::string(", subtractfrom? ") + std::to_string(settings.subtractfrom)
			+ std::string(", reciprocal? ") + std::to_string(settings.reciprocal)
		).c_str());
	}
	if (settings.alreadyfloat) {
		if (srcpixbytes == sizeof(float) && depthbytes2keep == sizeof(float)) {}
		else {
			reshade::log_message(1, "ERROR: settings.alreadyfloat() but invalid bytes per pix calculations");
			return;
		}
	}
	uint8_t *src_p = static_cast<uint8_t *>(data.data);
	uint64_t maxv = 0ull;
	uint64_t minv = (1ull << 62ull);
	float *dst_p;
	float *src_f;
	uint8_t endianflip[8];
	uint64_t vi;
	size_t x, y, z;
	double vf;
	for (y = 0; y < desc.texture.height; ++y, src_p += rowpitch) {
		dst_p = dstBuf.rowptr<float>(y);
		if (!settings.debug_mode) {
			if (!settings.alreadyfloat) {
				for (x = 0; x < desc.texture.width; ++x) {
					const uint8_t *const src = src_p + x * srcpixbytes;
					vi = 0;
					for (z = 0; z < depthbytes2keep; ++z) {
						vi += static_cast<uint64_t>(src[z]) << (8ull * z);
					}
					if (maxv < vi) maxv = vi;
					if (minv > vi) minv = vi;
					vf = settings.subtractfrom ? static_cast<double>(subfromme - vi) : static_cast<double>(vi);
					dst_p[x] = settings.reciprocal ? static_cast<float>(subfrmflt / std::max(0.1, vf)) : static_cast<float>(vf);
				}
			} else {
				if (settings.float_reverse_endian) {
					for (x = 0; x < desc.texture.width; ++x) {
						const uint8_t *const src = src_p + x * srcpixbytes;
						endianflip[3] = src[0];
						endianflip[2] = src[1];
						endianflip[1] = src[2];
						endianflip[0] = src[3];
						dst_p[x] = *reinterpret_cast<float *>(endianflip);
					}
				}
				else {
					src_f = reinterpret_cast<float *>(src_p);
					for (x = 0; x < desc.texture.width; ++x) {
						dst_p[x] = src_f[x];
					}
				}
			}
		} else {
			vi = ceil_int<uint64_t>(desc.texture.width, srcpixbytes);
			for (x = 0; x < desc.texture.width; ++x) {
				const uint8_t *const src = src_p + x * srcpixbytes;
				dst_p[x] = static_cast<float>(src[x / vi]);
			}
		}
	}
	if (settings.more_verbose) {
		reshade::log_message(3, std::string(std::string("depth_gray_bytesLE_to_f32: min ") + std::to_string(minv) + std::string(", max ") + std::to_string(maxv)).c_str());
	}
}

bool copy_texture_image_given_ready_resource_into_packedbuf(
	GameInterface *gamehandle, simple_packed_buf &dstBuf,
	const resource_desc &desc, const subresource_data &data,\
	bool isdepth, const depth_tex_settings& depth_settings)
{
	dstBuf.width = desc.texture.width;
	dstBuf.height = desc.texture.height;

	uint8_t *data_p = static_cast<uint8_t *>(data.data);

	switch (desc.texture.format)
	{
	case format::l8_unorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = src[0];
				dst[1] = src[0];
				dst[2] = src[0];
				dst[3] = 255;
			}
		}
		break;
	case format::a8_unorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = 0;
				dst[1] = 0;
				dst[2] = 0;
				dst[3] = src[0];
			}
		}
		break;
	case format::r8_typeless:
	case format::r8_unorm:
	case format::r8_snorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = src[0];
				dst[1] = 0;
				dst[2] = 0;
				dst[3] = 255;
			}
		}
		break;
	case format::l8a8_unorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x * 2;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = src[0];
				dst[1] = src[0];
				dst[2] = src[0];
				dst[3] = src[1];
			}
		}
		break;
	case format::r8g8_typeless:
	case format::r8g8_unorm:
	case format::r8g8_snorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x * 2;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = 0;
				dst[3] = 255;
			}
		}
		break;
	case format::r8g8b8a8_typeless:
	case format::r8g8b8a8_unorm:
	case format::r8g8b8a8_unorm_srgb:
	case format::r8g8b8x8_typeless:
	case format::r8g8b8x8_unorm:
	case format::r8g8b8x8_unorm_srgb:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x * 4;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = isdepth ? src[3] : 255;
			}
		}
		break;
	case format::b8g8r8a8_typeless:
	case format::b8g8r8a8_unorm:
	case format::b8g8r8a8_unorm_srgb:
	case format::b8g8r8x8_typeless:
	case format::b8g8r8x8_unorm:
	case format::b8g8r8x8_unorm_srgb:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		for (size_t y = 0; y < desc.texture.height; ++y, data_p += data.row_pitch)
		{
			for (size_t x = 0; x < desc.texture.width; ++x)
			{
				const uint8_t *const src = data_p + x * 4;
				uint8_t *const dst = dstBuf.entryptr<uint8_t>(y, x);

				// Swap red and blue channel
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst[3] = isdepth ? src[3] : 255;
			}
		}
		break;
	case format::bc1_typeless:
	case format::bc1_unorm:
	case format::bc1_unorm_srgb:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		bc1_block_copy(dstBuf, desc, data);
		break;
	case format::bc3_typeless:
	case format::bc3_unorm:
	case format::bc3_unorm_srgb:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		bc3_block_copy(dstBuf, desc, data);
		break;
	case format::bc4_typeless:
	case format::bc4_unorm:
	case format::bc4_snorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		bc4_block_copy(dstBuf, desc, data);
		break;
	case format::bc5_typeless:
	case format::bc5_unorm:
	case format::bc5_snorm:
		if (!dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_RGBA)) return false;
		bc5_block_copy(dstBuf, desc, data);
		break;
	case format::r24_g8_typeless: // crysis depth: "DXGI_FORMAT_R24G8_TYPELESS: A two-component, 32-bit typeless format that supports 24 bits for the red channel and 8 bits for the green channel."
		if (!isdepth || !dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_GRAYF32)) return false;
		depth_gray_bytesLE_to_f32(dstBuf, desc, data, 3, depth_settings);
		break;
	case format::r32_typeless: // Control depth
	case format::r32_g8_typeless: // cyberpunk depth map: "DXGI_FORMAT_R32G8X24_TYPELESS: A two-component, 64-bit typeless format that supports 32 bits for the red channel, 8 bits for the green channel, and 24 bits are unused."
		if (!isdepth || !dstBuf.set_pixfmt_and_alloc_bytes(BUF_PIX_FMT_GRAYF32)) return false;
		depth_gray_bytesLE_to_f32(dstBuf, desc, data, 4, depth_settings);
		break;
	default: {
		// Unsupported format
		reshade::log_message(1, std::string(std::string("Failed to save texture: unsupported texture format")+std::to_string(static_cast<int>(desc.texture.format))).c_str());
		return false;
	}
	}
	if (depth_settings.more_verbose) {
		reshade::log_message(3, std::string(std::string("copied texture with format ") + std::to_string(static_cast<int>(desc.texture.format))).c_str());
	}
	return true;
}


bool copy_texture_image_needing_resource_barrier_into_packedbuf(
	GameInterface *gamehandle, simple_packed_buf &dstBuf,
	reshade::api::command_queue *queue, reshade::api::resource tex,
	bool isdepth, const depth_tex_settings &depth_settings)
{
	device *const device = queue->get_device();
	const resource_desc desc = device->get_resource_desc(tex);

	uint32_t row_pitch = format_row_pitch(desc.texture.format, desc.texture.width);
	if (device->get_api() == device_api::d3d12) // Align row pitch to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256)
		row_pitch = (row_pitch + 255) & ~255;
	const uint32_t slice_pitch = format_slice_pitch(desc.texture.format, row_pitch, desc.texture.height);

	resource intermediate;
	if (desc.heap != memory_heap::gpu_only)
	{
		// Avoid copying to temporary system memory resource if texture is accessible directly
		intermediate = tex;
	}
	else if (device->check_capability(device_caps::copy_buffer_to_texture))
	{
		if ((desc.usage & resource_usage::copy_source) != resource_usage::copy_source) {
			reshade::log_message(1, std::string(std::string("Failed to save texture: bad desc.usage ") + std::to_string((int64_t)(desc.usage))).c_str());
			return false;
		}

		if (!device->create_resource(resource_desc(slice_pitch, memory_heap::gpu_to_cpu, resource_usage::copy_dest), nullptr, resource_usage::copy_dest, &intermediate))
		{
			reshade::log_message(1, "Failed to create system memory buffer for texture dumping!");
			return false;
		}

		command_list *const cmd_list = queue->get_immediate_command_list();
		cmd_list->barrier(tex, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->copy_texture_to_buffer(tex, 0, nullptr, intermediate, 0, desc.texture.width, desc.texture.height);
		cmd_list->barrier(tex, resource_usage::copy_source, resource_usage::shader_resource);
	}
	else
	{
		if ((desc.usage & resource_usage::copy_source) != resource_usage::copy_source) {
			reshade::log_message(1, std::string(std::string("Failed to save texture: bad desc.usage ") + std::to_string((int64_t)(desc.usage))).c_str());
			return false;
		}

		if (!device->create_resource(resource_desc(desc.texture.width, desc.texture.height, 1, 1, format_to_default_typed(desc.texture.format), 1, memory_heap::gpu_to_cpu, resource_usage::copy_dest), nullptr, resource_usage::copy_dest, &intermediate))
		{
			reshade::log_message(1, "Failed to create system memory texture for texture dumping!");
			return false;
		}

		command_list *const cmd_list = queue->get_immediate_command_list();
		cmd_list->barrier(tex, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->copy_texture_region(tex, 0, nullptr, intermediate, 0, nullptr);
		cmd_list->barrier(tex, resource_usage::copy_source, resource_usage::shader_resource);
	}

	queue->wait_idle();

	subresource_data mapped_data = {};
	if (desc.heap == memory_heap::gpu_only &&
		device->check_capability(device_caps::copy_buffer_to_texture))
	{
		device->map_buffer_region(intermediate, 0, std::numeric_limits<uint64_t>::max(), map_access::read_only, &mapped_data.data);

		mapped_data.row_pitch = row_pitch;
		mapped_data.slice_pitch = slice_pitch;
	}
	else
	{
		device->map_texture_region(intermediate, 0, nullptr, map_access::read_only, &mapped_data);
	}

	bool wasok = false;

	if (mapped_data.data != nullptr)
	{
		wasok = copy_texture_image_given_ready_resource_into_packedbuf(gamehandle, dstBuf, desc, mapped_data, isdepth, depth_settings);

		if (desc.heap == memory_heap::gpu_only &&
			device->check_capability(device_caps::copy_buffer_to_texture))
			device->unmap_buffer_region(intermediate);
		else
			device->unmap_texture_region(intermediate, 0);
	} else {
		reshade::log_message(1, "Failed to save texture: mapped_data.data == nullptr");
	}

	if (intermediate != tex)
		device->destroy_resource(intermediate);

	return wasok;
}
