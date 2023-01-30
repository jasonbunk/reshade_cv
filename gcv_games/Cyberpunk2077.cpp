// Copyright (C) 2022 Jason Bunk
#include "Cyberpunk2077.h"
#include "gcv_utils/depth_utils.h"
#include "gcv_utils/scripted_cam_buf_templates.h"

// For this game, scripting is made easy by Cyber Engine Tweaks,
// so I provide a simple lua script which stashes camera coordinates into a double[] buffer.

std::string GameCyberpunk2077::gamename_verbose() const { return "Cyberpunk2077"; } // hopefully continues to work with future patches via the mod lua

std::string GameCyberpunk2077::camera_dll_name() const { return ""; } // no dll name, it's available in the exe memory space
uint64_t GameCyberpunk2077::camera_dll_mem_start() const { return 0; }
GameCamDLLMatrixType GameCyberpunk2077::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf; }

scriptedcam_checkbuf_funptr GameCyberpunk2077::get_scriptedcambuf_checkfun() const {
	return template_check_scriptedcambuf_hash<double, 13, 1>;
}
uint64_t GameCyberpunk2077::get_scriptedcambuf_sizebytes() const {
	return template_scriptedcambuf_sizebytes<double, 13, 1>();
}
bool GameCyberpunk2077::copy_scriptedcambuf_to_matrix(uint8_t* buf, uint64_t buflen, CamMatrixData& rcam, std::string& errstr) const {
	return template_copy_scriptedcambuf_extrinsic_cam2world_and_fov<double, 13, 1>(buf, buflen, rcam, true, errstr);
}

bool GameCyberpunk2077::can_interpret_depth_buffer() const {
	return true;
}
float GameCyberpunk2077::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
  const double normalizeddepth = static_cast<double>(depthval) / 4294967296.0;
  // This game has a logarithmic depth buffer with unknown constant(s).
  // These numbers were found by a curve fit, so are approximate,
  // but should be about 99.5% accurate for depths from centimeters to kilometers
	return 1.28410601 / (0.000080821547 + exp_fast_approx(355.3397906 * normalizeddepth - 83.92854443));
}
