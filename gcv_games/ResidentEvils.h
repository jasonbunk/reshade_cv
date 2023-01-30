#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_with_camera_data_in_one_dll.h"

class GameResidentEvils : public GameWithCameraDataInOneDLL {
protected:
	virtual std::string camera_dll_name() const override;
	virtual uint64_t camera_dll_mem_start() const override;
	virtual GameCamDLLMatrixType camera_dll_matrix_format() const override;
public:
	virtual std::string gamename_simpler() const override { return "ResidentEvil"; }
	virtual std::string gamename_verbose() const override;

	virtual scriptedcam_checkbuf_funptr get_scriptedcambuf_checkfun() const override;
	virtual uint64_t get_scriptedcambuf_sizebytes() const override;
	virtual bool copy_scriptedcambuf_to_matrix(uint8_t* buf, uint64_t buflen, CamMatrixData& rcam, std::string& errstr) const override;

	virtual bool can_interpret_depth_buffer() const override;
	virtual float convert_to_physical_distance_depth_u64(uint64_t depthval) const override;
};

REGISTER_GAME_INTERFACE(GameResidentEvils, 0, "re2.exe"); // RE2 remake
REGISTER_GAME_INTERFACE(GameResidentEvils, 1, "re3.exe"); // RE3 remake
// todo test other games supported by REFramework
