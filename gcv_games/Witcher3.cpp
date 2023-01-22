// Copyright (C) 2022 Jason Bunk
#include "Witcher3.h"
#include "gcv_utils/depth_utils.h"

// For this game, script modding is available, so it's easiest to grab the camera coordinates using that.
// A script to create a Witcher 3 mod is provided in the mod scripts folder.
// The mod simply stashes camera coordinates every frame into a float[] buffer.

std::string GameWitcher3::gamename_verbose() const { return "Witcher3_patch20221222"; } // tested for this build

std::string GameWitcher3::camera_dll_name() const { return ""; }
uint64_t GameWitcher3::camera_dll_mem_start() const { return 0; } // position available at 0x56445B0ull for patch 2022-12-22
GameCamDLLMatrixType GameWitcher3::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_float; }

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