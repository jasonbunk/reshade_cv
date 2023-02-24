// Copyright (C) 2022 Jason Bunk
#include "geometry.h"

std::array<ftype, 12> cam_matrix_to_flattened_row_major_array(CamMatrix mat) {
	return eigen_matrix_to_flattened_row_major_array<3, 4>(mat);
}

CamMatrix build_cam_matrix_from_pos_and_lookdir(Vec3 pos, Vec3 lookdir) {
	CamMatrix m;
	for (int ii = 0; ii < 3; ++ii) {
		m(ii,3) = pos(ii);
		m(ii,1) = lookdir(ii);
	}
	m.col(1).normalize();
	m(0,0) = m(1,1);
	m(1,0) = -m(0,1);
	m(2,0) = ftype(0.0);
	m.col(0).normalize();
	m.col(2) = m.col(0).cross(m.col(1));
	return m;
}
