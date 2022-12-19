#pragma once
// Copyright (C) 2022 Jason Bunk
#include <vector>
#include <string>

#define defaultunkfloatinit -9.0f

struct Vec4ref {
	float &x, &y, &z, &t;
	Vec4ref(float &x_, float &y_, float &z_, float &t_);
	int serialize_into(char *rbuf, int buflen, bool wrapbrackets);
};

struct Vec3ref {
	float &x, &y, &z;
	Vec3ref(float &x_, float &y_, float &z_);
};

struct CamMatrix {
	float arr3x4[12];
	CamMatrix();
	CamMatrix(float *rawptr3x4);
	Vec3ref GetCol(int i);
	Vec4ref GetRow(int i);
	Vec3ref GetPosition();
	void make_zero();
	int serialize_into(char *rbuf, int buflen);
	std::string serialize();
};

struct Vec4 {
	float x, y, z, t;
	Vec4();
	Vec4(float x_, float y_, float z_, float t_);
	Vec4(const Vec4ref &other);
	int serialize_into(char *rbuf, int buflen, bool wrapbrackets);
};

int serialize_several_vec4s(std::vector<Vec4> &vecs, char *rbuf, int buflen);
