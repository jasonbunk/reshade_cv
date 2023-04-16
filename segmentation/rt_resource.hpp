// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#include <atomic>


struct rt_resource {
	reshade::api::resource rsc = { 0 };
	bool isvalid = false;
	bool attemptedcreation = false;
};


struct rt_resource_texture : public rt_resource {
	reshade::api::resource_view rtv = { 0 };
	std::atomic<int> iscleared = { 0 };
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	inline void delete_texture(reshade::api::device* device) {
		if (isvalid) {
			device->destroy_resource_view(rtv);
			device->destroy_resource(rsc);
		}
		isvalid = false;
		attemptedcreation = false;
	}

	inline bool create_texture(reshade::api::device* device, uint32_t width, uint32_t height, reshade::api::format fmt, reshade::api::resource_usage resourceusage) {
		if (attemptedcreation) return isvalid;
		attemptedcreation = true;

		reshade::api::resource_desc desc(width, height, 1, 1, fmt, 1, reshade::api::memory_heap::gpu_only, resourceusage);

		if (!device->create_resource(desc, nullptr, resourceusage, &rsc)) {
			reshade::log_message(reshade::log_level::error, "rt_resource_tex: Failed to create resource!");
			return false;
		}

		if (!device->create_resource_view(rsc, resourceusage, reshade::api::resource_view_desc(fmt), &rtv)) {
			reshade::log_message(reshade::log_level::error, "rt_resource_tex: Failed to create resource view!");
			return false;
		}

		isvalid = true;
		reshade::log_message(reshade::log_level::info, "rt_resource_tex: successfully created");
		return isvalid;
	}
};
