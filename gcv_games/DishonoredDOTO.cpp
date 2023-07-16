// Copyright (C) 2022 Jason Bunk
#include "DishonoredDOTO.h"
#include "gcv_utils/memread.h"

std::string GameDishonoredDOTO::gamename_verbose() const { return "DishonoredDOTO_Epic_v1p145"; } // tested for this build

std::string GameDishonoredDOTO::camera_dll_name() const { return ""; }
uint64_t GameDishonoredDOTO::camera_dll_mem_start() const { return 0x522ED50ull; }
GameCamDLLMatrixType GameDishonoredDOTO::camera_dll_matrix_format() const { return GameCamDLLMatrix_4x4; }

void GameDishonoredDOTO::camera_matrix_postprocess_rotate(CamMatrixData& rcam) const {
    Eigen::Matrix<ftype, 4, 4> rot;
    rot << 0, 0, 1, 0,
          -1, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 0, 1;
    rcam.extrinsic_cam2world = (rcam.extrinsic_cam2world * rot);
}

bool GameDishonoredDOTO::can_interpret_depth_buffer() const {
    return true;
}
float GameDishonoredDOTO::convert_to_physical_distance_depth_u64(uint64_t depthval) const {
    const double normalizeddepth = static_cast<double>(depthval) / 16777215.0;
    return 5415.69378002 / (1.0 + 541167.20430436 * normalizeddepth);
}
