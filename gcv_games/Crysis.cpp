// Copyright (C) 2022 Jason Bunk
#include "Crysis.h"
#include "gcv_utils/memread.h"

std::string GameCrysis::gamename_verbose() const {
	return "Crysis2008_GOG_DX10_x64"; // tested for this build
}

bool GameCrysis::init_in_game() {
	if (renderer_dll != 0)
		return true;
	renderer_dll = GetModuleHandle(TEXT("Cry3DEngine.dll"));
	return renderer_dll != 0;
}

uint8_t GameCrysis::get_camera_matrix(CamMatrix &rcam, std::string &errstr) {
	if (!init_in_game()) return CamMatrix_Uninitialized;
	UINT_PTR dll4cambaseaddr = (UINT_PTR)renderer_dll;
	SIZE_T nbytesread = 0;
	if (tryreadmemory(gamename_verbose()+std::string("_3x4cam"), errstr, mygame_handle_exe,
			(void *)(dll4cambaseaddr + 0x2008F0ull), reinterpret_cast<LPVOID>(rcam.arr3x4),
			12 * sizeof(float), &nbytesread)) {
		return CamMatrix_AllGood;
	}
	return CamMatrix_Uninitialized;
}
