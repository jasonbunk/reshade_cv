#pragma once
// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/geometry.h"
#include "gcv_utils/simple_packed_buf.h"
#include "game_interface_factory_registration.h"
#include <Windows.h>
#include <string>

enum CamMatrixStatus {
	CamMatrix_Uninitialized = 0,
	CamMatrix_PositionGood = 1,
	CamMatrix_RotationGood = 2,
	CamMatrix_AllGood = 3,
	CamMatrix_WIP = 4,
};

class GameInterface {
protected:
	HANDLE mygame_handle_exe = 0;
	bool init_get_game_exe();
public:
	virtual ~GameInterface() {}

	virtual std::string gamename_simpler() const = 0;
	virtual std::string gamename_verbose() const = 0;

	// can do on game startup
	virtual bool init_on_startup() { return init_get_game_exe(); }

	// wait until in 3D rendered world, not main menu
	virtual bool init_in_game() = 0;

	// return a camera matrix status from above enum (can return multiple flags)
	virtual uint8_t get_camera_matrix(CamMatrix &rcam, std::string &errstr) = 0;

	// convert from integer depth to floating-point distance
	virtual bool can_interpret_depth_buffer() const { return false; }
	virtual float convert_to_physical_distance_depth_u64(uint64_t depthval) const { return 0.0f; }

	// memory scans
	virtual bool scan_all_memory_for_scripted_cam_matrix(std::string& errstr) { errstr += "not implemented"; return false; }
};
