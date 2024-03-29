// Copyright (C) 2023 Jason Bunk
#pragma once
#include "custom_shader_layout_registers.hpp"
#include "segmentation_shadering/graphics_api_enum.hpp"
#include <sstream>

bool customize_shader_dxbc_or_dxil(
    bool b_truepixel_falsevertex,
    const my_graphics_api::api_enum graphics_api,
    bytebuf* buf,
    custom_shader_layout_registers& newregisters,
    std::stringstream& log);
