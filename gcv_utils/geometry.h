#pragma once
// Copyright (C) 2022 Jason Bunk
#include <vector>
#include <cmath>
#include <string>

#define defaultunkfloatinit -9.0

template<typename FT>
struct Vec3refT {
	FT &x, &y, &z;
	Vec3refT(FT& x_, FT& y_, FT& z_) : x(x_), y(y_), z(z_) {}
	FT& at(int which) { if (which <= 0) return x; if (which >= 2) return z; return y; }
	FT atc(int which) const { if (which <= 0) return x; if (which >= 2) return z; return y; }
	void operator=(const Vec3refT<FT>& other) { x = other.x; y = other.y; z = other.z; }
	//Vec3T<FT> clone() const { return Vec3T<FT>(x, y, z); }
	void cross(const Vec3refT<FT>& other, Vec3refT<FT>& out) const;
	void cross(const Vec3refT<FT>& other, FT& out_x, FT& out_y, FT& out_z) const;
	void cross(FT a, FT b, FT c, FT& out_x, FT& out_y, FT& out_z) const;
	void normalize();
	FT normlength() const { return std::sqrt(x*x + y*y + z*z); }
	std::string to_string(bool wrap_brackets = true) const;
};

template<typename FT>
struct Vec4refT {
	FT &x, &y, &z, &t;
	Vec4refT(FT &x_, FT &y_, FT &z_, FT &t_) : x(x_), y(y_), z(z_), t(t_) {}
	FT& at(int which) { if (which <= 0) return x; if (which >= 3) return t; return which == 1 ? y : z; }
	FT atc(int which) const { if (which <= 0) return x; if (which >= 3) return t; return which == 1 ? y : z; }
	int serialize_into(char *rbuf, int buflen, bool wrapbrackets) const;
	std::string to_string(bool wrap_brackets) const;
	//Vec4T<FT> clone() const { return Vec4T<FT>(x, y, z, t); }
};

template<typename FT>
struct Vec3T {
	FT x, y, z;
	Vec3T() : x(defaultunkfloatinit), y(defaultunkfloatinit), z(defaultunkfloatinit) {}
	Vec3T(FT x_, FT y_, FT z_) : x(x_), y(y_), z(z_) {}
	Vec3T(const Vec4refT<FT>& other) : x(other.x), y(other.y), z(other.z) {}
	FT& at(int which) { if (which <= 0) return x; if (which >= 2) return z; return y; }
	Vec3refT<FT> ref() { return Vec3refT<FT>(x, y, z); }
	FT normlength() const { return std::sqrt(x*x + y*y + z*z); }
	void normalize();
	FT dot(const Vec3T<FT>& other) const;
	Vec3T<FT> cross(const Vec3T<FT>& other) const;
	Vec3T<FT> cross(const Vec3refT<FT>& other) const;
	Vec3T<FT> cross(FT a, FT b, FT c) const;
	std::string to_string(bool wrap_brackets = true) const;
};

template<typename FT>
struct Vec4T {
	FT x, y, z, t;
	Vec4T() : x(defaultunkfloatinit), y(defaultunkfloatinit), z(defaultunkfloatinit), t(defaultunkfloatinit) {}
	Vec4T(FT x_, FT y_, FT z_, FT t_) : x(x_), y(y_), z(z_), t(t_) {}
	Vec4T(const Vec4refT<FT>& other) : x(other.x), y(other.y), z(other.z), t(other.t) {}
	Vec4refT<FT> ref() { return Vec4refT<FT>(x, y, z, t); }
	std::vector<FT> as_vector() const;
	FT& at(int i);
	void normalize();
	FT dot(const Vec4T<FT>& other) const;
	int serialize_into(char* rbuf, int buflen, bool wrapbrackets) const;
	std::string to_string(bool wrap_brackets) const;
};

template<typename FT>
struct CamMatrixT {
	FT arr3x4[12];
	CamMatrixT();
	CamMatrixT(const float* rawptr3x4);
	CamMatrixT(const double* rawptr3x4);
	Vec3refT<FT> GetCol(int i);
	Vec4refT<FT> GetRow(int i);
	Vec4T<FT> GetRowCopy(int i) const;
	Vec3refT<FT> GetPosition() { return GetCol(3); }
	FT& at(int i, int j);
	FT atc(int i, int j) const;
	void make_zero();
	void build_from_pos_and_lookdir(const Vec3refT<FT>& pos, const Vec3refT<FT>& lookdir);
	void build_from_pos_and_lookdir(Vec3T<FT> pos, Vec3T<FT> lookdir);
	int serialize_into(char *rbuf, int buflen) const;
	std::string serialize() const;
};

template<typename FT>
int serialize_several_vec4s(std::vector<Vec4T<FT> > &vecs, char *rbuf, int buflen);

typedef double ftype;
typedef CamMatrixT<ftype> CamMatrix;
typedef Vec4refT<ftype> Vec4ref;
typedef Vec3refT<ftype> Vec3ref;
typedef Vec4T<ftype> Vec4;
typedef Vec3T<ftype> Vec3;
