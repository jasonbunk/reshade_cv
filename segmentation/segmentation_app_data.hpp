// Copyright (C) 2023 Jason Bunk
#pragma once
#include "shader_types.hpp"
#include "resource_helper.hpp"
#include "draws_counting_data_buffer.hpp"
#include "segmentation_shadering/custom_shader_layout_registers.hpp"
#include "buffer_indexing_colorization.hpp"
#include <reshade.hpp>
#include <unordered_set>
#include <nlohmann/json.hpp>

struct __declspec(uuid("bab8ffb9-5e8b-4b48-b4f2-4bac4bf87e82")) segmentation_app_data : public segmentation_app_buffer_indexing_colorization {
	std::unordered_set<uint64_t> rsrcs_of_presented_rgb;
	reshade::api::resource rsrc_of_presented_depth;
	bool do_intercept_draw = false;
	bool save_buf_tex_at_end_of_draw = false;

	// log once across threads
	std::atomic<int> logged_device_on_draw_bind_api_compatibility = { 0 };

	// shader customization data
	std::unordered_map<shader_hash_t, bytebuf*> shader_hash_to_custom_shader_bytes;
	std::unordered_map<uint64_t, custom_shader_layout_registers> pipeline_handle_to_shader_layout_registers;
	std::unordered_map<uint64_t, uint64_t> pipeline_handle_to_pipeline_layout_handle;
	std::unordered_map<uint64_t, shader_hash_t> pipeline_handle_to_pixel_shader_hash;
	std::unordered_map<uint64_t, shader_hash_t> pipeline_handle_to_vertex_shader_hash;
	resource_helper_texture r_accum_bonus; // our custom render target texture
	draws_counting_data_buffer<perdraw_metadata_type> r_counter_buf; // store metadata for tracked draws

	// for saving segmentation results to disk
	bool copy_and_index_seg_tex_needing_resource_barrier_into_packedbuf_and_metajson(
		reshade::api::command_queue* queue, simple_packed_buf& segBuf, simple_packed_buf& triBuf, nlohmann::json& dstMetaJson);
};
