// Copyright (C) 2023 Jason Bunk
#pragma once
#include "rt_resource.hpp"
#include <string>
#include <atomic>
#include <mutex>
#include <vector>

/*
* For this shader app: create one unchanging buffer of uint32,
* whose values simply count from 0 to "N".
* Then create "N" views pointing to 1 element each.
* For each draw, we can bind one of those views as a shader resource,
* so that that draw's shader can access it as the indicator of which draw call it was.
* A custom shader can draw that "draw index" as a pixel segmentation map.
* Meanwhile, metadata for each draw call is stashed here to interpret that segmentation map.
*/

template<typename MetaT>
class draws_counting_data_buffer : public rt_resource {
	std::vector<uint32_t> cpu_data;
	std::vector<reshade::api::resource_view> ridiculous_number_of_views;

	std::mutex frame_mut;
	bool counter_wrapped = false;
	std::vector<MetaT> perdraw_meta;
	uint32_t perdraw_counter = 1u;

public:
	inline void reset_at_end_of_frame() {
		std::lock_guard<std::mutex> lock(frame_mut);
		counter_wrapped = false;
		perdraw_counter = 1u; // the 0th entry is for "null" case and its metadata is never touched
	}

	inline reshade::api::resource_view get_view_for_blank_or_null_draw() const {
		std::lock_guard<std::mutex> lock(frame_mut);
		return ridiculous_number_of_views[0];
	}

	template<typename MetaT>
	void set_null_draw_metadata(const MetaT meta) {
		std::lock_guard<std::mutex> lock(frame_mut);
		if (!perdraw_meta.empty()) perdraw_meta[0] = meta;
	}

	template<typename MetaT>
	inline std::vector<MetaT> get_copy_of_frame_perdraw_metadata() {
		std::lock_guard<std::mutex> lock(frame_mut);
		if (perdraw_meta.empty()) return std::vector<MetaT>();
		return std::vector<MetaT>(perdraw_meta.begin(), perdraw_meta.begin()+perdraw_counter);
	}

	template<typename MetaT>
	inline reshade::api::resource_view stash_metadata_and_get_view_for_draw(const MetaT meta) {
		std::lock_guard<std::mutex> lock(frame_mut);
		if (ridiculous_number_of_views.empty()) {
			reshade::log_message(reshade::log_level::error, "draws_counting_data_buffer: need to create buffer before using");
			return { 0 };
		}
		if (perdraw_counter >= perdraw_meta.size()) {
			counter_wrapped = true;
			return ridiculous_number_of_views[0]; // "null" case
		}
		perdraw_meta[perdraw_counter] = meta;
		return ridiculous_number_of_views[perdraw_counter++];
	}

	inline bool create_buffer(reshade::api::device* device) {
		if (device == nullptr) return false;
		std::lock_guard<std::mutex> lock(frame_mut);
		if (attemptedcreation) return isvalid;
		attemptedcreation = true;
		const reshade::api::resource_usage resourceusage = reshade::api::resource_usage::shader_resource;

		const uint64_t num_views = 16384ull;

		reshade::api::resource_desc desc(num_views*4ull, reshade::api::memory_heap::gpu_only, resourceusage);
		cpu_data.resize(num_views*4ull);
		for (uint32_t ii = 0; ii < cpu_data.size(); ++ii) cpu_data[ii] = ii;
		reshade::api::subresource_data initial_data;
		initial_data.data = cpu_data.data();

		if (!device->create_resource(desc, &initial_data, resourceusage, &rsc)) {
			reshade::log_message(reshade::log_level::error, "draws_counting_data_buffer: Failed to create resource!");
			return false;
		}

		ridiculous_number_of_views.resize(num_views);
		perdraw_meta.resize(ridiculous_number_of_views.size());
		for(size_t ii=0; ii < ridiculous_number_of_views.size(); ++ii)
			if (!device->create_resource_view(rsc, resourceusage, reshade::api::resource_view_desc(reshade::api::format::r32_uint, ii, 1u), &(ridiculous_number_of_views[ii]))) {
				reshade::log_message(reshade::log_level::error, std::string(std::string("draws_counting_data_buffer: Failed to create resource view ")+std::to_string(ii)).c_str());
				device->destroy_resource(rsc);
				return false;
			}

		isvalid = true;
		reshade::log_message(reshade::log_level::info, std::string(std::string("draws_counting_data_buffer: successfully created counter for up to ")+std::to_string(num_views)+std::string(" draws")).c_str());
		return isvalid;
	}

	inline void delete_resources(reshade::api::device* device) {
		if (device == nullptr) return;
		std::lock_guard<std::mutex> lock(frame_mut);
		if (isvalid) {
			for (auto& rsv : ridiculous_number_of_views)
				device->destroy_resource_view(rsv);
			ridiculous_number_of_views.clear();
			device->destroy_resource(rsc);
			rsc.handle = 0ull;
		}
		isvalid = false;
		attemptedcreation = false;
	}
};
