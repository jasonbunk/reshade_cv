// Copyright (C) 2022 Jason Bunk
#include "Cyberpunk2077.h"
#include "gcv_utils/depth_utils.h"

// For this game, scripting is made easy by Cyber Engine Tweaks,
// so I provide a simple lua script which stashes camera coordinates into a double[] buffer.

std::string GameCyberpunk2077::gamename_verbose() const { return "Cyberpunk2077_patch161"; } // tested for this build

std::string GameCyberpunk2077::camera_dll_name() const { return ""; } // no dll name, it's available in the exe memory space
uint64_t GameCyberpunk2077::camera_dll_mem_start() const { return 0; }
GameCamDLLMatrixType GameCyberpunk2077::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_double; }

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