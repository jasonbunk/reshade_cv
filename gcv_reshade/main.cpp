// Copyright (C) 2022 Jason Bunk
#include <imgui.h>
#include "image_writer_thread_pool.h"
#include "generic_depth_struct.h"
#include "gcv_games/game_interface_factory.h"
#include "gcv_utils/miscutils.h"
#include <fstream>
typedef std::chrono::steady_clock hiresclock;

static void on_init(reshade::api::swapchain *swapchain)
{
	/*image_writer_thread_pool &data =*/ swapchain->create_private_data<image_writer_thread_pool>();
	reshade::log_message(3, std::string(std::string("tests: ")+run_utils_tests()).c_str());
}
static void on_destroy(reshade::api::swapchain *swapchain)
{
	image_writer_thread_pool &shdata = swapchain->create_private_data<image_writer_thread_pool>();
	shdata.change_num_threads(0);
	shdata.print_waiting_log_messages();
	swapchain->destroy_private_data<image_writer_thread_pool>();
}

static void on_reshade_finish_effects(reshade::api::effect_runtime *runtime,
	reshade::api::command_list *, reshade::api::resource_view rtv, reshade::api::resource_view)
{
	image_writer_thread_pool &shdata = runtime->get_private_data<image_writer_thread_pool>();
	CamMatrixData gamecam;
	std::string errstr;
	bool shaderupdatedwithcampos = false;
	float shadercamposbuf[4];

	if (runtime->is_key_pressed(VK_F11))
	{
		generic_depth_data &genericdepdata = runtime->get_private_data<generic_depth_data>();
		reshade::api::device *const device = runtime->get_device();
		reshade::api::command_queue *cmdqueue = runtime->get_command_queue();
		const std::string milliselps = std::to_string(
			std::chrono::duration_cast<std::chrono::milliseconds>(hiresclock::now() - shdata.init_time).count());
		const std::string basefilen = shdata.gamename_simpler() + std::string("_")
			+ get_datestr_yyyy_mm_dd() + std::string("_") + milliselps + std::string("_");

		if (shdata.save_texture_image_needing_resource_barrier_copy(basefilen + std::string("RGB"),
			ImageWriter_STB_png, cmdqueue, device->get_resource_from_view(rtv), false))
		{
			if (shdata.save_texture_image_needing_resource_barrier_copy(basefilen + std::string("depth"),
				ImageWriter_STB_png | ImageWriter_numpy | (shdata.game_knows_depthbuffer() ? ImageWriter_fpzip : 0), cmdqueue, genericdepdata.selected_depth_stencil, true))
			{
				//const char *sercamdata = shdata.get_serialized_camera_data(errstr);
				if(shdata.get_camera_matrix(gamecam, errstr)) {
					std::string camstr = gamecam.serialize_for_json(false);
					if (!camstr.empty()) {
						std::ofstream outjson(shdata.output_filepath_creates_outdir_if_needed(basefilen + std::string("camera.json")));
						if (outjson.is_open() && outjson.good()) {
							outjson << "{\"time_ms\":" << milliselps << "," << camstr << "}" << std::endl;
							outjson.close();
							reshade::log_message(3, std::string(
								std::string("Captured RGB, depth buffers, and camera data at time ") + milliselps).c_str());
						} else {
							reshade::log_message(2, std::string(
								std::string("Captured RGB and depth buffers at time ") + milliselps
								+ std::string(", but failed to write camera data to json")).c_str());
						}
					} else {
						reshade::log_message(2, std::string(
							std::string("Captured RGB and depth buffers at time ") + milliselps
							+ std::string(", but failed to serialize camera data: ") + errstr).c_str());
					}
				} else {
					reshade::log_message(2, std::string(
						std::string("Captured RGB and depth buffers at time ")+milliselps
						+std::string(", but failed to get any camera data: ")+ errstr).c_str());
				}
			} else {
				reshade::log_message(1, "Failed to capture depth buffer...");
			}
		} else {
			reshade::log_message(1, "Failed to capture RGB display buffer...");
		}
		errstr.clear();
	}

	if(shdata.grabcamcoords) {
		if (gamecam.extrinsic_status == CamMatrix_Uninitialized) {
			shdata.get_camera_matrix(gamecam, errstr);
		}
		if (gamecam.extrinsic_status != CamMatrix_Uninitialized) {
			shadercamposbuf[3] = 1.0f;
			if (gamecam.extrinsic_status & CamMatrix_PositionGood || gamecam.extrinsic_status & CamMatrix_WIP) {
				shadercamposbuf[0] = gamecam.extrinsic_cam2world.GetPosition().x;
				shadercamposbuf[1] = gamecam.extrinsic_cam2world.GetPosition().y;
				shadercamposbuf[2] = gamecam.extrinsic_cam2world.GetPosition().z;
				runtime->set_uniform_value_float(runtime->find_uniform_variable("displaycamcoords.fx", "dispcam_latestcampos"), shadercamposbuf, 4);
				shaderupdatedwithcampos = true;
			}
			if (gamecam.extrinsic_status & CamMatrix_RotationGood || gamecam.extrinsic_status & CamMatrix_WIP) {
				for (int colidx = 0; colidx < 3; ++colidx) {
					shadercamposbuf[0] = gamecam.extrinsic_cam2world.GetCol(colidx).x;
					shadercamposbuf[1] = gamecam.extrinsic_cam2world.GetCol(colidx).y;
					shadercamposbuf[2] = gamecam.extrinsic_cam2world.GetCol(colidx).z;
					runtime->set_uniform_value_float(runtime->find_uniform_variable("displaycamcoords.fx", (std::string("dispcam_latestcamcol")+std::to_string(colidx)).c_str()), shadercamposbuf, 4);
				}
				shaderupdatedwithcampos = true;
			}
		}
	}
	if (!shdata.camcoordsinitialized) {
		if (!shaderupdatedwithcampos) {
			shadercamposbuf[0] = 0.0f;
			shadercamposbuf[1] = 0.0f;
			shadercamposbuf[2] = 0.0f;
			shadercamposbuf[3] = 0.0f;
			runtime->set_uniform_value_float(runtime->find_uniform_variable("displaycamcoords.fx", "dispcam_latestcampos"), shadercamposbuf, 4);
		}
		shdata.camcoordsinitialized = true;
	}
	shdata.print_waiting_log_messages();
}

static void draw_settings_overlay(reshade::api::effect_runtime *runtime)
{
	image_writer_thread_pool &shdata = runtime->get_private_data<image_writer_thread_pool>();
	ImGui::Checkbox("Depth map: debug mode", &shdata.depth_settings.debug_mode);
	ImGui::Checkbox("Depth map: verbose mode", &shdata.depth_settings.more_verbose);
	ImGui::Checkbox("Depth map: already float?", &shdata.depth_settings.alreadyfloat);
	ImGui::Checkbox("Depth map: float endian flip?", &shdata.depth_settings.float_reverse_endian);
	ImGui::Checkbox("Grab camera coordinates every frame?", &shdata.grabcamcoords);
	ImGui::SliderInt("Depth map: row pitch rescale (powers of 2)", &shdata.depth_settings.adjustpitchhack, -8, 8);
	ImGui::SliderInt("Depth map: bytes per pix", &shdata.depth_settings.depthbytes, 0, 8);
	ImGui::SliderInt("Depth map: bytes per pix to keep", &shdata.depth_settings.depthbyteskeep, 0, 8);
	if (shdata.grabcamcoords) {
		CamMatrixData lcam; std::string errstr;
		if (shdata.get_camera_matrix(lcam, errstr) != CamMatrix_Uninitialized) {
			ImGui::Text("%f, %f, %f", lcam.extrinsic_cam2world.GetPosition().x, lcam.extrinsic_cam2world.GetPosition().y, lcam.extrinsic_cam2world.GetPosition().z);
		} else {
			ImGui::Text(errstr.c_str());
		}
	}
}

extern "C" __declspec(dllexport) const char *NAME = "CV Capture";
extern "C" __declspec(dllexport) const char *DESCRIPTION =
    "Add-on that captures the screen after effects were rendered, and also the depth buffer, every time key is pressed.";

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		if (!reshade::register_addon(hinstDLL))
			return FALSE;
		reshade::register_event<reshade::addon_event::init_swapchain>(on_init);
		reshade::register_event<reshade::addon_event::destroy_swapchain>(on_destroy);
		reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
		reshade::register_overlay(nullptr, draw_settings_overlay);
		break;
	case DLL_PROCESS_DETACH:
		reshade::unregister_addon(hinstDLL);
		break;
	}
	return TRUE;
}
