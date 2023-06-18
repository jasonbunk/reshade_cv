#pragma once
// Copyright (C) 2023 Jason Bunk
#include <reshade.hpp>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

struct rsc_stats {
	uint64_t total_draws = 0;
	uint64_t num_indirect_draws = 0;
	uint64_t total_vertices = 0;
	uint64_t max_num_rtvsbound = 0;
	uint64_t num_command_lists_which_touched = 0;

	void zero() {
		total_draws = 0;
		num_indirect_draws = 0;
		total_vertices = 0;
		max_num_rtvsbound = 0;
		num_command_lists_which_touched = 0;
	}
};

struct __declspec(uuid("fba6fdd8-0096-406f-be07-97e1726ad30c")) stats_accum {
	std::map<uint64_t, rsc_stats> resource2stats;
	std::mutex statsmut;
	void merge(stats_accum& source) {
		if (std::addressof(resource2stats) != std::addressof(source.resource2stats)) {
			std::lock_guard<std::mutex> guard_this(statsmut);
			std::lock_guard<std::mutex> guard_other(source.statsmut);
			for (const auto& [src_handle, src_stats] : source.resource2stats) {
				rsc_stats& sthandle = resource2stats[src_handle];
				sthandle.total_draws += src_stats.total_draws;
				sthandle.num_indirect_draws += src_stats.num_indirect_draws;
				sthandle.total_vertices += src_stats.total_vertices;
				sthandle.max_num_rtvsbound = std::max(sthandle.max_num_rtvsbound, src_stats.max_num_rtvsbound);
				sthandle.num_command_lists_which_touched += src_stats.num_command_lists_which_touched;
			}
			source.resource2stats.clear();
		}
	}
	void reset_stats() {
		std::lock_guard<std::mutex> guard(statsmut);
		resource2stats.clear();
	}
	void zero_stats() {
		std::lock_guard<std::mutex> guard(statsmut);
		for (auto& [handle, stats] : resource2stats) {
			stats.zero();
		}
	}
};

struct __declspec(uuid("c58e40fa-bf45-4e8e-9c39-6d091c5ae03f")) device_draw_stats : public stats_accum {
	uint32_t render_width = 0;
	uint32_t render_height = 0;
	// List of queues created for this device
	std::vector<reshade::api::command_queue*> queues;
	// Keep track of when each resource was last drawn to
	uint64_t frameidx = 0;
	std::unordered_map<uint64_t, uint64_t> resource2lastseenframeidx;
	std::unordered_map<uint64_t, reshade::api::format> resource2lastseendrawformat;
	// Keep track of what the user has actually clicked on
	uint64_t suggested_resource_to_click = 0;
	std::unordered_set<uint64_t> actually_clicked_resources;
};

struct __declspec(uuid("a0e48321-b043-4368-934f-387a93169e6c")) cmdlist_draw_stats : public stats_accum {
	std::vector<reshade::api::resource_view> render_targets;
	reshade::api::resource_view depth_stencil = { 0 };

	void reset_cmdlist() {
		render_targets.clear();
		depth_stencil = { 0 };
	}
};

typedef stats_accum queue_draw_stats;


void register_rgb_render_target_stats_tracking();
void unregister_rgb_render_target_stats_tracking();
void imgui_draw_rgb_render_target_stats_in_reshade_overlay(reshade::api::effect_runtime* runtime);
