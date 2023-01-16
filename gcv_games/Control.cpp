// Copyright (C) 2022 Jason Bunk
#include "Control.h"
#include "gcv_utils/memread.h"

bool GameControl::can_interpret_depth_buffer() const {
	return true;
}
float GameControl::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
	const double normalizeddepth = static_cast<double>(depthval) / 1073741824.0; // 30 bits depth
	return 0.00310475 / (1.0 - normalizeddepth * 1.00787427);
}

std::string GameControlDX11::gamename_verbose() const { return "Control_DX11"; }
std::string GameControlDX12::gamename_verbose() const { return "Control_DX12"; }

std::string GameControlDX11::camera_dll_name() const { return "renderer_rmdwin7_f.dll"; }
uint64_t GameControlDX11::camera_dll_mem_start() const { return 0x1266D30ull; } // also at 0x126FC80
GameCamDLLMatrixType GameControlDX11::camera_dll_matrix_format() const { return GameCamDLLMatrix_4x4; }

std::string GameControlDX12::camera_dll_name() const { return "renderer_rmdwin10_f.dll"; }
uint64_t GameControlDX12::camera_dll_mem_start() const { return 0x1291110ull; } // also at 0x129A630
GameCamDLLMatrixType GameControlDX12::camera_dll_matrix_format() const { return GameCamDLLMatrix_4x4; }
