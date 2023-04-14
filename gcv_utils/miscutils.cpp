// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/miscutils.h"
#include "gcv_utils/geometry.h"
#include <locale>
#include <codecvt>
#include <algorithm>

// https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
std::string string_from_wstring(const std::wstring &wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}
std::wstring wstring_from_string(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string string_from_TCHARptr(TCHAR *wcharptr) {
	if (sizeof(TCHAR) == 1) {
		return std::string(reinterpret_cast<char *>(wcharptr));
	}
	return string_from_wstring(std::wstring(reinterpret_cast<wchar_t *>(wcharptr)));
}

std::string string_lowercase(const std::string &input) {
	std::string copyofin(input);
	std::transform(copyofin.begin(), copyofin.end(), copyofin.begin(), [](unsigned char c) { return std::tolower(c); });
	return copyofin;
}

std::string pathofcurrentprocessexe() {
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	return string_from_TCHARptr(szFileName);
}

std::string basename_from_filepath(const std::string &input) {
	size_t lfwsl = input.find_last_of('/');
	size_t lbwsl = input.find_last_of('\\');
	if (lfwsl == std::string::npos && lbwsl == std::string::npos) {
		return input;
	}
	lfwsl = std::max(lfwsl, lbwsl);
	if ((lfwsl + 1) >= input.length()) {
		return std::string();
	}
	return input.substr(input.find_last_of("/\\") + 1);
}

std::string lowercasenameofcurrentprocessexe() {
	return string_lowercase(basename_from_filepath(pathofcurrentprocessexe()));
}

std::string get_datestr_yyyy_mm_dd() {
	std::time_t currtime = std::time(nullptr);
	std::tm gottime;
	localtime_s(&gottime, &currtime);
	char tmpbuf[20];
	sprintf_s(tmpbuf, 20, "%d-%02d-%02d", 1900 + gottime.tm_year, 1 + gottime.tm_mon, gottime.tm_mday);
	return std::string(tmpbuf);
}

#define hertsttbufsz 1023
#define RETURNFAILST(xx) return std::string("failed: ") + xx

template<typename FT>
bool vec3_near(const Vec3& aa, FT bx, FT by, FT bz, FT threshold) {
	return std::abs(aa(0) - bx) < threshold
		&& std::abs(aa(1) - by) < threshold
		&& std::abs(aa(2) - bz) < threshold;
}

#define CHECKVECNEAR(pstr, refvec, expectx, expecty, expectz) if(!vec3_near<double>(refvec, expectx, expecty, expectz, 0.00001)){ RETURNFAILST(pstr)+std::string(":\n")+to_string(refvec)+std::string("\nversus\n")+to_string(Vec3(expectx, expecty, expectz)); }

std::string run_utils_tests() {
	//std::vector<std::pair<std::string, std::string> > teststrings = { {"/hello/x", "x"}, {"/hello/", ""}, {"/hello", "hello"}, {"/", ""} };
	if (std::string().length() > 0) RETURNFAILST("A: empty string not empty!?");
	if (std::string("").length() > 0) RETURNFAILST("B: empty string not empty!?");
	if (basename_from_filepath("/hello/x").compare("x")) RETURNFAILST("basename_from_filepath");
	
	{const Vec3 testcross = Vec3( 5, 3, 2).cross(Vec3( 11, 7, 13)); CHECKVECNEAR("Vec3::cross test1", testcross, 25.0,-43.0, 2.0)}
	{const Vec3 testcross = Vec3(-5, 3,-2).cross(Vec3( 11,-7, 13)); CHECKVECNEAR("Vec3::cross test2", testcross, 25.0, 43.0, 2.0)}
	{const Vec3 testcross = Vec3(-5, 3,-2).cross(Vec3( 11,-7,-13)); CHECKVECNEAR("Vec3::cross test3", testcross,-53.0,-87.0, 2.0)}
	{const Vec3 testcross = Vec3(-5, 3,-2).cross(Vec3(-11,-7,-13)); CHECKVECNEAR("Vec4::cross test4", testcross,-53.0,-43.0,68.0)}

	const Vec3 testcampos = Vec3(5, 2, 3);
	{
		const Vec3 testlookdir = Vec3(-0.398519, 0.475435, -0.784311); CamMatrix testlookcam = build_cam_matrix_from_pos_and_lookdir(testcampos, testlookdir);
		CHECKVECNEAR("lookcam1_col0", testlookcam.col(0), 0.766376, 0.642392, 0.0) CHECKVECNEAR("lookcam1_col1", testlookcam.col(1), testlookdir(0), testlookdir(1), testlookdir(2)) CHECKVECNEAR("lookcam1_col2", testlookcam.col(2), -0.503835, 0.601077, 0.620368) CHECKVECNEAR("lookcam1_col3", testlookcam.col(3), 5.0, 2.0, 3.0)
	} {
		const Vec3 testlookdir = Vec3(0.422797, -0.901974, -0.087669); CamMatrix testlookcam = build_cam_matrix_from_pos_and_lookdir(testcampos, testlookdir);
		CHECKVECNEAR("lookcam2_col0", testlookcam.col(0), -0.905460, -0.424431, 0.0) CHECKVECNEAR("lookcam2_col1", testlookcam.col(1), testlookdir(0), testlookdir(1), testlookdir(2)) CHECKVECNEAR("lookcam2_col2", testlookcam.col(2), 0.037209, -0.079381, 0.996150) CHECKVECNEAR("lookcam2_col3", testlookcam.col(3), 5.0, 2.0, 3.0)
	} {
		const Vec3 testlookdir = Vec3(0.758219, 0.605454, 0.241927); CamMatrix testlookcam = build_cam_matrix_from_pos_and_lookdir(testcampos, testlookdir);
		CHECKVECNEAR("lookcam3_col0", testlookcam.col(0), 0.623990, -0.781432, 0.0) CHECKVECNEAR("lookcam3_col1", testlookcam.col(1), testlookdir(0), testlookdir(1), testlookdir(2)) CHECKVECNEAR("lookcam3_col2", testlookcam.col(2), -0.189050, -0.150960, 0.970294) CHECKVECNEAR("lookcam3_col3", testlookcam.col(3), 5.0, 2.0, 3.0)
	} {
		const Vec3 testlookdir = Vec3(-0.441620, -0.853757, 0.275811); CamMatrix testlookcam = build_cam_matrix_from_pos_and_lookdir(testcampos, testlookdir);
		CHECKVECNEAR("lookcam4_col0", testlookcam.col(0), -0.888209, 0.459441, 0.0) CHECKVECNEAR("lookcam4_col1", testlookcam.col(1), testlookdir(0), testlookdir(1), testlookdir(2)) CHECKVECNEAR("lookcam4_col2", testlookcam.col(2), 0.126719, 0.244978, 0.961212) CHECKVECNEAR("lookcam4_col3", testlookcam.col(3), 5.0, 2.0, 3.0)
	}
	return std::string("ok");
}
