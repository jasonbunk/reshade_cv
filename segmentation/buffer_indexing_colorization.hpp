// Copyright (C) 2023 Jason Bunk
#pragma once
#include "gcv_utils/simple_packed_buf.h"
#include "gcv_utils/typed_2d_array.hpp"
#include "shader_types.hpp"
#include <reshade.hpp>
#include <unordered_map>
#include <thread>

enum ColorizationVizMode : uint32_t {
	CVM_BufChannel0 = 0,
	CVM_BufChannel1,
	CVM_BufChannel2,
	CVM_FullMetaHash,
	CVM_ShaderIDs,
	CVM_DrawInstance,
	CVM_PrimitiveHash,
	CVM_number_of_modes,
};
constexpr const char* ColorizationVizModeNames[] = {
	"buf[0]",
	"buf[1]",
	"buf[2]",
	"full_meta_hash",
	"shader_ids",
	"instance",
	"triangle",
};

struct segmentation_app_buffer_indexing_colorization
{
	reshade::api::resource viz_intmdt_resource_copydest = { 0ull };
	simple_packed_buf viz_seg_colorized_for_display;
	ColorizationVizMode viz_seg_colorization_mode = CVM_FullMetaHash;
	int viz_seg_colorization_seed = 0;
	typed_2d_array<perdraw_metadata_type> draw_metadata_seg_image;
	std::array<std::thread, 4> row_colorization_threads;
	std::vector<uint32_t> row_colorization_thread_const_rowidxs_bulk;

	inline void delete_resources(reshade::api::device* device) {
		if (device == nullptr) return;
		if (viz_intmdt_resource_copydest.handle != 0ull) {
			device->destroy_resource(viz_intmdt_resource_copydest);
			viz_intmdt_resource_copydest.handle = 0ull;
		}
	}
};

void custom_shader_buffer_visualization_on_reshade_begin_effects(reshade::api::effect_runtime* runtime,
	reshade::api::command_list* cmd_list, reshade::api::resource_view rtv, reshade::api::resource_view);

void imgui_draw_custom_shader_debug_viz_in_reshade_overlay(reshade::api::effect_runtime* runtime);
