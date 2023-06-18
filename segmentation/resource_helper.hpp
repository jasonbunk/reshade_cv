// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#include <atomic>
#include "render_target_stats/reshade_tex_format_info.hpp"

class resource_helper {
protected:
	bool attemptedcreation = false;
	bool isvalid = false;
public:
	reshade::api::resource rsc = { 0 };

	inline bool is_valid() const { return isvalid; }

	virtual void delete_resource(reshade::api::device* device) {
		if (isvalid) {
			device->destroy_resource(rsc);
		}
		isvalid = false;
		attemptedcreation = false;
	}
	
	inline bool create_or_resize_texture(reshade::api::device* device, uint32_t width, uint32_t height, reshade::api::format fmt, reshade::api::resource_usage resourceusage) {
		if (isvalid) {
			reshade::api::resource_desc old = device->get_resource_desc(rsc);
			if (old.texture.width == width && old.texture.height == height) {
				return true;
			} else {
				delete_resource(device);
			}
		} else if (attemptedcreation) {
			return false; // dont bother trying more than once
		}
		attemptedcreation = true;

		const std::string dstr = std::string(" shape ")+std::to_string(width)+std::string(" x ")+std::to_string(height)+std::string(" of format ")+reshade::api::fmtnames.at(fmt);

		reshade::api::resource_desc desc(width, height, 1, 1, fmt, 1, reshade::api::memory_heap::gpu_only, resourceusage);

		if (!device->create_resource(desc, nullptr, resourceusage, &rsc)) {
			reshade::log_message(reshade::log_level::error, std::string(std::string("resource_helper: Failed to create resource of")+dstr).c_str());
			return false;
		}

		isvalid = true;
		reshade::log_message(reshade::log_level::info, "resource_helper: successfully created");
		return isvalid;
	}
};


struct resource_helper_texture : public resource_helper {
	reshade::api::resource_view rtv = { 0 };
	std::atomic<int> iscleared = { 0 };
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	virtual void delete_resource(reshade::api::device* device) override {
		if (isvalid) {
			device->destroy_resource_view(rtv);
			device->destroy_resource(rsc);
		}
		isvalid = false;
		attemptedcreation = false;
	}

	inline bool create_or_resize_texture(reshade::api::device* device, uint32_t width, uint32_t height, reshade::api::format fmt, reshade::api::resource_usage resourceusage) {
		if (isvalid) {
			reshade::api::resource_desc old = device->get_resource_desc(rsc);
			if (old.texture.width == width && old.texture.height == height) {
				return true;
			} else {
				delete_resource(device);
			}
		} else if (attemptedcreation) {
			return false; // dont bother trying more than once
		}
		attemptedcreation = true;

		const std::string dstr = std::string(" shape ")+std::to_string(width)+std::string(" x ")+std::to_string(height)+std::string(" of format ")+reshade::api::fmtnames.at(fmt);

		reshade::api::resource_desc desc(width, height, 1, 1, fmt, 1, reshade::api::memory_heap::gpu_only, resourceusage);

		if (!device->create_resource(desc, nullptr, resourceusage, &rsc)) {
			reshade::log_message(reshade::log_level::error, std::string(std::string("resource_helper_tex: Failed to create resource of")+dstr).c_str());
			return false;
		}

		if (!device->create_resource_view(rsc, resourceusage, reshade::api::resource_view_desc(fmt), &rtv)) {
			reshade::log_message(reshade::log_level::error, std::string(std::string("resource_helper_tex: Failed to create resource view of")+dstr).c_str());
			device->destroy_resource(rsc);
			return false;
		}

		isvalid = true;
		reshade::log_message(reshade::log_level::info, std::string(std::string("resource_helper_tex: successfully created resource of ")+dstr).c_str());
		return isvalid;
	}
};
