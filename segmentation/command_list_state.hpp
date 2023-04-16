// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#include <vector>

class __declspec(uuid("78aad126-d069-424c-aa0a-77ee31f8c1c1")) segmentation_app_cmdlist_state {
	bool my_bonus_rtv_is_bound = false;
public:
	std::vector<reshade::api::resource_view> rtvs;
	reshade::api::resource_view dsv;
	std::unordered_map<reshade::api::pipeline_stage, reshade::api::pipeline> pipelines; // track bound shaders

	// returns true upon state change
	inline bool bind_bonus_rtv_if_not_bound(reshade::api::command_list* cmd_list, reshade::api::resource_view& extra_rtv) {
		if (!my_bonus_rtv_is_bound) {
			my_bonus_rtv_is_bound = true;
			rtvs.push_back(extra_rtv);
			cmd_list->bind_render_targets_and_depth_stencil(rtvs.size(), rtvs.data(), dsv);
			return true;
		}
		return false;
	}
	inline void unbind_bonus_rtv(reshade::api::command_list* cmd_list) {
		if (my_bonus_rtv_is_bound) {
			my_bonus_rtv_is_bound = false;
			rtvs.pop_back();
			cmd_list->bind_render_targets_and_depth_stencil(rtvs.size(), rtvs.data(), dsv);
		}
	}

	inline void reset_binding_of_bonus_rtv() {
		my_bonus_rtv_is_bound = false;
	}

	inline void reset_cmdlist() {
		my_bonus_rtv_is_bound = false;
		rtvs.clear();
		dsv = { 0 };
		pipelines.clear();
	}
};
