#pragma once
// Copyright (C) 2022 Jason Bunk
#include <cmath>
#include <sstream>
#include "camera_data_struct.h"

// https://stackoverflow.com/questions/4915462/how-should-i-do-floating-point-comparison
template<typename FT>
bool floats_nearly_equal(FT a, FT b, FT epsmult = FT(128)) {
    if (a == b) return true;
    const FT diff = std::abs(a - b);
    const FT norm = std::min((std::abs(a) + std::abs(b)), std::numeric_limits<FT>::max());
    return diff < std::max(std::numeric_limits<FT>::min(), (epsmult* std::numeric_limits<FT>::epsilon())* norm);
}

// if float32 with stride>1, will have trouble, because was expecting to trigger on a unique 8-byte signature
template<typename FT, int bufstride> constexpr
uint64_t template_scriptedcambuf_numtriggerbytes() {
    return (bufstride == 1) ? 8 : (sizeof(FT) * bufstride);
}

template<typename FT, int numcamfloatshashed, int bufstride> constexpr
uint64_t template_scriptedcambuf_sizebytes() {
    return template_scriptedcambuf_numtriggerbytes<FT, bufstride>() + ((numcamfloatshashed + 3) * sizeof(FT) * bufstride);
}

// A simple lua script can write camera coordinates into an array of contiguous floats.
// During a memory scan, this function can check a potentially matching memory buffer by reading the floats and verifying their hash.
template<typename FT, int numcamfloatshashed, int bufstride>
bool template_check_scriptedcambuf_hash(const void* scanctx, const uint8_t* buf, uint64_t buflen) {
    if (buflen < template_scriptedcambuf_sizebytes<FT, numcamfloatshashed, bufstride>()) {
        return false;
    }
    const FT* bufvals = reinterpret_cast<const FT*>(buf + template_scriptedcambuf_numtriggerbytes<FT, bufstride>());
    if (std::isfinite(bufvals[0]) && bufvals[0] > 0.5) {
        FT allsumhash = bufvals[0]; // start with the counter; the counter is included in the hash
        FT plusorminushhash = bufvals[0];
        for (int ii = 1; ii < (1 + numcamfloatshashed); ++ii) {
            allsumhash += bufvals[ii * bufstride];
            if (ii % 2 == 0) {
                plusorminushhash += bufvals[ii * bufstride];
            }
            else {
                plusorminushhash -= bufvals[ii * bufstride];
            }
        }
        return floats_nearly_equal<FT>(allsumhash, bufvals[(1 + numcamfloatshashed) * bufstride])
            && floats_nearly_equal<FT>(plusorminushhash, bufvals[(2 + numcamfloatshashed) * bufstride]);
    }
    return false;
}

template<typename FT, int numcamfloatshashed, int bufstride>
bool template_copy_scriptedcambuf_extrinsic_cam2world_and_fov(uint8_t* buf, uint64_t buflen, CamMatrixData& rcam, bool fov_is_vertical, std::string& errstr) {
    const FT* dbuf = reinterpret_cast<const FT*>(buf + template_scriptedcambuf_numtriggerbytes<FT, bufstride>() + sizeof(FT) * bufstride);
    for (int ii = 0; ii < 12; ++ii) {
        rcam.extrinsic_cam2world.arr3x4[ii] = dbuf[ii * bufstride];
    }
    if (fov_is_vertical) {
        rcam.fov_v_degrees = dbuf[12 * bufstride];
    } else {
        rcam.fov_h_degrees = dbuf[12 * bufstride];
    }
    return true;
}
