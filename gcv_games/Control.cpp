// Copyright (C) 2022 Jason Bunk
#include "Control.h"
#include "gcv_utils/memread.h"

std::string GameControl::gamename_verbose() const {
	return "Control_DX11"; // tested for this build
}

bool GameControl::init_in_game() {
	if (renderer_dll != 0)
		return true;
	renderer_dll = GetModuleHandle(TEXT("renderer_rmdwin7_f.dll"));
	return renderer_dll != 0;
}

uint8_t GameControl::get_camera_matrix(CamMatrix &rcam, std::string &errstr) {
	if (!init_in_game()) return CamMatrix_Uninitialized;
	UINT_PTR dll4cambaseaddr = (UINT_PTR)renderer_dll;
	SIZE_T nbytesread = 0;
	const uint64_t camloc = 0x1266D60ull;
	float cambuf[3];
	if (!tryreadmemory(gamename_verbose() + std::string("_3x4cam"),
		errstr, mygame_handle_exe, (void *)(dll4cambaseaddr + camloc),
		reinterpret_cast<LPVOID>(cambuf), 3 * sizeof(float), &nbytesread)) {
		return CamMatrix_Uninitialized;
	}
	rcam.make_zero();
	rcam.GetPosition().x = cambuf[0];
	rcam.GetPosition().y = cambuf[1];
	rcam.GetPosition().z = cambuf[2];
	return CamMatrix_PositionGood;
}
