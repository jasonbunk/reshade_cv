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
	uint8_t camstatus = CamMatrix_Uninitialized;
	CamMatrix gamecam;
	std::string errstr;

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
				ImageWriter_STB_png | ImageWriter_numpy | ImageWriter_fpzip, cmdqueue, genericdepdata.selected_depth_stencil, true))
			{
				camstatus = shdata.get_camera_matrix(gamecam, errstr);
				//const char *sercamdata = shdata.get_serialized_camera_data(errstr);
				if(camstatus != CamMatrix_Uninitialized) {
					std::string camstr = gamecam.serialize();
					if (!camstr.empty()) {
						std::ofstream outjson(shdata.output_filepath_creates_outdir_if_needed(basefilen + std::string("camera.json")));
						if (outjson.is_open() && outjson.good()) {
							outjson << "{\"time_ms\":" << milliselps << ",\"camdata\":" << camstr << "}" << std::endl;
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
	}
	shdata.print_waiting_log_messages();

	if(shdata.grabcamcoords) {
		if (camstatus == CamMatrix_Uninitialized) {
			camstatus = shdata.get_camera_matrix(gamecam, errstr);
		}
		if (camstatus != CamMatrix_Uninitialized) {
			float gotcampos[4];
			gotcampos[0] = gamecam.GetPosition().x;
			gotcampos[1] = gamecam.GetPosition().y;
			gotcampos[2] = gamecam.GetPosition().z;
			gotcampos[3] = 0.0f;
			runtime->set_uniform_value_float(runtime->find_uniform_variable("displaycamcoords.fx", "dispcam_latestcampos"), gotcampos, 3);
		}
	}
}

static void draw_settings_overlay(reshade::api::effect_runtime *runtime)
{
	image_writer_thread_pool &shdata = runtime->get_private_data<image_writer_thread_pool>();
	ImGui::Checkbox("Depth map: debug mode", &shdata.depth_settings.debug_mode);
	ImGui::Checkbox("Depth map: reciprocal?", &shdata.depth_settings.reciprocal);
	ImGui::Checkbox("Depth map: subtract int?", &shdata.depth_settings.subtractfrom);
	ImGui::Checkbox("Depth map: already float?", &shdata.depth_settings.alreadyfloat);
	ImGui::Checkbox("Depth map: float endian flip?", &shdata.depth_settings.float_reverse_endian);
	ImGui::Checkbox("Grab camera coordinates every frame?", &shdata.grabcamcoords);
	ImGui::SliderInt("Depth map: row pitch rescale (powers of 2)", &shdata.depth_settings.adjustpitchhack, -8, 8);
	ImGui::SliderInt("Depth map: bytes per pix", &shdata.depth_settings.depthbytes, 0, 8);
	ImGui::SliderInt("Depth map: bytes per pix to keep", &shdata.depth_settings.depthbyteskeep, 0, 8);
	ImGui::SliderInt("Depth map: normalization (power of 2)", &shdata.depth_settings.depthnormalization, 0, 64);
	if (ImGui::Button("log all shader uniform variables")) {
		runtime->enumerate_uniform_variables(nullptr, [](reshade::api::effect_runtime *runtime, auto variable) {
			char source[256] = ""; memset(source, 0, 256); size_t buflen = 256;
			runtime->get_uniform_variable_name(variable, source, &buflen);
			std::string varname(source);
			//get_uniform_variable_type(effect_uniform_variable variable, format *out_base_type, uint32_t *out_rows = nullptr, uint32_t *out_columns = nullptr, uint32_t *out_array_length = nullptr)
			reshade::api::format fndtype; uint32_t fndrows = 0; uint32_t fndcols = 0; uint32_t fndarlen = 0;
			runtime->get_uniform_variable_type(variable, &fndtype, &fndrows, &fndcols, &fndarlen);
			reshade::log_message(3, std::string(varname+std::string(" of type ")+std::to_string(static_cast<int64_t>(fndtype))+std::string(" of shape ")+std::to_string(fndcols)+std::string(" x ")+std::to_string(fndrows)+std::string(" of len ")+std::to_string(fndarlen)).c_str());
		});
	}
	if (shdata.grabcamcoords) {
		CamMatrix lcam; std::string errstr;
		if (shdata.get_camera_matrix(lcam, errstr) != CamMatrix_Uninitialized) {
			ImGui::InputInt("Cam scanner mem start", &shdata.imguicamscanslider);
			if (ImGui::Button("Start cam scan")) {
				shdata.imguicamscandesired = static_cast<int64_t>(shdata.imguicamscanslider);
			}
			ImGui::Text("%f, %f, %f", lcam.GetPosition().x, lcam.GetPosition().y, lcam.GetPosition().z);
		}
		else {
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
