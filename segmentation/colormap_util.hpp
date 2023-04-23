// Copyright (C) 2023 Jason Bunk
#pragma once
#include <array>

inline uint32_t halton_sequence_u32_powerof2_i(uint32_t exponent, uint32_t i) {
  if (exponent >= 32) return 0; // TODO assert instead of silently fail
  exponent = (1 << exponent);
  uint32_t r = 0;
  while(i > 0) {
    exponent /= 2;
    r += exponent * (i % 2);
    i /= 2;
  }
  return r;
}

template<typename rtype, uint32_t dim, uint32_t bitsperdim>
std::array<rtype,dim> morton_halton_curve(uint32_t i) {
  const uint32_t halton_value = halton_sequence_u32_powerof2_i(dim*bitsperdim, i);
  std::array<rtype,dim> r;
  for(uint32_t d=0; d<dim; ++d) {
    r[d] = rtype(0);
    for(uint32_t b=0; b<bitsperdim; ++b) {
      r[d] += static_cast<rtype>(((halton_value & (1 << (b*dim + d))) > 0) << b);
    }
  }
  return r;
}

// Index colors along morton z curve using halton sequence from 0 to 2^24-1
// This quickly generates a sequence of colors that are reasonably spread out
inline std::array<uint8_t,3> morton_halton_curve_rgb_3d(uint32_t i) {
  return morton_halton_curve<uint8_t,3,8>(i);
}