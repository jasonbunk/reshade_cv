// Copyright (C) 2022 Jason Bunk
#include <imgui.h>
#include "image_writer_thread_pool.h"
#include "generic_depth_struct.h"
#include "gcv_games/game_interface_factory.h"
#include "gcv_utils/miscutils.h"
#include <fstream>
typedef std::chrono::steady_clock hiresclock;

static void on_init(reshade::api::device* device)
{
	image_writer_thread_pool &shdata = device->create_private_data<image_writer_thread_pool>();
	reshade::log_message(3, std::string(std::string("tests: ")+run_utils_tests()).c_str());
	shdata.init_time = hiresclock::now();
}
static void on_destroy(reshade::api::device* device)
{
	device->get_private_data<image_writer_thread_pool>().change_num_threads(0);
	device->get_private_data<image_writer_thread_pool>().print_waiting_log_messages();
	device->destroy_private_data<image_writer_thread_pool>();
}

static void on_reshade_finish_effects(reshade::api::effect_runtime *runtime,
	reshade::api::command_list *, reshade::api::resource_view rtv, reshade::api::resource_view)
{
	image_writer_thread_pool &shdata = runtime->get_device()->get_private_data<image_writer_thread_pool>();
	CamMatrixData gamecam;
	std::string errstr;
	bool shaderupdatedwithcampos = false;
	float shadercamposbuf[4];

	if (runtime->is_key_pressed(VK_F11))
	{
		generic_depth_data &genericdepdata = runtime->get_private_data<generic_depth_data>();
		reshade::api::device *const device = runtime->get_device();
		reshade::api::command_queue *cmdqueue = runtime->get_command_queue();
		const int64_t microseconds_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(hiresclock::now() - shdata.init_time).count();
		const std::string microelapsedstr = std::to_string(microseconds_elapsed);
		const std::string basefilen = shdata.gamename_simpler() + std::string("_")
			+ get_datestr_yyyy_mm_dd() + std::string("_") + microelapsedstr + std::string("_");
		std::stringstream capmessage;
		capmessage << "capture " << basefilen << ": ";
		bool capgood = true;

		if (shdata.get_camera_matrix(gamecam, errstr, microseconds_elapsed)) {
			std::ofstream outjson(shdata.output_filepath_creates_outdir_if_needed(basefilen + std::string("camera.json")));
			if (outjson.is_open() && outjson.good()) {
				nlohmann::json camj = gamecam.as_json();
				camj["time_us"] = microelapsedstr;
				outjson << camj.dump() << std::endl;
				outjson.close();
				capmessage << "camjson: good";
			} else {
				capmessage << "camjson: failed to write";
				capgood = false;
			}
		} else {
			capmessage << "camjson: failed to get any camera data";
			capgood = false;
		}
		if (!errstr.empty()) {
			capmessage << ", " << errstr;
			errstr.clear();
		}
		capmessage << "; ";

		if (shdata.save_texture_image_needing_resource_barrier_copy(basefilen + std::string("RGB"),
			ImageWriter_STB_png, cmdqueue, device->get_resource_from_view(rtv), false))
		{
			if (shdata.save_texture_image_needing_resource_barrier_copy(basefilen + std::string("depth"),
				ImageWriter_STB_png | ImageWriter_numpy | (shdata.game_knows_depthbuffer() ? ImageWriter_fpzip : 0),
				cmdqueue, genericdepdata.selected_depth_stencil, true))
			{
				capmessage << "RGB and depth good";
			} else {
				capmessage << "RGB good, but failed to capture depth";
				capgood = false;
			}
		} else {
			capmessage << "failed to capture RGB (so didnt try depth)";
			capgood = false;
		}
		if (!errstr.empty()) {
			capmessage << ", " << errstr;
			errstr.clear();
		}
		reshade::log_message(capgood ? 3 : 1, capmessage.str().c_str());
	}

	if(shdata.grabcamcoords) {
		if (gamecam.extrinsic_status == CamMatrix_Uninitialized) {
			shdata.get_camera_matrix(gamecam, errstr);
		}
		if (gamecam.extrinsic_status != CamMatrix_Uninitialized) {
			shadercamposbuf[3] = 1.0f;
			if (gamecam.extrinsic_status & CamMatrix_PositionGood || gamecam.extrinsic_status & CamMatrix_WIP) {
				for(int ii=0; ii<3; ++ii) shadercamposbuf[ii] = gamecam.extrinsic_cam2world(ii, cam_matrix_position_column);
				runtime->set_uniform_value_float(runtime->find_uniform_variable("displaycamcoords.fx", "dispcam_latestcampos"), shadercamposbuf, 4);
				shaderupdatedwithcampos = true;
			}
			if (gamecam.extrinsic_status & CamMatrix_RotationGood || gamecam.extrinsic_status & CamMatrix_WIP) {
				for (int colidx = 0; colidx < 3; ++colidx) {
					for (int ii = 0; ii < 3; ++ii) shadercamposbuf[ii] = gamecam.extrinsic_cam2world(ii, colidx);
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
	image_writer_thread_pool &shdata = runtime->get_device()->get_private_data<image_writer_thread_pool>();
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
			ImGui::Text("%f, %f, %f",
				lcam.extrinsic_cam2world(0, cam_matrix_position_column),
				lcam.extrinsic_cam2world(1, cam_matrix_position_column),
				lcam.extrinsic_cam2world(2, cam_matrix_position_column));
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
		reshade::register_event<reshade::addon_event::init_device>(on_init);
		reshade::register_event<reshade::addon_event::destroy_device>(on_destroy);
		reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
		reshade::register_overlay(nullptr, draw_settings_overlay);
		break;
	case DLL_PROCESS_DETACH:
		reshade::unregister_addon(hinstDLL);
		break;
	}
	return TRUE;
}
