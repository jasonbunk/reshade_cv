// Copyright (C) 2022 Jason Bunk
#include "Cyberpunk2077.h"
#include "gcv_utils/memread.h"

std::string GameCyberpunk2077::gamename_verbose() const { return "Cyberpunk2077_patch161"; } // tested for this build

std::string GameCyberpunk2077::camera_dll_name() const { return ""; } // no dll name, it's available in the exe memory space
uint64_t GameCyberpunk2077::camera_dll_mem_start() const { return 0; }
GameCamDLLMatrixType GameCyberpunk2077::camera_dll_matrix_format() const { return GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform; }

bool GameCyberpunk2077::can_interpret_depth_buffer() const {
	return true;
}
float GameCyberpunk2077::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
  const double normalizeddepth = static_cast<double>(depthval) / 1073741824.0; // 30 bits depth
	return exp(84.1418 - 88.79965 * normalizeddepth); // the numbers, what do they mean?
  // warning: cyberpunk uses a fast approximate log, so this isn't very precise
}
