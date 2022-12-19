#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_interface.h"

class GameControl : public GameInterface {
protected:
	HMODULE renderer_dll = 0;
public:
	virtual std::string gamename_simpler() const override { return "Control"; }
	virtual std::string gamename_verbose() const override;

	virtual bool init_in_game() override;
	virtual uint8_t get_camera_matrix(CamMatrix &rcam, std::string &errstr) override;
};

REGISTER_GAME_INTERFACE(GameControl, 0, "control_dx11.exe");
