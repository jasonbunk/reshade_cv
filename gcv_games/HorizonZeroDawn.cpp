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

bool GameHorizonZeroDawn::get_camera_matrix(CamMatrixData& rcam, std::string& errstr) {
	rcam.extrinsic_status = CamMatrix_Uninitialized;
	if (!init_in_game()) return false;
	const UINT_PTR dll4cambaseaddr = (UINT_PTR)camera_dll;
	SIZE_T nbytesread = 0;
	const uint64_t camlocstart = camera_dll_mem_start();
	float readbuf[25];
	Vec3 campos, camdir;
	if (tryreadmemory(gamename_verbose() + std::string("_camandlook"), errstr, mygame_handle_exe,
		(LPCVOID)(dll4cambaseaddr + camlocstart), reinterpret_cast<LPVOID>(readbuf), 100, &nbytesread)) {
		for (int ii = 0; ii < 3; ++ii) {
			campos(ii) = readbuf[ii * 4];
			camdir(ii) = readbuf[(ii + 4) * 4];
		}
		rcam.extrinsic_cam2world = build_cam_matrix_from_pos_and_lookdir(campos, camdir);
		rcam.extrinsic_status = CamMatrix_AllGood;
		return true;
	}
	return false;
}
