// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#include "segmentation_shadering/graphics_api_enum.hpp"
#include <string>

inline my_graphics_api::api_enum reshade_api_to_my_graphics_api(reshade::api::device_api resh_api) {
	switch (resh_api) {
	case reshade::api::device_api::d3d9:  return my_graphics_api::d3d9;
	case reshade::api::device_api::d3d10: return my_graphics_api::d3d10;
	case reshade::api::device_api::d3d11: return my_graphics_api::d3d11;
	case reshade::api::device_api::d3d12: return my_graphics_api::d3d12;
	case reshade::api::device_api::opengl: return my_graphics_api::opengl;
	case reshade::api::device_api::vulkan: return my_graphics_api::vulkan;
	}
	reshade::log_message(reshade::log_level::error,
		std::string(std::string("UNKNOWN RESHADE DEVICE API ") + std::to_string(static_cast<int>(resh_api))).c_str());
	return my_graphics_api::d3d11;
}