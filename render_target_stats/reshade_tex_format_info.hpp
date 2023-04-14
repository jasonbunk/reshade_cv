#pragma once
// Copyright (C) 2023 Jason Bunk
#include <reshade.hpp>
#include <map>
#include <string>

namespace reshade::api
{
#define RESHADE_API_FORMATS_XMACRO \
		X(unknown,     0, 0) \
		X(r1_unorm,    1, 66) \
		X(l8_unorm,    1, 0x3030384C) \
		X(a8_unorm,    1, 65) \
		X(r8_typeless, 1, 60) \
		X(r8_uint,     1, 62) \
		X(r8_sint,     1, 64) \
		X(r8_unorm,    1, 61) \
		X(r8_snorm,    1, 63) \
		X(l8a8_unorm,    2, 0x3038414C) \
		X(r8g8_typeless, 2, 48) \
		X(r8g8_uint,     2, 50) \
		X(r8g8_sint,     2, 52) \
		X(r8g8_unorm,    2, 49) \
		X(r8g8_snorm,    2, 51) \
		X(r8g8b8a8_typeless,  4, 27) \
		X(r8g8b8a8_uint,      4, 30) \
		X(r8g8b8a8_sint,      4, 32) \
		X(r8g8b8a8_unorm,     4, 28) \
		X(r8g8b8a8_unorm_srgb,4, 29) \
		X(r8g8b8a8_snorm,     4, 31) \
		X(r8g8b8x8_unorm,     4, 0x424757B9) \
		X(r8g8b8x8_unorm_srgb,4, 0x424757BA) \
		X(b8g8r8a8_typeless,  4, 90) \
		X(b8g8r8a8_unorm,     4, 87) \
		X(b8g8r8a8_unorm_srgb,4, 91) \
		X(b8g8r8x8_typeless,  4, 92) \
		X(b8g8r8x8_unorm,     4, 88) \
		X(b8g8r8x8_unorm_srgb,4, 93) \
		X(r10g10b10a2_typeless,4, 23) \
		X(r10g10b10a2_uint,   4, 25) \
		X(r10g10b10a2_unorm,  4, 24) \
		X(r10g10b10a2_xr_bias,4, 89) \
		X(b10g10r10a2_typeless,4, 0x42475330) \
		X(b10g10r10a2_uint,   4, 0x42475332) \
		X(b10g10r10a2_unorm,  4, 0x42475331) \
		X(l16_unorm,    1, 0x3036314C) \
		X(r16_typeless, 1, 53) \
		X(r16_uint,     1, 57) \
		X(r16_sint,     1, 59) \
		X(r16_unorm,    1, 56) \
		X(r16_snorm,    1, 58) \
		X(r16_float,    1, 54) \
		X(l16a16_unorm, 2, 0x3631414C) \
		X(r16g16_typeless,2, 33) \
		X(r16g16_uint,  2, 36) \
		X(r16g16_sint,  2, 38) \
		X(r16g16_unorm, 2, 35) \
		X(r16g16_snorm, 2, 37) \
		X(r16g16_float, 2, 34) \
		X(r16g16b16a16_typeless,4, 9) \
		X(r16g16b16a16_uint,    4, 12) \
		X(r16g16b16a16_sint,    4, 14) \
		X(r16g16b16a16_unorm,   4, 11) \
		X(r16g16b16a16_snorm,   4, 13) \
		X(r16g16b16a16_float,   4, 10) \
		X(r32_typeless,1, 39) \
		X(r32_uint,    1, 42) \
		X(r32_sint,    1, 43) \
		X(r32_float,   1, 41) \
		X(r32g32_typeless,2, 15) \
		X(r32g32_uint,    2, 17) \
		X(r32g32_sint,    2, 18) \
		X(r32g32_float,   2, 16) \
		X(r32g32b32_typeless,3, 5) \
		X(r32g32b32_uint,    3, 7) \
		X(r32g32b32_sint,    3, 8) \
		X(r32g32b32_float,   3, 6) \
		X(r32g32b32a32_typeless,4, 1) \
		X(r32g32b32a32_uint,    4, 3) \
		X(r32g32b32a32_sint,    4, 4) \
		X(r32g32b32a32_float,   4, 2) \
		X(r9g9b9e5, 4, 67) \
		X(r11g11b10_float, 3, 26) \
		X(b5g6r5_unorm, 3, 85) \
		X(b5g5r5a1_unorm,4, 86) \
		X(b5g5r5x1_unorm,4, 0x424757B5) \
		X(b4g4r4a4_unorm,4, 115) \
		X(s8_uint,       1, 0x30303853) \
		X(d16_unorm,     1, 55) \
		X(d16_unorm_s8_uint,2, 0x38363144) \
		X(d24_unorm_x8_uint,2, 0x38343244) \
		X(d24_unorm_s8_uint,2, 45) \
		X(d32_float,        1, 40) \
		X(d32_float_s8_uint,2, 20) \
		X(r24_g8_typeless,  2, 44) \
		X(r24_unorm_x8_uint,2, 46) \
		X(x24_unorm_g8_uint,2, 47) \
		X(r32_g8_typeless,  2, 19) \
		X(r32_float_x8_uint,2, 21) \
		X(x32_float_g8_uint,2, 22) \
		X(bc1_typeless,  5, 70) \
		X(bc1_unorm,     5, 71) \
		X(bc1_unorm_srgb,5, 72) \
		X(bc2_typeless,  5, 73) \
		X(bc2_unorm,     5, 74) \
		X(bc2_unorm_srgb,5, 75) \
		X(bc3_typeless,  5, 76) \
		X(bc3_unorm,     5, 77) \
		X(bc3_unorm_srgb,5, 78) \
		X(bc4_typeless,  5, 79) \
		X(bc4_unorm,     5, 80) \
		X(bc4_snorm,     5, 81) \
		X(bc5_typeless,  5, 82) \
		X(bc5_unorm,     5, 83) \
		X(bc5_snorm,     5, 84) \
		X(bc6h_typeless, 5, 94) \
		X(bc6h_ufloat,   5, 95) \
		X(bc6h_sfloat,   5, 96) \
		X(bc7_typeless,  5, 97) \
		X(bc7_unorm,     5, 98) \
		X(bc7_unorm_srgb,5, 99) \
		X(r8g8_b8g8_unorm,3, 68) \
		X(g8r8_g8b8_unorm,3, 69) \
		X(intz, 1, 0x5A544E49)

#define X(evv,ech,eii) { format::evv, #eii ":" #evv },
	static const std::map<format,std::string> fmtnames = {
RESHADE_API_FORMATS_XMACRO
	};
#undef X

#define X(evv,ech,eii) { format::evv, ech },
	static const std::map<format,int> fmtchannels = {
RESHADE_API_FORMATS_XMACRO
	};
#undef X

#define X(evv,ech,eii) { format::evv, (ech==3 || ech==4) },
	static const std::map<format, bool> fmtchannelsdisplaycolorlike = {
RESHADE_API_FORMATS_XMACRO
	};
#undef X

#define X(evv,ech,eii) { format::evv, (ech==1 || ech==2) },
	static const std::map<format, bool> fmtchannelsdepthlike = {
RESHADE_API_FORMATS_XMACRO
	};
#undef X
}