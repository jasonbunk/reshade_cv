#pragma once
/*
 * Source: reshade addon examples: texture_dump/save_texture.cpp
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 */
#include <stdint.h>
#include <reshade.hpp>
#include <string>
#include "gcv_utils/simple_packed_buf.h"

void unpack_r5g6b5(uint16_t data, uint8_t rgb[3]);
void r10g10b10a2_to_r8g8b8(uint32_t data, uint8_t rgb[3]);
void unpack_bc1_value(const uint8_t color_0[3], const uint8_t color_1[3], uint32_t color_index, uint8_t result[4], bool not_degenerate = true);
void unpack_bc4_value(uint8_t alpha_0, uint8_t alpha_1, uint32_t alpha_index, uint8_t *result);

void bc1_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data);
void bc3_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data);
void bc4_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data);
void bc5_block_copy(simple_packed_buf &dstBuf, const reshade::api::resource_desc &desc, const reshade::api::subresource_data &data);
