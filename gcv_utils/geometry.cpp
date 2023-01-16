// Copyright (C) 2022 Jason Bunk
#include "geometry.h"
#include <sstream>

template<typename FT>
int Vec4T<FT>::serialize_into(char *rbuf, int buflen, bool wrapbrackets) {
	if (rbuf == nullptr || buflen <= 3) return -1;
	if (wrapbrackets) {
		rbuf[0] = '[';
		int gotlen = 1 + _snprintf(rbuf + 1, buflen - 1, "%.7f,%.7f,%.7f,%.7f", x, y, z, t);
		if (gotlen <= 0 || (gotlen + 2) >= buflen) return -1;
		rbuf[gotlen] = ']';
		return gotlen + 1;
	}
	return _snprintf(rbuf, buflen, "%.7f,%.7f,%.7f,%.7f", x, y, z, t);
}

template<typename FT>
int Vec4refT<FT>::serialize_into(char *rbuf, int buflen, bool wrapbrackets) {
	if (rbuf == nullptr || buflen <= 1) return -1;
	return Vec4(x, y, z, t).serialize_into(rbuf, buflen, wrapbrackets);
}

template<typename FT>
std::string Vec3refT<FT>::to_string(bool wrap_brackets) const {
	std::stringstream strstr;
	if (wrap_brackets) strstr << "[";
	strstr << x << "," << y << "," << z;
	if (wrap_brackets) strstr << "]";
	return strstr.str();
}
template<typename FT>
std::string Vec3T<FT>::to_string(bool wrap_brackets) const {
	std::stringstream strstr;
	if (wrap_brackets) strstr << "[";
	strstr << x << "," << y << "," << z;
	if (wrap_brackets) strstr << "]";
	return strstr.str();
}

template<typename FT>
CamMatrixT<FT>::CamMatrixT() {
	for (int ii = 0; ii < 12; ++ii) arr3x4[ii] = defaultunkfloatinit;
}
template<typename FT>
CamMatrixT<FT>::CamMatrixT(const float *rawptr3x4) {
	for (int ii = 0; ii < 12; ++ii) arr3x4[ii] = static_cast<FT>(rawptr3x4[ii]);
}
template<typename FT>
CamMatrixT<FT>::CamMatrixT(const double* rawptr3x4) {
	for (int ii = 0; ii < 12; ++ii) arr3x4[ii] = static_cast<FT>(rawptr3x4[ii]);
}
template<typename FT>
Vec4refT<FT> CamMatrixT<FT>::GetRow(int i) {
	// TODO: assertion if bad i
	return Vec4refT<FT>(arr3x4[i * 4 + 0],
				        arr3x4[i * 4 + 1],
				        arr3x4[i * 4 + 2],
				        arr3x4[i * 4 + 3]);
}
template<typename FT>
Vec3refT<FT> CamMatrixT<FT>::GetCol(int i) {
	// TODO: assertion if bad i
	return Vec3refT<FT>(arr3x4[i], arr3x4[4 + i], arr3x4[8 + i]);
}
template<typename FT>
FT& CamMatrixT<FT>::at(int i, int j) {
	// TODO: assertion if bad i or j
	return arr3x4[i * 4 + j];
}
template<typename FT>
FT CamMatrixT<FT>::atc(int i, int j) const {
	// TODO: assertion if bad i or j
	return arr3x4[i * 4 + j];
}
template<typename FT>
void CamMatrixT<FT>::make_zero() {
	memset(arr3x4, 0, 12 * sizeof(FT));
}
// this uses same coordinate system as crysis: the player looks down the positive y axis
template<typename FT>
void CamMatrixT<FT>::build_from_pos_and_lookdir(const Vec3refT<FT>& pos, const Vec3refT<FT>& lookdir) {
	for (int ii = 0; ii < 3; ++ii) {
		at(ii,3) = pos.atc(ii);
		at(ii,1) = lookdir.atc(ii);
	}
	GetCol(1).normalize();
	at(0,0) = atc(1,1);
	at(1,0) = -atc(0,1);
	at(2,0) = FT(0.0);
	GetCol(0).normalize();
	GetCol(0).cross(GetCol(1), GetCol(2)); // col2 = col0 x col1;
}
template<typename FT>
void CamMatrixT<FT>::build_from_pos_and_lookdir(Vec3T<FT> pos, Vec3T<FT> lookdir) {
	build_from_pos_and_lookdir(pos.ref(), lookdir.ref());
}

#define CAMMATBUFLEN 96
template<typename FT>
std::string CamMatrixT<FT>::serialize() {
	char cstrbuf[CAMMATBUFLEN*sizeof(FT)];
	memset(cstrbuf, 0, CAMMATBUFLEN*sizeof(FT));
	serialize_into(cstrbuf, CAMMATBUFLEN*sizeof(FT));
	return std::string(cstrbuf);
}

template<typename FT>
void Vec3refT<FT>::normalize() {
	const FT norm = std::max(std::numeric_limits<FT>::epsilon(), std::sqrt(x*x + y*y + z*z));
	x /= norm;
	y /= norm;
	z /= norm;
}
template<typename FT>
void Vec3T<FT>::normalize() {
	const FT norm = std::max(std::numeric_limits<FT>::epsilon(), std::sqrt(x*x + y*y + z*z));
	x /= norm;
	y /= norm;
	z /= norm;
}
template<typename FT>
FT Vec3T<FT>::dot(const Vec3T<FT>& other) const {
	return x*other.x + y*other.y + z*other.z;
}

template<typename FT>
void Vec3refT<FT>::cross(FT a, FT b, FT c, FT& out_x, FT& out_y, FT& out_z) const {
	out_x = y*c - z*b;
	out_y = z*a - x*c;
	out_z = x*b - y*a;
}
template<typename FT>
void Vec3refT<FT>::cross(const Vec3refT<FT>& other, FT& out_x, FT& out_y, FT& out_z) const {
	cross(other.x, other.y, other.z, out_x, out_y, out_z);
}
template<typename FT>
void Vec3refT<FT>::cross(const Vec3refT<FT>& other, Vec3refT<FT>& out) const {
	cross(other, out.x, out.y, out.z);
}
template<typename FT>
Vec3T<FT> Vec3T<FT>::cross(const Vec3T<FT>& other) const {
	return cross(other.x, other.y, other.z);
}
template<typename FT>
Vec3T<FT> Vec3T<FT>::cross(const Vec3refT<FT>& other) const {
	return cross(other.x, other.y, other.z);
}
template<typename FT>
Vec3T<FT> Vec3T<FT>::cross(FT a, FT b, FT c) const {
	return Vec3T<FT>(y*c - z*b,
		             z*a - x*c,
		             x*b - y*a);
}

template<typename FT>
void Vec4T<FT>::normalize() {
	const FT norm = std::max(std::numeric_limits<FT>::epsilon(), std::sqrt(x*x + y*y + z*z + t*t));
	x /= norm;
	y /= norm;
	z /= norm;
	t /= norm;
}
template<typename FT>
FT Vec4T<FT>::dot(const Vec4T<FT>& other) const {
	return x*other.x + y*other.y + z*other.z + t*other.t;
}

template<typename FT>
int serialize_several_vec4s(std::vector<Vec4T<FT> > &vecs, char *rbuf, int buflen) {
	if (rbuf == nullptr || buflen <= 1) return -1;
	rbuf[0] = '[';
	int thisn;
	int wpos = 1;
	for (size_t i = 0; i < vecs.size(); ++i) {
		if (wpos > 1) {
			rbuf[wpos] = ',';
			wpos += 1;
		}
		thisn = vecs[i].serialize_into(rbuf + wpos, buflen - wpos, false);
		if (thisn <= 0 || (wpos + thisn + 2) >= buflen) return -1;
		wpos += thisn;
	}
	if ((wpos + 1) >= buflen) return -1;
	rbuf[wpos] = ']';
	return wpos + 1;
}

template<typename FT>
int CamMatrixT<FT>::serialize_into(char *rbuf, int buflen) {
	if (rbuf == nullptr || buflen <= 1) return -1;
	std::vector<Vec4T<FT> > vecs;
	vecs.emplace_back(GetRow(0));
	vecs.emplace_back(GetRow(1));
	vecs.emplace_back(GetRow(2));
	return serialize_several_vec4s(vecs, rbuf, buflen);
}

// instantiate float and double classes
#define INSTANTIATEGEOMCLASS(clsnam) template struct clsnam <float>; template struct clsnam <double>

INSTANTIATEGEOMCLASS(Vec3refT);
INSTANTIATEGEOMCLASS(Vec4refT);
INSTANTIATEGEOMCLASS(Vec3T);
INSTANTIATEGEOMCLASS(Vec4T);
INSTANTIATEGEOMCLASS(CamMatrixT);
