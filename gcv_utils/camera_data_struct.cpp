// Copyright (C) 2022 Jason Bunk
#include "camera_data_struct.h"
#include <sstream>

template<typename FT>
std::string CamMatrixDataT<FT>::serialize_for_json(bool wrap_dict) const {
	std::stringstream rstr;
	if (wrap_dict) rstr << "{";
	rstr << "\"" << ((extrinsic_status == CamMatrix_AllGood) ? "extrinsic_cam2world" : "extrinsic_WIP")
		<< "\":" << extrinsic_cam2world.serialize();
	if (fov_v_degrees > FT(0.0)) {
		rstr << ",\"fov_v_degrees\":" << fov_v_degrees;
	}
	if (fov_h_degrees > FT(0.0)) {
		rstr << ",\"fov_h_degrees\":" << fov_h_degrees;
	}
	if (wrap_dict) rstr << "}";
	return rstr.str();
}

// instantiate float and double classes
#define INSTANTIATEGEOMCLASS(clsnam) template struct clsnam <float>; template struct clsnam <double>

INSTANTIATEGEOMCLASS(CamMatrixDataT);