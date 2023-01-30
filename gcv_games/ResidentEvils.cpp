// Copyright (C) 2022 Jason Bunk
#include "ResidentEvils.h"
#include "gcv_utils/depth_utils.h"
#include "gcv_utils/scripted_cam_buf_templates.h"

// For these games, scripting is made easy by https://github.com/praydog/REFramework
// so I provide a simple lua script which stashes camera coordinates into a contiguous buffer which is double[].

std::string GameResidentEvils::gamename_verbose() const { return "ResidentEvil"; }

std::string GameResidentEvils::camera_dll_name() const { return ""; } // no dll name, it's available in the exe memory space
uint64_t GameResidentEvils::camera_dll_mem_start() const { return 0; }
GameCamDLLMatrixType GameResidentEvils::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf; }

scriptedcam_checkbuf_funptr GameResidentEvils::get_scriptedcambuf_checkfun() const {
	return template_check_scriptedcambuf_hash<double, 13, 2>;
}
uint64_t GameResidentEvils::get_scriptedcambuf_sizebytes() const {
	return template_scriptedcambuf_sizebytes<double, 13, 2>();
}
bool GameResidentEvils::copy_scriptedcambuf_to_matrix(uint8_t* buf, uint64_t buflen, CamMatrixData& rcam, std::string& errstr) const {
	return template_copy_scriptedcambuf_extrinsic_cam2world_and_fov<double, 13, 2>(buf, buflen, rcam, false, errstr);
}

bool GameResidentEvils::can_interpret_depth_buffer() const {
	return true;
}
float GameResidentEvils::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
	const double normalizeddepth = static_cast<double>(depthval) / 4294967295.0;
	// This game has a logarithmic depth buffer with unknown constant(s).
	// These numbers were found by a curve fit, so are approximate.
	return 1.28064135 / (0.00045998854 + exp_fast_approx(355.142568 * normalizeddepth - 83.194776));
}