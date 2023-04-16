// Copyright (C) 2023 Jason Bunk
#include <imgui.h> // must be included before reshade.hpp
#include <reshade.hpp>
#include "gcv_reshade/generic_depth_struct.h"
#include "render_target_stats/clicked_rgb_rendertargets.hpp"
#include "rt_resource.hpp"
#include "segmentation_app_data.hpp"
#include "semseg_shader_register_bind.hpp"
#include "command_list_state.hpp"
#include "buffer_indexing_colorization.hpp"

static void on_device_init(reshade::api::device* device) {
	device->create_private_data<segmentation_app_data>();
}
static void on_device_destroy(reshade::api::device* device) {
	{
		auto& mapp = device->get_private_data<segmentation_app_data>();
		if (mapp.r_accum_bonus.isvalid) mapp.r_accum_bonus.delete_texture(device);
		if (mapp.r_counter_buf.isvalid) mapp.r_counter_buf.delete_resources(device);
	}
	device->destroy_private_data<segmentation_app_data>();
}
static void on_init_command_list(reshade::api::command_list* cmd_list) {
	cmd_list->create_private_data<segmentation_app_cmdlist_state>();
}
static void on_destroy_command_list(reshade::api::command_list* cmd_list) {
	cmd_list->destroy_private_data<segmentation_app_cmdlist_state>();
}
static void on_reset_cmdlist(reshade::api::command_list* cmd_list) {
	auto& state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	state.reset_cmdlist();
}

static void on_bind_pipeline(reshade::api::command_list* cmd_list, reshade::api::pipeline_stage stages, reshade::api::pipeline pipeline) {
	auto& state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	state.pipelines[stages] = pipeline;
}


static void on_bind_render_targets_and_depth_stencil(reshade::api::command_list* cmd_list, uint32_t count, const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
	auto& cmdlst_state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	cmdlst_state.reset_binding_of_bonus_rtv();
	cmdlst_state.rtvs.assign(rtvs, rtvs + count);
	cmdlst_state.dsv = dsv;
}

template<bool draw_is_indexed>
bool segmapp_on_draw_plain_or_indexed(reshade::api::command_list* cmd_list, uint32_t vertices_per_instance, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
	auto& cmdlst_state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	if (vertices_per_instance > 1 && !cmdlst_state.rtvs.empty() && cmdlst_state.rtvs[0].handle != 0ull && cmdlst_state.dsv.handle != 0ull) {
		reshade::api::device* const device = cmd_list->get_device();
		auto& mapp = device->get_private_data<segmentation_app_data>();
		if (mapp.do_intercept_draw && mapp.r_accum_bonus.isvalid && mapp.r_counter_buf.isvalid) {
			const reshade::api::resource currst0rsrc = device->get_resource_from_view(cmdlst_state.rtvs[0]);
			const reshade::api::resource currdepthrc = device->get_resource_from_view(cmdlst_state.dsv);
			if (mapp.rsrcs_of_presented_rgb.count(currst0rsrc.handle) && currdepthrc == mapp.rsrc_of_presented_depth) {
				// guess whether it is a fullscreen clear draw... this may miss some actual 3D objects with 6 vertices that appear once on screen
				const bool is_fullscreen_draw = (vertices_per_instance <= 6 && instance_count <= 1); //&& cmdlst_state.first_draw_since_bind);
				if (is_fullscreen_draw) {
					cmdlst_state.unbind_bonus_rtv(cmd_list);
				} else {
					if (cmdlst_state.bind_bonus_rtv_if_not_bound(cmd_list, mapp.r_accum_bonus.rtv)) {
						if (!mapp.r_accum_bonus.iscleared.exchange(1)) {
							cmd_list->clear_render_target_view(mapp.r_accum_bonus.rtv, mapp.r_accum_bonus.clear_color);
						}
					}
					reshade::api::resource_view bindme_bonusbufview = mapp.r_counter_buf.stash_metadata_and_get_view_for_draw(get_draw_metadata_including_shader_info(device, cmd_list, vertices_per_instance));
					uint64_t oldrsc = 0ull;
					if (pipeline_bind_bonus_tex_for_a_draw(device, cmd_list, mapp, bindme_bonusbufview, oldrsc)) {
						if (draw_is_indexed) cmd_list->draw_indexed(vertices_per_instance, instance_count, first_index, vertex_offset, first_instance);
						else cmd_list->draw(vertices_per_instance, instance_count, first_index, first_instance);
						uint64_t tmp;
						pipeline_bind_bonus_tex_for_a_draw(device, cmd_list, mapp, { oldrsc }, tmp);
						return true; // we just did our own draw, so skip what it would have done
					}
				}
			}
		}
	}
	//cmdlst_state.first_draw_since_bind = false;
	return false; // resume normal draw
}

static bool on_draw(reshade::api::command_list* cmd_list, uint32_t vertices, uint32_t instances, uint32_t first_vertex, uint32_t first_instance) {
	return segmapp_on_draw_plain_or_indexed<false>(cmd_list, vertices, instances, first_vertex, 0, first_instance);
}
// index_count == vertices == The number of indices read from the index buffer for each instance.
static bool on_draw_indexed(reshade::api::command_list* cmd_list, uint32_t vertices_per_instance, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
	return segmapp_on_draw_plain_or_indexed<true>(cmd_list, vertices_per_instance, instance_count, first_index, vertex_offset, first_instance);
}
// TODO
//static bool on_draw_or_dispatch_indirect(reshade::api::command_list* cmd_list, reshade::api::indirect_command type, reshade::api::resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride) {
//	return false;
//}

static bool set_or_resize_app_texture(segmentation_app_data& mapp, reshade::api::effect_runtime* runtime,
		reshade::api::device* device, std::unordered_set<uint64_t> texs_rgb_handles, reshade::api::resource tex_depth)
{
	if (texs_rgb_handles.empty() || tex_depth.handle == 0ull)
		return false;
	for (const auto& th : texs_rgb_handles)
		if (th == 0ull) return false;

	mapp.rsrcs_of_presented_rgb.clear();
	for (const auto& th : texs_rgb_handles)
		mapp.rsrcs_of_presented_rgb.insert(th);
	mapp.rsrc_of_presented_depth = tex_depth;

	reshade::api::resource_desc depthdesc = device->get_resource_desc(tex_depth);
	bool neednew = !mapp.r_accum_bonus.isvalid;
	if (!neednew) {
		reshade::api::resource_desc curdesc = device->get_resource_desc(mapp.r_accum_bonus.rsc);
		neednew = (depthdesc.texture.width != curdesc.texture.width || depthdesc.texture.height != curdesc.texture.height);
	}
	if (neednew) {
		reshade::log_message(reshade::log_level::info, "creating texture");
		if (mapp.r_accum_bonus.isvalid) {
			mapp.r_accum_bonus.delete_texture(device);
		}
		mapp.r_accum_bonus.create_texture(device, depthdesc.texture.width, depthdesc.texture.height,
				reshade::api::format::r32g32b32a32_uint, reshade::api::resource_usage::render_target);
	}
	return true;
}

bool segmentation_app_update_on_finish_effects(reshade::api::effect_runtime* runtime, bool requested_draw) {
	reshade::api::device* const device = runtime->get_device();
	auto& mapp = device->get_private_data<segmentation_app_data>();
	auto& clicked = device->get_private_data<clicked_rgb_rendertargets>();
	if (mapp.save_buf_tex_at_end_of_draw) {
		mapp.save_buf_tex_at_end_of_draw = false;
		return true;
	} else if (requested_draw) {
		mapp.r_accum_bonus.iscleared.store(0);
		reshade::log_message(reshade::log_level::info, "user pressed <CAPTURE>");
		if (clicked.clicked_resources.empty()) {
			reshade::log_message(reshade::log_level::info, "warning: cant capture because no RGB render target resources clicked!");
		} else {
			if (set_or_resize_app_texture(mapp, runtime, device, clicked.clicked_resources, runtime->get_private_data<generic_depth_data>().selected_depth_stencil)) {
				mapp.do_intercept_draw = true;
				mapp.save_buf_tex_at_end_of_draw = true;
			} else {
				reshade::log_message(reshade::log_level::info, "do_intercept_draw ignored due to failed texture init!");
			}
		}
	} else if (!clicked.clicked_resources.empty()) {
		// immediately initialize texture when ready: useful for displaying sem seg visualization
		mapp.r_accum_bonus.iscleared.store(0);
		if (set_or_resize_app_texture(mapp, runtime, device, clicked.clicked_resources, runtime->get_private_data<generic_depth_data>().selected_depth_stencil)) {
			mapp.do_intercept_draw = true;
		}
	}
	return false;
}

static void on_reshade_finish_effects(reshade::api::effect_runtime* runtime,
	reshade::api::command_list* cmd_list, reshade::api::resource_view rtv, reshade::api::resource_view rtv_srgb)
{
	reshade::api::device* const device = runtime->get_device();
	auto& mapp = device->get_private_data<segmentation_app_data>();
	mapp.r_counter_buf.create_buffer(device);
	//mapp.r_counter_buf.reset_at_end_of_frame(); // moved to main app
}

void register_segmentation_app_hooks() {
	reshade::register_event<reshade::addon_event::init_device>(on_device_init);
	reshade::register_event<reshade::addon_event::destroy_device>(on_device_destroy);
	reshade::register_event<reshade::addon_event::init_command_list>(on_init_command_list);
	reshade::register_event<reshade::addon_event::destroy_command_list>(on_destroy_command_list);

	reshade::register_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
	reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);

	reshade::register_event<reshade::addon_event::create_pipeline>(on_create_pipeline_add_semseg);
	reshade::register_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline_register_semseg);

	reshade::register_event<reshade::addon_event::draw>(on_draw);
	reshade::register_event<reshade::addon_event::draw_indexed>(on_draw_indexed);

	reshade::register_event<reshade::addon_event::reshade_begin_effects>(custom_shader_buffer_visualization_on_reshade_begin_effects);
	reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
}

void unregister_segmentation_app_hooks() {
	reshade::unregister_event<reshade::addon_event::init_device>(on_device_init);
	reshade::unregister_event<reshade::addon_event::destroy_device>(on_device_destroy);
	reshade::unregister_event<reshade::addon_event::init_command_list>(on_init_command_list);
	reshade::unregister_event<reshade::addon_event::destroy_command_list>(on_destroy_command_list);

	reshade::unregister_event<reshade::addon_event::bind_pipeline>(on_bind_pipeline);
	reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);

	reshade::unregister_event<reshade::addon_event::create_pipeline>(on_create_pipeline_add_semseg);
	reshade::unregister_event<reshade::addon_event::init_pipeline>(on_after_create_pipeline_register_semseg);

	reshade::unregister_event<reshade::addon_event::draw>(on_draw);
	reshade::unregister_event<reshade::addon_event::draw_indexed>(on_draw_indexed);

	reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(custom_shader_buffer_visualization_on_reshade_begin_effects);
	reshade::unregister_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
}
