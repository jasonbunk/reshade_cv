#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Eigen/Dense>

typedef double ftype;
typedef Eigen::Matrix<ftype,3,4> CamMatrix;
typedef Eigen::Matrix<ftype,4,1> Vec4;
typedef Eigen::Matrix<ftype,3,1> Vec3;

constexpr int cam_matrix_position_column = 3;

CamMatrix build_cam_matrix_from_pos_and_lookdir(Vec3 pos, Vec3 lookdir);

// helpers for serializing

template<int rows, int cols>
std::array<ftype, (rows*cols)> eigen_matrix_to_flattened_row_major_array(Eigen::Matrix<ftype, rows, cols> mat) {
	std::array<ftype, (rows*cols)> flattened_rowmajor;
	for (int ii = 0; ii < rows; ++ii) {
		for (int jj = 0; jj < cols; ++jj) {
			flattened_rowmajor[ii * cols + jj] = mat(ii, jj);
		}
	}
	return flattened_rowmajor;
}

std::array<ftype, 12> cam_matrix_to_flattened_row_major_array(CamMatrix mat);

// helpers for deserializing

template<typename SRCFT, int rows, int cols>
Eigen::Matrix<ftype, rows, cols> eigen_matrix_from_flattened_row_major_buffer(const SRCFT* buf) {
	Eigen::Matrix<ftype, rows, cols> mat;
	for (int ii = 0; ii < rows; ++ii) {
		for (int jj = 0; jj < cols; ++jj) {
			mat(ii, jj) = buf[ii * cols + jj];
		}
	}
	return mat;
}

template<typename SRCFT>
CamMatrix cam_matrix_from_flattened_row_major_buffer(const SRCFT* buf) {
	return eigen_matrix_from_flattened_row_major_buffer<SRCFT, 3, 4>(buf);
}
