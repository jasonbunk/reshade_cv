// Copyright (C) 2023 Jason Bunk
#include <imgui.h>
#include "buffer_indexing_colorization.hpp"
#include "segmentation_app_data.hpp"
#include "xxhash.h"
#include "concurrentqueue.h"
#include "colormap_util.hpp"
using namespace reshade::api;


inline uint32_t colorhashfun(const void* buf, size_t buflen, uint32_t seed) {
	uint32_t tmp = XXH32(buf, buflen, seed);
	reinterpret_cast<uint8_t*>(&tmp)[3] = 255;
	return tmp;
}

typedef std::array<uint32_t, 2> DrawInstIDbuf; // pair (Draw#, InstanceID) uniquely identifies each object within one frame


void multithreaded_row_colorization_worker(moodycamel::ConcurrentQueue<uint32_t>* row_queue,
	const uint8_t* datastartptr, uint32_t row_stride_bytes, uint32_t row_width_pix,
	std::vector<perdraw_metadata_type> const* const draw_metadata,
	segmentation_app_buffer_indexing_colorization* mapp)
{
	const size_t one_minus_draw_meta_size = draw_metadata->size() - 1ull;
	uint8_t primbuf[sizeof(perdraw_metadata_type) + sizeof(uint32_t)];
	DrawInstIDbuf objbuf;
	uint32_t seg_idx_color = 0u;
	uint32_t rowidx = 0;
	while (row_queue->try_dequeue(rowidx)) {
		const uint32_t* inrowptr = reinterpret_cast<const uint32_t*>(datastartptr + rowidx * row_stride_bytes);
		perdraw_metadata_type* outmetarowptr = mapp->draw_metadata_seg_image.rowptr(rowidx);
		for (size_t x = 0; x < row_width_pix; ++x) {
			outmetarowptr[x] = (*draw_metadata)[std::min<size_t>(one_minus_draw_meta_size, inrowptr[x * 4u])];
			switch (mapp->viz_seg_colorization_mode) {
			case CVM_FullMetaHash:
				seg_idx_color = colorhashfun(outmetarowptr[x].data(), sizeof(perdraw_metadata_type), mapp->viz_seg_colorization_seed);
				break;
			case CVM_ShaderIDs:
				perdraw_metadata_type copyofmeta = outmetarowptr[x];
				copyofmeta[0] = 0;
				seg_idx_color = colorhashfun(copyofmeta.data(), sizeof(perdraw_metadata_type), mapp->viz_seg_colorization_seed);
				break;
			case CVM_DrawInstance:
				objbuf[0] = inrowptr[x * 4u + 0u]; // draw call number
				objbuf[1] = inrowptr[x * 4u + 1u]; // instanced number
				seg_idx_color = colorhashfun(objbuf.data(), sizeof(objbuf), mapp->viz_seg_colorization_seed);
				break;
			case CVM_PrimitiveHash:
				memcpy(primbuf, outmetarowptr[x].data(), sizeof(perdraw_metadata_type));
				*reinterpret_cast<uint32_t*>(primbuf+sizeof(perdraw_metadata_type)) = inrowptr[x*4u+2u];
				seg_idx_color = colorhashfun(primbuf, sizeof(perdraw_metadata_type) + 4u, mapp->viz_seg_colorization_seed);
				break;
			case CVM_BufChannel0: case CVM_BufChannel1: case CVM_BufChannel2:
				seg_idx_color = colorhashfun(inrowptr + (x * 4u + mapp->viz_seg_colorization_mode), 4ull, mapp->viz_seg_colorization_seed);
				break;
			}
			*(mapp->viz_seg_colorized_for_display.entryptr<uint32_t>(rowidx, x)) = seg_idx_color;
		}
	}
}


static constexpr char technique_file[] = "segmentation_visualization.fx";

static effect_texture_variable check_for_effect_tex(effect_runtime* runtime, device* device)
{
	{
		effect_technique efftech = runtime->find_technique(technique_file, "SemSegView");
		if (efftech.handle == 0ull) return { 0ull };
		if (!runtime->get_technique_state(efftech)) return { 0ull }; // returns false if shader technique is disabled
	}
	effect_texture_variable efftexvar = runtime->find_texture_variable(technique_file, "SemSegTex");
	if (efftexvar.handle == 0ull) return { 0ull };
	{
		resource_view texrsv;
		runtime->get_texture_binding(efftexvar, &texrsv);
		if (texrsv.handle == 0ull) return { 0ull }; // this can happen if the shader is disabled or hasn't been enabled yet
		resource texrsc = device->get_resource_from_view(texrsv);
		if (texrsc.handle == 0ull) return { 0ull }; // this happens every frame that the shader is disabled (it's not a problem)
	}
	return efftexvar;
}


void imgui_draw_custom_shader_debug_viz_in_reshade_overlay(effect_runtime* runtime)
{
	device* const device = runtime->get_device();

#ifndef RENDERDOC_FOR_SHADERS
	ImGui::Text("Not compiled with semantic segmentation support!");
	return;
#endif

	// don't show any of this imgui stuff if the debug shader isn't enabled
	if (check_for_effect_tex(runtime, device).handle == 0ull) {
		ImGui::Text(std::string(std::string("Live semantic segmentation visualization can be enabled by ") + std::string(technique_file)).c_str());
		return;
	}

	auto& mapp = device->get_private_data<segmentation_app_data>();
	ImGui::Text("Semantic segmentation visualization debug tool");
	ImGui::DragInt("Color seed", &mapp.viz_seg_colorization_seed);
	for (uint32_t vm = 0; vm < CVM_number_of_modes; ++vm) {
		if (ImGui::RadioButton(ColorizationVizModeNames[vm], vm == mapp.viz_seg_colorization_mode)) {
			mapp.viz_seg_colorization_mode = static_cast<ColorizationVizMode>(vm);
		}
	}
	if (mapp.draw_metadata_seg_image.width > 0 && mapp.draw_metadata_seg_image.height > 0) {
		const ImVec2 mousep = ImGui::GetMousePos();
		const size_t mouse_x = std::min(static_cast<size_t>(std::max(std::llround(mousep.x), 0ll)), std::max(1ull, mapp.draw_metadata_seg_image.width) - 1ull);
		const size_t mouse_y = std::min(static_cast<size_t>(std::max(std::llround(mousep.y), 0ll)), std::max(1ull, mapp.draw_metadata_seg_image.height) - 1ull);
		const perdraw_metadata_type& meta = mapp.draw_metadata_seg_image.centryptr(mouse_y, mouse_x)[0];
		ImGui::Text("drawmeta(%4llu, %4llu) == (%6llu, 0x%016llx, 0x%016llx)", mouse_x, mouse_y, meta[0], meta[1], meta[2]);
	}
}


static resource_desc create_or_resize_intermediate_resource_copydest(device* device, segmentation_app_data& mapp) {
	resource_desc tdesc;
	if (!mapp.r_accum_bonus.is_valid() || mapp.r_accum_bonus.rsc.handle == 0ull) return tdesc;
	tdesc = device->get_resource_desc(mapp.r_accum_bonus.rsc);
	const bool tdesc_shape_same = (mapp.viz_seg_colorized_for_display.width == tdesc.texture.width && mapp.viz_seg_colorized_for_display.height == tdesc.texture.height);
	if (mapp.viz_intmdt_resource_copydest.handle == 0ull || !tdesc_shape_same) {
		if (!tdesc_shape_same) {
			device->destroy_resource(mapp.viz_intmdt_resource_copydest);
			mapp.viz_intmdt_resource_copydest.handle = 0ull;
		}
		if (device->create_resource(resource_desc(tdesc.texture.width, tdesc.texture.height, 1, 1,
			format::r32g32b32a32_uint, 1, memory_heap::gpu_to_cpu, resource_usage::copy_dest),
			nullptr, resource_usage::copy_dest, &mapp.viz_intmdt_resource_copydest)) {
			mapp.viz_seg_colorized_for_display.init_full(tdesc.texture.width, tdesc.texture.height, BUF_PIX_FMT_RGBA);
			mapp.draw_metadata_seg_image.init(tdesc.texture.width, tdesc.texture.height);
			mapp.row_colorization_thread_const_rowidxs_bulk.resize(tdesc.texture.height);
			for (uint32_t y = 0; y < tdesc.texture.height; ++y) mapp.row_colorization_thread_const_rowidxs_bulk[y] = y;
		}
		else {
			reshade::log_message(reshade::log_level::warning, "failed to create viz_intmdt_resource_copydest");
			mapp.viz_intmdt_resource_copydest.handle = 0ull;
		}
	}
	return tdesc;
}


void custom_shader_buffer_visualization_on_reshade_begin_effects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view)
{
	device* const device = runtime->get_device();
	auto& mapp = device->get_private_data<segmentation_app_data>();
	resource_desc tdesc = create_or_resize_intermediate_resource_copydest(device, mapp);
	if (mapp.viz_intmdt_resource_copydest.handle == 0ull || !mapp.do_intercept_draw || !mapp.r_accum_bonus.is_valid() || mapp.r_accum_bonus.rsc.handle == 0ull) return;

	// check for reshade shader tex handle
	effect_texture_variable efftexvar = check_for_effect_tex(runtime, device);
	if (efftexvar.handle == 0ull) return;

	// ready to copy to intermediate
	command_queue* const cmdqueue = runtime->get_command_queue();
	if (cmdqueue == nullptr) return;
	command_list* const icmdlst = cmdqueue->get_immediate_command_list();
	if (icmdlst == nullptr) return;

	icmdlst->barrier(mapp.r_accum_bonus.rsc, resource_usage::render_target, resource_usage::copy_source);
	icmdlst->copy_texture_region(mapp.r_accum_bonus.rsc, 0, nullptr, mapp.viz_intmdt_resource_copydest, 0, nullptr);
	icmdlst->barrier(mapp.r_accum_bonus.rsc, resource_usage::copy_source,   resource_usage::render_target);
	cmdqueue->wait_idle();

	// map intermediate to cpu and start processing (indexing colors)
	subresource_data viz_intmdt_mapped_data;
	if (device->map_texture_region(mapp.viz_intmdt_resource_copydest, 0, nullptr, map_access::read_only, &viz_intmdt_mapped_data)) {
		const auto draw_metadata = mapp.r_counter_buf.get_copy_of_frame_perdraw_metadata<perdraw_metadata_type>();
		if (draw_metadata.empty()) {
			memset(mapp.viz_seg_colorized_for_display.bytes.data(), 0, mapp.viz_seg_colorized_for_display.num_total_bytes());
		} else {
			// multithreaded processing
			moodycamel::ConcurrentQueue<uint32_t> rowqueue;
			rowqueue.enqueue_bulk(mapp.row_colorization_thread_const_rowidxs_bulk.data(), mapp.row_colorization_thread_const_rowidxs_bulk.size());
			for (uint32_t y = 0; y < mapp.row_colorization_threads.size(); ++y) {
				mapp.row_colorization_threads[y] = std::thread(multithreaded_row_colorization_worker, &rowqueue,
					static_cast<const uint8_t*>(viz_intmdt_mapped_data.data), viz_intmdt_mapped_data.row_pitch, tdesc.texture.width,
					&draw_metadata, &mapp);
			}
			for (uint32_t y = 0; y < mapp.row_colorization_threads.size(); ++y)
				mapp.row_colorization_threads[y].join();
			runtime->update_texture(efftexvar, tdesc.texture.width, tdesc.texture.height, mapp.viz_seg_colorized_for_display.bytes.data());
		}
		device->unmap_texture_region(mapp.viz_intmdt_resource_copydest, 0);
	}
}


typedef std::array<uint64_t, std::tuple_size<perdraw_metadata_type>::value + 2u> TriBuf; // like "perdraw_metadata_type" but with 2 more values: DrawInstID, PrimitiveID

bool segmentation_app_data::copy_and_index_seg_tex_needing_resource_barrier_into_packedbuf_and_metajson(
	reshade::api::command_queue* cmdqueue, simple_packed_buf& segBuf, simple_packed_buf& triBuf, nlohmann::json& dstMetaJson)
{
	if (cmdqueue == nullptr) return false;
	command_list* const icmdlst = cmdqueue->get_immediate_command_list();
	if (icmdlst == nullptr) return false;
	device* const device = cmdqueue->get_device();
	if (device == nullptr) return false;

	resource_desc tdesc = create_or_resize_intermediate_resource_copydest(device, *this);
	if (viz_intmdt_resource_copydest.handle == 0ull || !do_intercept_draw || !r_accum_bonus.is_valid() || r_accum_bonus.rsc.handle == 0ull) return false;
	if (tdesc.texture.width <= 1 || tdesc.texture.height <= 1) return false;

	icmdlst->barrier(r_accum_bonus.rsc, resource_usage::render_target, resource_usage::copy_source);
	icmdlst->copy_texture_region(r_accum_bonus.rsc, 0, nullptr, viz_intmdt_resource_copydest, 0, nullptr);
	icmdlst->barrier(r_accum_bonus.rsc, resource_usage::copy_source,   resource_usage::render_target);
	cmdqueue->wait_idle();

	// map intermediate to cpu and start processing (indexing colors)
	subresource_data intmdt_mapped_data;
	if (!device->map_texture_region(viz_intmdt_resource_copydest, 0, nullptr, map_access::read_only, &intmdt_mapped_data))
		return false;

	// We will write to a color-indexed lossless PNG file.
	// The color index (mapping from RGB to actual metadata) will be saved as a json.
	segBuf.init_full(tdesc.texture.width, tdesc.texture.height, BUF_PIX_FMT_RGB24);
	triBuf.init_full(tdesc.texture.width, tdesc.texture.height, BUF_PIX_FMT_RGB24);
	std::map<perdraw_metadata_type, uint32_t> seg2color;
	std::map<TriBuf, uint32_t> tri2color;
	std::map<DrawInstIDbuf, uint32_t> inst2objid;
	std::unordered_map<uint32_t, perdraw_metadata_type> color2seg;
	std::unordered_map<uint32_t, TriBuf> color2tri;
	uint32_t idx_color = 0u;
	uint8_t* const idx_color_bytes_view = reinterpret_cast<uint8_t*>(&idx_color);

	const auto draw_metadata = r_counter_buf.get_copy_of_frame_perdraw_metadata<perdraw_metadata_type>();
	if (draw_metadata.empty()) {
		memset(segBuf.bytes.data(), 0, segBuf.num_total_bytes());
		memset(triBuf.bytes.data(), 0, triBuf.num_total_bytes());
	} else {
		const size_t one_minus_draw_meta_size = draw_metadata.size() - 1ull;
		DrawInstIDbuf ibuf;
		TriBuf tbuf;
		for (uint32_t y = 0; y < tdesc.texture.height; ++y) {
			const uint32_t* rowptr = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(intmdt_mapped_data.data) + y * intmdt_mapped_data.row_pitch);
			for (uint32_t x = 0; x < tdesc.texture.width; ++x) {
				const auto& drawmeta = draw_metadata[std::min<size_t>(one_minus_draw_meta_size, rowptr[x * 4u])];
				// colorize seg
				if (auto mci = seg2color.find(drawmeta); mci != seg2color.end()) {
					idx_color = mci->second;
				} else {
					// Generate a new color as a 24-bit hash. We can't accept a hash collision, so repeatedly try with different seeds until we get a new unique color.
					uint32_t xseed = 0u;
					while (color2seg.count(idx_color = colorhashfun(drawmeta.data(), sizeof(perdraw_metadata_type), xseed)))
						xseed++;
					seg2color.emplace(drawmeta, idx_color);
					color2seg.emplace(idx_color, drawmeta);
				}
				uint8_t* optr = segBuf.entryptr<uint8_t>(y, x);
				optr[0] = idx_color_bytes_view[0];
				optr[1] = idx_color_bytes_view[1];
				optr[2] = idx_color_bytes_view[2];
				// Colorize detailed triangle+metadata map
				// Don't bother hashing for colors
				// InstanceID changes from frame to frame, so there is no expectation of continuity across frames
				memcpy(tbuf.data(), drawmeta.data(), sizeof(perdraw_metadata_type));
				ibuf[0] = rowptr[x * 4u]; // draw #
				ibuf[1] = rowptr[x * 4u + 1u]; // InstanceID
				if (auto ibo = inst2objid.find(ibuf); ibo != inst2objid.end()) {
					tbuf[std::tuple_size<perdraw_metadata_type>::value] = ibo->second;
				} else {
					inst2objid.emplace(ibuf, tbuf[std::tuple_size<perdraw_metadata_type>::value] = inst2objid.size());
				}
				tbuf[std::tuple_size<perdraw_metadata_type>::value + 1u] = rowptr[x * 4u + 2u]; // PrimitiveID
				if (auto mci = tri2color.find(tbuf); mci != tri2color.end()) {
					idx_color = mci->second;
				} else {
					idx_color = 0u;
					const auto newcolor = morton_halton_curve_rgb_3d(color2tri.size());
					idx_color_bytes_view[0] = newcolor[0];
					idx_color_bytes_view[1] = newcolor[1];
					idx_color_bytes_view[2] = newcolor[2];
					if (color2tri.count(idx_color)) reshade::log_message(reshade::log_level::error, "error: repeated color in colormap"); // TODO assert
					tri2color.emplace(tbuf, idx_color);
					color2tri.emplace(idx_color, tbuf);
				}
				optr = triBuf.entryptr<uint8_t>(y, x);
				optr[0] = idx_color_bytes_view[0];
				optr[1] = idx_color_bytes_view[1];
				optr[2] = idx_color_bytes_view[2];
			}
		}
	}
	device->unmap_texture_region(viz_intmdt_resource_copydest, 0);
	//dstMetaJson["seg_color2seg"] = color2seg;
	char sprbuf[16];
	for(auto cmi = color2seg.cbegin(); cmi != color2seg.cend(); ++cmi) {
		idx_color = cmi->first;
		sprintf_s(sprbuf, "%02x%02x%02x", idx_color_bytes_view[0], idx_color_bytes_view[1], idx_color_bytes_view[2]);
		dstMetaJson["seg_hexcolor2meta"][sprbuf] = cmi->second;
	}
	for(auto cmi = color2tri.cbegin(); cmi != color2tri.cend(); ++cmi) {
		idx_color = cmi->first;
		sprintf_s(sprbuf, "%02x%02x%02x", idx_color_bytes_view[0], idx_color_bytes_view[1], idx_color_bytes_view[2]);
		dstMetaJson["tri_hexcolor2meta"][sprbuf] = cmi->second;
	}
	return true;
}
