// Copyright (C) 2023 Jason Bunk
// This is derived from a simplified version of the generic_depth example provided by reshade.
#include <imgui.h>
#include <reshade.hpp>
#include "reshade_tex_format_info.hpp"
#include "render_target_stats_tracking.hpp"
#include "clicked_rgb_rendertargets.hpp"
using namespace reshade::api;

static void on_device_init(device* device) {
	device->create_private_data<clicked_rgb_rendertargets>();
	device->create_private_data<device_draw_stats>();
}
static void on_device_destroy(device* device) {
	device->destroy_private_data<clicked_rgb_rendertargets>();
	device->destroy_private_data<device_draw_stats>();
}
static void on_init_command_list(command_list* cmd_list) {
	cmd_list->create_private_data<cmdlist_draw_stats>();
}
static void on_destroy_command_list(command_list* cmd_list) {
	cmd_list->destroy_private_data<cmdlist_draw_stats>();
}
static void on_init_command_queue(command_queue* cmd_queue) {
	cmd_queue->create_private_data<queue_draw_stats>();
	if ((cmd_queue->get_type() & command_queue_type::graphics) == 0)
		return;
	auto& device_data = cmd_queue->get_device()->get_private_data<device_draw_stats>();
	device_data.queues.push_back(cmd_queue);
}
static void on_destroy_command_queue(command_queue* cmd_queue) {
	cmd_queue->destroy_private_data<queue_draw_stats>();
	auto& device_data = cmd_queue->get_device()->get_private_data<device_draw_stats>();
	if(std::find(device_data.queues.begin(), device_data.queues.end(), cmd_queue) != device_data.queues.end())
		device_data.queues.erase(std::remove(device_data.queues.begin(), device_data.queues.end(), cmd_queue), device_data.queues.end());
}
static void on_reset_cmdlist(command_list *cmd_list) {
	auto &state = cmd_list->get_private_data<cmdlist_draw_stats>();
	state.reset_stats();
	state.reset_cmdlist();
}
static void on_execute_primary(command_queue *queue, command_list *cmd_list) {
	if(queue != nullptr && cmd_list != nullptr) {
		auto& target_state = queue->get_private_data<queue_draw_stats>();
		auto& source_state = cmd_list->get_private_data<cmdlist_draw_stats>();
		target_state.merge(source_state);
	}
}
static void on_execute_secondary(command_list *cmd_list, command_list *secondary_cmd_list) {
	if (cmd_list != nullptr && secondary_cmd_list != nullptr) {
		auto& target_state = cmd_list->get_private_data<cmdlist_draw_stats>();
		auto& source_state = secondary_cmd_list->get_private_data<cmdlist_draw_stats>();
		target_state.merge(source_state);
	}
}

static void on_bind_render_targets_and_depth_stencil(command_list* cmd_list, uint32_t count, const resource_view* rtvs, resource_view dsv) {
	auto &state = cmd_list->get_private_data<cmdlist_draw_stats>();
	state.render_targets.assign(rtvs, rtvs + count);
	state.depth_stencil = dsv;
}

template<bool is_direct>
bool rstats_on_draw_plain_or_indexed(command_list* cmd_list, uint32_t vertices_per_instance, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
	device* const device = cmd_list->get_device();
	const auto& mapp = device->get_private_data<device_draw_stats>();
	if (mapp.render_height > 0 && mapp.render_width > 0) {
		auto& cmd_stat = cmd_list->get_private_data<cmdlist_draw_stats>();
		if (!cmd_stat.render_targets.empty() && cmd_stat.render_targets[0].handle != 0ull) {
			const resource rhndl = device->get_resource_from_view(cmd_stat.render_targets[0]);
			if (rhndl.handle != 0ull) {
				const resource_desc rdesc = device->get_resource_desc(rhndl);
				if (rdesc.texture.width == mapp.render_width && rdesc.texture.height == mapp.render_height) {
					std::lock_guard<std::mutex> guard(cmd_stat.statsmut);
					rsc_stats& rst = cmd_stat.resource2stats[rhndl.handle];
					if (rst.num_command_lists_which_touched == 0) {
						rst.num_command_lists_which_touched = 1;
					}
					rst.total_vertices += vertices_per_instance * instance_count;
					rst.max_num_rtvsbound = std::max<uint64_t>(rst.max_num_rtvsbound, cmd_stat.render_targets.size());
					if (is_direct) {
						rst.total_draws++;
					} else {
						rst.total_draws += instance_count;
						rst.num_indirect_draws += instance_count;
					}
				}
			}
		}
	}
	return false;
}
static bool on_draw(command_list* cmd_list, uint32_t vertices, uint32_t instances, uint32_t first_vertex, uint32_t first_instance) {
	return rstats_on_draw_plain_or_indexed<true>(cmd_list, vertices, instances, first_vertex, 0, first_instance);
}
static bool on_draw_indexed(command_list* cmd_list, uint32_t vertices_per_instance, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
	return rstats_on_draw_plain_or_indexed<true>(cmd_list, vertices_per_instance, instance_count, first_index, vertex_offset, first_instance);
}
static bool on_draw_or_dispatch_indirect(command_list* cmd_list, indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride) {
	if (type == indirect_command::dispatch) return false;
	return rstats_on_draw_plain_or_indexed<false>(cmd_list, 3, draw_count, 0, 0, 0);
}

static void on_present(command_queue*, swapchain* swapchain, const rect*, const rect*, uint32_t, const rect*) {
	device* const device = swapchain->get_device();
	auto& mapp = device->get_private_data<device_draw_stats>();
	mapp.zero_stats();
	// Merge state from all graphics queues
	for (command_queue *const queue : mapp.queues) {
		auto &state = queue->get_private_data<queue_draw_stats>();
		mapp.merge(state);
	}
}

static void on_reshade_finish_effects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb) {
	device* const device = runtime->get_device();
	auto& mapp = device->get_private_data<device_draw_stats>();
	runtime->get_screenshot_width_and_height(&mapp.render_width, &mapp.render_height);
	mapp.frameidx++;
	std::lock_guard<std::mutex> guard(mapp.statsmut);
	// Update tracking of clickable resources
	for (const auto& [rhndl, rstats] : mapp.resource2stats) {
		if(rstats.total_draws > 0)
			mapp.resource2lastseenframeidx[rhndl] = mapp.frameidx;
	}
	// Remove stale buffers that haven't been drawn to in a while
	for (auto mit = mapp.resource2lastseenframeidx.cbegin(); mit != mapp.resource2lastseenframeidx.cend(); ) {
		if (mapp.frameidx - mit->second > 9) {
			mapp.resource2stats.erase(mit->first);
			mit = mapp.resource2lastseenframeidx.erase(mit);
		} else {
			mit++;
		}
	}
	// Suggest a valid buffer (that wouldnt have been grayed out) with most draws
	uint64_t bestscore = 0;
	uint64_t bestresource = 0;
	for (const auto& [rhndl, rstats] : mapp.resource2stats) {
		if (rstats.total_draws > 0 && rstats.total_vertices > 0) {
			const resource_desc rdesc = device->get_resource_desc({ rhndl });
			if (fmtchannelsdisplaycolorlike.at(rdesc.texture.format)) {
				const uint64_t thisscore = rstats.total_draws * rstats.total_vertices;
				if (thisscore > bestscore) {
					bestresource = rhndl;
					bestscore = thisscore;
				}
			}
		}
	}
	mapp.suggested_resource_to_click = (bestscore > 0) ? bestresource : 0ull;
	// Assign the best resource if the user hasn't clicked on anything
	if (mapp.actually_clicked_resources.empty()) {
		auto& mclicks = device->get_private_data<clicked_rgb_rendertargets>();
		if (mapp.suggested_resource_to_click != 0ull)
			mclicks.clicked_resources = { mapp.suggested_resource_to_click };
		else
			mclicks.clicked_resources.clear();
	}
}

static thread_local char imguiprintbuf[128];

void imgui_draw_rgb_render_target_stats_in_reshade_overlay(effect_runtime* runtime)
{
	device* const device = runtime->get_device();
	auto& mapp = device->get_private_data<device_draw_stats>();
	std::lock_guard<std::mutex> guard(mapp.statsmut);
	bool clicked_something = false;
	for (const auto& [rhndl, rstats] : mapp.resource2stats) {
		bool greyedout;
		std::string texfmtname;
		if (rstats.total_draws > 0 && rstats.total_vertices > 0) {
			resource_desc rdesc = device->get_resource_desc({ rhndl });
			texfmtname = fmtnames.at(rdesc.texture.format);
			greyedout = !mapp.actually_clicked_resources.count(rhndl) && !fmtchannelsdisplaycolorlike.at(rdesc.texture.format);
		} else {
			texfmtname = "?"; // todo cache last known name of resource in a map
			greyedout = !mapp.actually_clicked_resources.count(rhndl);
		}
		if (greyedout) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
		}
		ImGui::Text(rhndl == mapp.suggested_resource_to_click ? ">" : " ");
		ImGui::SameLine();
		sprintf_s(imguiprintbuf, "0x%016llx | %-22s |", rhndl, texfmtname.c_str());
		const bool wasclicked = mapp.actually_clicked_resources.count(rhndl);
		bool isnowclicked = wasclicked;
		if (ImGui::Checkbox(imguiprintbuf, &isnowclicked)) {
			clicked_something = true;
			if(wasclicked)
				mapp.actually_clicked_resources.erase(rhndl);
			else
				mapp.actually_clicked_resources.insert(rhndl);
		}
		ImGui::SameLine();
		ImGui::Text("%5lld draw | %5lld indirect | %8lld vert | %lld rtvbnd | %2lld cmdlst",
			rstats.total_draws, rstats.num_indirect_draws, rstats.total_vertices, rstats.max_num_rtvsbound, rstats.num_command_lists_which_touched);
		if (greyedout) {
			ImGui::PopStyleColor();
		}
	}
	if (clicked_something) {
		auto& mclicks = device->get_private_data<clicked_rgb_rendertargets>();
		if (mapp.actually_clicked_resources.empty()) {
			if (mapp.suggested_resource_to_click != 0ull)
				mclicks.clicked_resources = { mapp.suggested_resource_to_click };
			else
				mclicks.clicked_resources.clear();
		} else {
			mclicks.clicked_resources = mapp.actually_clicked_resources;
		}
	}
}

void register_rgb_render_target_stats_tracking() {
	reshade::register_event<reshade::addon_event::init_device>(on_device_init);
	reshade::register_event<reshade::addon_event::destroy_device>(on_device_destroy);
	reshade::register_event<reshade::addon_event::init_command_list>(on_init_command_list);
	reshade::register_event<reshade::addon_event::destroy_command_list>(on_destroy_command_list);
	reshade::register_event<reshade::addon_event::init_command_queue>(on_init_command_queue);
	reshade::register_event<reshade::addon_event::destroy_command_queue>(on_destroy_command_queue);
	reshade::register_event<reshade::addon_event::reset_command_list>(on_reset_cmdlist);
	reshade::register_event<reshade::addon_event::execute_command_list>(on_execute_primary);
	reshade::register_event<reshade::addon_event::execute_secondary_command_list>(on_execute_secondary);

	reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);
	reshade::register_event<reshade::addon_event::draw>(on_draw);
	reshade::register_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
	reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_or_dispatch_indirect);
	reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
	reshade::register_event<reshade::addon_event::present>(on_present);

	//reshade::register_overlay(nullptr, imgui_draw_rgb_render_target_stats_in_reshade_overlay);
}

void unregister_rgb_render_target_stats_tracking() {
	reshade::unregister_event<reshade::addon_event::init_device>(on_device_init);
	reshade::unregister_event<reshade::addon_event::destroy_device>(on_device_destroy);
	reshade::unregister_event<reshade::addon_event::init_command_list>(on_init_command_list);
	reshade::unregister_event<reshade::addon_event::destroy_command_list>(on_destroy_command_list);
	reshade::unregister_event<reshade::addon_event::init_command_queue>(on_init_command_queue);
	reshade::unregister_event<reshade::addon_event::destroy_command_queue>(on_destroy_command_queue);
	reshade::unregister_event<reshade::addon_event::reset_command_list>(on_reset_cmdlist);
	reshade::unregister_event<reshade::addon_event::execute_command_list>(on_execute_primary);
	reshade::unregister_event<reshade::addon_event::execute_secondary_command_list>(on_execute_secondary);

	reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);
	reshade::unregister_event<reshade::addon_event::draw>(on_draw);
	reshade::unregister_event<reshade::addon_event::draw_indexed>(on_draw_indexed);
	reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(on_draw_or_dispatch_indirect);
	reshade::unregister_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
	reshade::unregister_event<reshade::addon_event::present>(on_present);
}
