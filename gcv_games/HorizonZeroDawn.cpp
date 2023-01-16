// Copyright (C) 2022 Jason Bunk
#include "HorizonZeroDawn.h"
#include "gcv_utils/memread.h"

std::string GameHorizonZeroDawn::gamename_verbose() const { return "HorizonZeroDawn_GOG"; } // tested for this build

std::string GameHorizonZeroDawn::camera_dll_name() const { return ""; } // no dll name, it's available in the exe memory space
uint64_t GameHorizonZeroDawn::camera_dll_mem_start() const { return 0x300D200ull; }
GameCamDLLMatrixType GameHorizonZeroDawn::camera_dll_matrix_format() const { return GameCamDLLMatrix_3x4; }

bool GameHorizonZeroDawn::can_interpret_depth_buffer() const {
	return true;
}
float GameHorizonZeroDawn::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
	const double normalizeddepth = static_cast<double>(depthval) / 1073741824.0; // 30 bits depth
	return 0.00313259 / (1.0 - normalizeddepth * 1.00787352);
}
