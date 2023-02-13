#pragma once
// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/geometry.h"

enum CamMatrixStatus {
	CamMatrix_Uninitialized = 0,
	CamMatrix_PositionGood = 1,
	CamMatrix_RotationGood = 2,
	CamMatrix_AllGood = 3,
	CamMatrix_WIP = 4,
};

template<typename FT>
struct CamMatrixDataT {
	CamMatrixStatus extrinsic_status = CamMatrix_Uninitialized;
	CamMatrixT<FT> extrinsic_cam2world;
	FT fov_v_degrees = FT(-9999.0);
	FT fov_h_degrees = FT(-9999.0);
	bool intrinsic_status() const { return fov_v_degrees > FT(0.0) || fov_h_degrees > FT(0.0); }
	std::string serialize_for_json(bool wrap_dict) const;
};

typedef CamMatrixDataT<ftype> CamMatrixData;
