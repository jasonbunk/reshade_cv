// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#ifdef RENDERDOC_FOR_SHADERS
#include "api/replay/replay_enums.h" // renderdoc header
#include <sstream>

inline GraphicsAPI reshade_api_to_renderdoc_api(reshade::api::device_api resh_api) {
	switch (resh_api) {
	case reshade::api::device_api::d3d9:  return GraphicsAPI::D3D11; // renderdoc doesnt support d3d9,  but we only need parts of renderdoc
	case reshade::api::device_api::d3d10: return GraphicsAPI::D3D11; // renderdoc doesnt support d3d10, but we only need parts of renderdoc
	case reshade::api::device_api::d3d11: return GraphicsAPI::D3D11;
	case reshade::api::device_api::d3d12: return GraphicsAPI::D3D12;
	case reshade::api::device_api::opengl: return GraphicsAPI::OpenGL;
	case reshade::api::device_api::vulkan: return GraphicsAPI::Vulkan;
	}
	reshade::log_message(reshade::log_level::error,
		std::string(std::string("UNKNOWN RESHADE DEVICE API ") + std::to_string(static_cast<int>(resh_api))).c_str());
	return GraphicsAPI::D3D11;
}
#endif
