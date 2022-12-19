#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_interface.h"

class GameCyberpunk2077 : public GameInterface {
protected:
	HMODULE renderer_dll = 0;
public:
	virtual std::string gamename_simpler() const override { return "Cyberpunk2077"; }
	virtual std::string gamename_verbose() const override;

	virtual bool init_in_game() override;
	virtual uint8_t get_camera_matrix(CamMatrix &rcam, std::string &errstr) override;
};

REGISTER_GAME_INTERFACE(GameCyberpunk2077, 0, "cyberpunk2077.exe");
