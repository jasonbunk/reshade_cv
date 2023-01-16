/*
 * Source: reshade addon examples: texture_dump/save_texture.cpp
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */
#include "tex_buffer_utils.h"

void unpack_r5g6b5(uint16_t data, uint8_t rgb[3])
{
	uint32_t temp;
	temp = (data >> 11) * 255 + 16;
	rgb[0] = static_cast<uint8_t>((temp / 32 + temp) / 32);
	temp = ((data & 0x07E0) >> 5) * 255 + 32;
	rgb[1] = static_cast<uint8_t>((temp / 64 + temp) / 64);
	temp = (data & 0x001F) * 255 + 16;
	rgb[2] = static_cast<uint8_t>((temp / 32 + temp) / 32);
}

#define TENBITS_MASK ((1u << 10u) - 1u)
void r10g10b10a2_to_r8g8b8(uint32_t data, uint8_t rgb[3]) {
	rgb[0] = static_cast<uint8_t>((data & TENBITS_MASK) >> 2u);
	rgb[1] = static_cast<uint8_t>((data & (TENBITS_MASK << 10u)) >> 12u);
	rgb[2] = static_cast<uint8_t>((data & (TENBITS_MASK << 20u)) >> 22u);
}

void unpack_bc1_value(const uint8_t color_0[3], const uint8_t color_1[3], uint32_t color_index, uint8_t result[4], bool not_degenerate)
{
	switch (color_index)
	{
	case 0:
		for (int c = 0; c < 3; ++c)
			result[c] = color_0[c];
		result[3] = 255;
		break;
	case 1:
		for (int c = 0; c < 3; ++c)
			result[c] = color_1[c];
		result[3] = 255;
		break;
	case 2:
		for (int c = 0; c < 3; ++c)
			result[c] = not_degenerate ? (2 * color_0[c] + color_1[c]) / 3 : (color_0[c] + color_1[c]) / 2;
		result[3] = 255;
		break;
	case 3:
		for (int c = 0; c < 3; ++c)
			result[c] = not_degenerate ? (color_0[c] + 2 * color_1[c]) / 3 : 0;
		result[3] = not_degenerate ? 255 : 0;
		break;
	}
}

void unpack_bc4_value(uint8_t alpha_0, uint8_t alpha_1, uint32_t alpha_index, uint8_t *result)
{
	const bool interpolation_type = alpha_0 > alpha_1;

	switch (alpha_index)
	{
	case 0:
		*result = alpha_0;
		break;
	case 1:
		*result = alpha_1;
		break;
	case 2:
		*result = interpolation_type ? (6 * alpha_0 + 1 * alpha_1) / 7 : (4 * alpha_0 + 1 * alpha_1) / 5;
		break;
	case 3:
		*result = interpolation_type ? (5 * alpha_0 + 2 * alpha_1) / 7 : (3 * alpha_0 + 2 * alpha_1) / 5;
		break;
	case 4:
		*result = interpolation_type ? (4 * alpha_0 + 3 * alpha_1) / 7 : (2 * alpha_0 + 3 * alpha_1) / 5;
		break;
	case 5:
		*result = interpolation_type ? (3 * alpha_0 + 4 * alpha_1) / 7 : (1 * alpha_0 + 4 * alpha_1) / 5;
		break;
	case 6:
		*result = interpolation_type ? (2 * alpha_0 + 5 * alpha_1) / 7 : 0;
		break;
	case 7:
		*result = interpolation_type ? (1 * alpha_0 + 6 * alpha_1) / 7 : 255;
		break;
	}
}

void bc1_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data) {
	uint8_t *data_p = static_cast<uint8_t *>(data.data);
	const uint32_t block_count_x = (desc.texture.width + 3) / 4;
	const uint32_t block_count_y = (desc.texture.height + 3) / 4;
	// See https://docs.microsoft.com/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1
	for (size_t block_y = 0; block_y < block_count_y; ++block_y, data_p += data.row_pitch)
	{
		for (size_t block_x = 0; block_x < block_count_x; ++block_x)
		{
			const uint8_t *const src = data_p + block_x * 8;

			const uint16_t color_0 = *reinterpret_cast<const uint16_t *>(src);
			const uint16_t color_1 = *reinterpret_cast<const uint16_t *>(src + 2);
			const uint32_t color_i = *reinterpret_cast<const uint32_t *>(src + 4);

			uint8_t color_0_rgb[3];
			unpack_r5g6b5(color_0, color_0_rgb);
			uint8_t color_1_rgb[3];
			unpack_r5g6b5(color_1, color_1_rgb);
			const bool degenerate = color_0 > color_1;

			for (int y = 0; y < 4; ++y)
			{
				for (int x = 0; x < 4; ++x)
				{
					uint8_t *const dst = dstBuf.data<uint8_t>() + ((block_y * 4 + y) * desc.texture.width + (block_x * 4 + x)) * 4;

					unpack_bc1_value(color_0_rgb, color_1_rgb, (color_i >> (2 * (y * 4 + x))) & 0x3, dst, degenerate);
				}
			}
		}
	}
}

void bc3_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data) {
	uint8_t *data_p = static_cast<uint8_t *>(data.data);
	const uint32_t block_count_x = (desc.texture.width + 3) / 4;
	const uint32_t block_count_y = (desc.texture.height + 3) / 4;
	// See https://docs.microsoft.com/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc3
	for (size_t block_y = 0; block_y < block_count_y; ++block_y, data_p += data.row_pitch)
	{
		for (size_t block_x = 0; block_x < block_count_x; ++block_x)
		{
			const uint8_t *const src = data_p + block_x * 16;

			const uint8_t  alpha_0 = src[0];
			const uint8_t  alpha_1 = src[1];
			const uint64_t alpha_i =
				(static_cast<uint64_t>(src[2])) |
				(static_cast<uint64_t>(src[3]) << 8) |
				(static_cast<uint64_t>(src[4]) << 16) |
				(static_cast<uint64_t>(src[5]) << 24) |
				(static_cast<uint64_t>(src[6]) << 32) |
				(static_cast<uint64_t>(src[7]) << 40);

			const uint16_t color_0 = *reinterpret_cast<const uint16_t *>(src + 8);
			const uint16_t color_1 = *reinterpret_cast<const uint16_t *>(src + 10);
			const uint32_t color_i = *reinterpret_cast<const uint32_t *>(src + 12);

			uint8_t color_0_rgb[3];
			unpack_r5g6b5(color_0, color_0_rgb);
			uint8_t color_1_rgb[3];
			unpack_r5g6b5(color_1, color_1_rgb);

			for (int y = 0; y < 4; ++y)
			{
				for (int x = 0; x < 4; ++x)
				{
					uint8_t *const dst = dstBuf.data<uint8_t>() + ((block_y * 4 + y) * desc.texture.width + (block_x * 4 + x)) * 4;

					unpack_bc1_value(color_0_rgb, color_1_rgb, (color_i >> (2 * (y * 4 + x))) & 0x3, dst);
					unpack_bc4_value(alpha_0, alpha_1, (alpha_i >> (3 * (y * 4 + x))) & 0x7, dst + 3);
				}
			}
		}
	}
}

void bc4_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data) {
	uint8_t *data_p = static_cast<uint8_t *>(data.data);
	const uint32_t block_count_x = (desc.texture.width + 3) / 4;
	const uint32_t block_count_y = (desc.texture.height + 3) / 4;
	// See https://docs.microsoft.com/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4
	for (size_t block_y = 0; block_y < block_count_y; ++block_y, data_p += data.row_pitch)
	{
		for (size_t block_x = 0; block_x < block_count_x; ++block_x)
		{
			const uint8_t *const src = data_p + block_x * 8;

			const uint8_t  red_0 = src[0];
			const uint8_t  red_1 = src[1];
			const uint64_t red_i =
				(static_cast<uint64_t>(src[2])) |
				(static_cast<uint64_t>(src[3]) << 8) |
				(static_cast<uint64_t>(src[4]) << 16) |
				(static_cast<uint64_t>(src[5]) << 24) |
				(static_cast<uint64_t>(src[6]) << 32) |
				(static_cast<uint64_t>(src[7]) << 40);

			for (int y = 0; y < 4; ++y)
			{
				for (int x = 0; x < 4; ++x)
				{
					uint8_t *const dst = dstBuf.data<uint8_t>() + ((block_y * 4 + y) * desc.texture.width + (block_x * 4 + x)) * 4;

					unpack_bc4_value(red_0, red_1, (red_i >> (3 * (y * 4 + x))) & 0x7, dst);
					dst[1] = dst[0];
					dst[2] = dst[0];
					dst[3] = 255;
				}
			}
		}
	}
}

void bc5_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data) {
	uint8_t *data_p = static_cast<uint8_t *>(data.data);
	const uint32_t block_count_x = (desc.texture.width + 3) / 4;
	const uint32_t block_count_y = (desc.texture.height + 3) / 4;
	// See https://docs.microsoft.com/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5
	for (size_t block_y = 0; block_y < block_count_y; ++block_y, data_p += data.row_pitch)
	{
		for (size_t block_x = 0; block_x < block_count_x; ++block_x)
		{
			const uint8_t *const src = data_p + block_x * 16;

			const uint8_t  red_0 = src[0];
			const uint8_t  red_1 = src[1];
			const uint64_t red_i =
				(static_cast<uint64_t>(src[2])) |
				(static_cast<uint64_t>(src[3]) << 8) |
				(static_cast<uint64_t>(src[4]) << 16) |
				(static_cast<uint64_t>(src[5]) << 24) |
				(static_cast<uint64_t>(src[6]) << 32) |
				(static_cast<uint64_t>(src[7]) << 40);

			const uint8_t  green_0 = src[8];
			const uint8_t  green_1 = src[9];
			const uint64_t green_i =
				(static_cast<uint64_t>(src[10])) |
				(static_cast<uint64_t>(src[11]) << 8) |
				(static_cast<uint64_t>(src[12]) << 16) |
				(static_cast<uint64_t>(src[13]) << 24) |
				(static_cast<uint64_t>(src[14]) << 32) |
				(static_cast<uint64_t>(src[15]) << 40);

			for (int y = 0; y < 4; ++y)
			{
				for (int x = 0; x < 4; ++x)
				{
					uint8_t *const dst = dstBuf.data<uint8_t>() + ((block_y * 4 + y) * desc.texture.width + (block_x * 4 + x)) * 4;

					unpack_bc4_value(red_0, red_1, (red_i >> (3 * (y * 4 + x))) & 0x7, dst);
					unpack_bc4_value(green_0, green_1, (green_i >> (3 * (y * 4 + x))) & 0x7, dst + 1);
					dst[2] = 0;
					dst[3] = 255;
				}
			}
		}
	}
}
