#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_with_camera_data_in_one_dll.h"

class GameDishonoredDOTO : public GameWithCameraDataInOneDLL {
protected:
    virtual std::string camera_dll_name() const override;
    virtual uint64_t camera_dll_mem_start() const override;
    virtual GameCamDLLMatrixType camera_dll_matrix_format() const override;
    virtual void camera_matrix_postprocess_rotate(CamMatrixData& rcam) const override;
public:
    virtual std::string gamename_simpler() const override { return "DishonoredDOTO"; }
    virtual std::string gamename_verbose() const override;

    virtual bool can_interpret_depth_buffer() const override;
    virtual float convert_to_physical_distance_depth_u64(uint64_t depthval) const override;
};

REGISTER_GAME_INTERFACE(GameDishonoredDOTO, 0, "dishonored_do.exe");
