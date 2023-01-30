// Copyright (C) 2022 Jason Bunk
#include "Witcher3.h"
#include "gcv_utils/depth_utils.h"
#include "gcv_utils/scripted_cam_buf_templates.h"

// For this game, script modding is available, so it's easiest to grab the camera coordinates using that.
// A script to create a Witcher 3 mod is provided in the mod scripts folder.
// The mod simply stashes camera coordinates every frame into a float[] buffer.

std::string GameWitcher3::gamename_verbose() const { return "Witcher3_nextgen"; } // hopefully continues to work as long as the game script doesnt change too much

std::string GameWitcher3::camera_dll_name() const { return ""; }
uint64_t GameWitcher3::camera_dll_mem_start() const { return 0; }
GameCamDLLMatrixType GameWitcher3::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf; }

scriptedcam_checkbuf_funptr GameWitcher3::get_scriptedcambuf_checkfun() const {
	return template_check_scriptedcambuf_hash<float, 13, 1>;
}
uint64_t GameWitcher3::get_scriptedcambuf_sizebytes() const {
	return template_scriptedcambuf_sizebytes<float, 13, 1>();
}
bool GameWitcher3::copy_scriptedcambuf_to_matrix(uint8_t* buf, uint64_t buflen, CamMatrixData& rcam, std::string& errstr) const {
	return template_copy_scriptedcambuf_extrinsic_cam2world_and_fov<float, 13, 1>(buf, buflen, rcam, true, errstr);
}

bool GameWitcher3::can_interpret_depth_buffer() const {
	return true;
}
float GameWitcher3::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
  const double normalizeddepth = static_cast<double>(depthval) / 4294967296.0;
  // This game has a logarithmic depth buffer with unknown constant(s).
  // These numbers were found by a curve fit over a range from centimeters to 40 meters.
  // Should be accurate to the millimeter over that range
  return 1.60130532 / (0.00041544 + exp_fast_approx(355.02435228 * normalizeddepth - 84.55415024));
}
