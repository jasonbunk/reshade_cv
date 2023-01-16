#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_with_camera_data_in_one_dll.h"

class GameControl : public GameWithCameraDataInOneDLL {
public:
	virtual std::string gamename_simpler() const override { return "Control"; }

	virtual bool can_interpret_depth_buffer() const override;
	virtual float convert_to_physical_distance_depth_u64(uint64_t depthval) const override;
};

class GameControlDX11 : public GameControl {
protected:
	virtual std::string camera_dll_name() const override;
	virtual uint64_t camera_dll_mem_start() const override;
	virtual GameCamDLLMatrixType camera_dll_matrix_format() const override;
public:
	virtual std::string gamename_verbose() const override;
};

class GameControlDX12 : public GameControl {
protected:
	virtual std::string camera_dll_name() const override;
	virtual uint64_t camera_dll_mem_start() const override;
	virtual GameCamDLLMatrixType camera_dll_matrix_format() const override;
public:
	virtual std::string gamename_verbose() const override;
};

REGISTER_GAME_INTERFACE(GameControlDX11, 0, "control_dx11.exe");
REGISTER_GAME_INTERFACE(GameControlDX12, 1, "control_dx12.exe");
