// Copyright (C) 2022 Jason Bunk
#include "game_interface.h"

bool GameInterface::init_get_game_exe() {
	if (mygame_handle_exe != 0) return true;
	mygame_handle_exe = GetCurrentProcess();
	return mygame_handle_exe != 0;
}
