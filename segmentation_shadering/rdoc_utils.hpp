// Copyright (C) 2023 Jason Bunk
#pragma once
#ifdef RENDERDOC_FOR_SHADERS
#include "api/replay/replay_enums.h" // renderdoc header
#include "segmentation_shadering/graphics_api_enum.hpp"
#include <string>

inline GraphicsAPI my_graphics_api_to_renderdoc_api(my_graphics_api::api_enum resh_api) {
	switch (resh_api) {
	case my_graphics_api::d3d9:  return GraphicsAPI::D3D11; // renderdoc doesnt support d3d9,  but we only need parts of renderdoc
	case my_graphics_api::d3d10: return GraphicsAPI::D3D11; // renderdoc doesnt support d3d10, but we only need parts of renderdoc
	case my_graphics_api::d3d11: return GraphicsAPI::D3D11;
	case my_graphics_api::d3d12: return GraphicsAPI::D3D12;
	case my_graphics_api::opengl: return GraphicsAPI::OpenGL;
	case my_graphics_api::vulkan: return GraphicsAPI::Vulkan;
	}
	//reshade::log_message(reshade::log_level::error,
	//	std::string(std::string("UNKNOWN DEVICE API ") + std::to_string(static_cast<int>(resh_api))).c_str());
	return GraphicsAPI::D3D11;
}
#endif
