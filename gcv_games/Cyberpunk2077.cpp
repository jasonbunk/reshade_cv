// Copyright (C) 2022 Jason Bunk
#include "Cyberpunk2077.h"
#include "gcv_utils/memread.h"

std::string GameCyberpunk2077::gamename_verbose() const {
	return "Cyberpunk2077_patch161"; // tested for this build
}

bool GameCyberpunk2077::init_in_game() {
	if (renderer_dll != 0)
		return true;
	renderer_dll = GetModuleHandle(0); // no dll, camera is within the exe memory space
	return renderer_dll != 0;
}

uint8_t GameCyberpunk2077::get_camera_matrix(CamMatrix &rcam, std::string &errstr) {
	if (!init_in_game()) return CamMatrix_Uninitialized;
	UINT_PTR dll4cambaseaddr = (UINT_PTR)renderer_dll;
	SIZE_T nbytesread = 0;
	const uint64_t camloc = 0x4AA79F0ull;
	float cambuf[3];
	for (uint64_t colidx = 0; colidx < 4; ++colidx) {
		if (!tryreadmemory(gamename_verbose() + std::string("_3x4cam_col") + std::to_string(colidx),
				errstr, mygame_handle_exe, (void *)(dll4cambaseaddr + camloc + colidx*16),
				reinterpret_cast<LPVOID>(cambuf), 3*sizeof(float), &nbytesread)) {
			return CamMatrix_Uninitialized;
		}
		rcam.GetCol(colidx).x = cambuf[0];
		rcam.GetCol(colidx).y = cambuf[1];
		rcam.GetCol(colidx).z = cambuf[2];
	}
	return CamMatrix_AllGood;
}
