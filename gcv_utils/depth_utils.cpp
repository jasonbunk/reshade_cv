#include "depth_utils.h"

// source: Nicol Schraudolph, Edward Kmett
// Academic paper: "A Fast, Compact Approximation of the Exponential Function", Nicol N. Schraudolph, 1999
// https://github.com/ekmett/approximate/blob/master/cbits/fast.c
// License of Edward Kmett's code: BSD 3-clause

double exp_fast_approx(double a) {
  union { double d; long long x; } u;
  u.x = (long long)(6497320848556798LL * a + 0x3fef127e83d16f12LL);
  return u.d;
}
