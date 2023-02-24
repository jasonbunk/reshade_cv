#pragma once
// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/geometry.h"
#include <nlohmann/json.hpp>

enum CamMatrixStatus {
	CamMatrix_Uninitialized = 0,
	CamMatrix_PositionGood = 1,
	CamMatrix_RotationGood = 2,
	CamMatrix_AllGood = 3,
	CamMatrix_WIP = 4,
};

struct CamMatrixData {
	CamMatrixStatus extrinsic_status = CamMatrix_Uninitialized;
	CamMatrix extrinsic_cam2world;
	ftype fov_v_degrees = ftype(-9999.0);
	ftype fov_h_degrees = ftype(-9999.0);
	bool intrinsic_status() const { return fov_v_degrees > ftype(0.0) || fov_h_degrees > ftype(0.0); }
	nlohmann::json as_json() const;
};
