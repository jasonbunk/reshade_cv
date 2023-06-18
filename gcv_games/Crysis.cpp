// Copyright (C) 2022 Jason Bunk
#include "Crysis.h"
#include "gcv_utils/memread.h"

std::string GameCrysis::gamename_verbose() const { return "Crysis2008_GOG_DX10_x64"; } // tested for this build

std::string GameCrysis::camera_dll_name() const { return "Cry3DEngine.dll"; }
uint64_t GameCrysis::camera_dll_mem_start() const { return 0x2008F0ull; }
GameCamDLLMatrixType GameCrysis::camera_dll_matrix_format() const { return GameCamDLLMatrix_3x4; }

bool GameCrysis::can_interpret_depth_buffer() const {
	return true;
}

#define NEAR_PLANE_DISTANCE 0.25

// need to scan far plane in memory because it seems to change per level?
#define FAR_PLANE_DISTANCE 5000.0

float GameCrysis::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
	const double znorm = static_cast<double>(depthval) / 16777215.0;
	return static_cast<float>(NEAR_PLANE_DISTANCE / std::max(0.0000001, 1.0 - znorm * (1.0 - NEAR_PLANE_DISTANCE / FAR_PLANE_DISTANCE)));
}
