// Copyright (C) 2022 Jason Bunk
#include "geometry.h"

Vec3ref::Vec3ref(float &x_, float &y_, float &z_) : x(x_), y(y_), z(z_) {}

Vec4::Vec4() : x(defaultunkfloatinit), y(defaultunkfloatinit), z(defaultunkfloatinit), t(defaultunkfloatinit) {}
Vec4::Vec4(float x_, float y_, float z_, float t_) : x(x_), y(y_), z(z_), t(t_) {}
Vec4::Vec4(const Vec4ref &other) : x(other.x), y(other.y), z(other.z), t(other.t) {}

int Vec4::serialize_into(char *rbuf, int buflen, bool wrapbrackets) {
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
Vec4ref::Vec4ref(float &x_, float &y_, float &z_, float &t_) : x(x_), y(y_), z(z_), t(t_) {}
int Vec4ref::serialize_into(char *rbuf, int buflen, bool wrapbrackets) {
	if (rbuf == nullptr || buflen <= 1) return -1;
	return Vec4(x, y, z, t).serialize_into(rbuf, buflen, wrapbrackets);
}

CamMatrix::CamMatrix() {
	for (int ii = 0; ii < 12; ++ii) arr3x4[ii] = defaultunkfloatinit;
}
CamMatrix::CamMatrix(float *rawptr3x4) {
	for (int ii = 0; ii < 12; ++ii) arr3x4[ii] = rawptr3x4[ii];
}
Vec4ref CamMatrix::GetRow(int i) {
	// TODO: assertion if bad i
	return Vec4ref(arr3x4[i * 4 + 0],
				   arr3x4[i * 4 + 1],
				   arr3x4[i * 4 + 2],
				   arr3x4[i * 4 + 3]);
}
Vec3ref CamMatrix::GetCol(int i) {
	// TODO: assertion if bad i
	return Vec3ref(arr3x4[i], arr3x4[4 + i], arr3x4[8 + i]);
}
Vec3ref CamMatrix::GetPosition() {
	return GetCol(3);
}
void CamMatrix::make_zero() {
	memset(arr3x4, 0, 12 * sizeof(float));
}
#define CAMMATBUFLEN 384
std::string CamMatrix::serialize() {
	char cstrbuf[CAMMATBUFLEN];
	memset(cstrbuf, 0, CAMMATBUFLEN);
	serialize_into(cstrbuf, CAMMATBUFLEN);
	return std::string(cstrbuf);
}

int serialize_several_vec4s(std::vector<Vec4> &vecs, char *rbuf, int buflen) {
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

int CamMatrix::serialize_into(char *rbuf, int buflen) {
	if (rbuf == nullptr || buflen <= 1) return -1;
	std::vector<Vec4> vecs;
	vecs.push_back(GetRow(0));
	vecs.push_back(GetRow(1));
	vecs.push_back(GetRow(2));
	return serialize_several_vec4s(vecs, rbuf, buflen);
}
