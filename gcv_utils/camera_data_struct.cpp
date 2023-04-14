// Copyright (C) 2022 Jason Bunk
#include "camera_data_struct.h"
#include <sstream>
#include <iomanip>

nlohmann::json CamMatrixData::as_json() const {
	nlohmann::json rj;
	rj[((extrinsic_status == CamMatrix_AllGood) ? "extrinsic_cam2world" : "extrinsic_WIP")] = cam_matrix_to_flattened_row_major_array(extrinsic_cam2world);
	if (fov_v_degrees > ftype(0.0)) rj["fov_v_degrees"] = fov_v_degrees;
	if (fov_h_degrees > ftype(0.0)) rj["fov_h_degrees"] = fov_h_degrees;
	return rj;
}
