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
	std::tm *const pTInfo = std::localtime(&currtime);
	char tmpbuf[16];
	sprintf(tmpbuf, "%d-%02d-%02d", 1900 + pTInfo->tm_year, pTInfo->tm_mon, pTInfo->tm_wday);
	return std::string(tmpbuf);
}

#define hertsttbufsz 1023

std::string run_utils_tests() {
	//std::vector<std::pair<std::string, std::string> > teststrings = { {"/hello/x", "x"}, {"/hello/", ""}, {"/hello", "hello"}, {"/", ""} };
	if (std::string().length() > 0) return "A: empty string not empty!?";
	if (std::string("").length() > 0) return "B: empty string not empty!?";
	if (basename_from_filepath("/hello/x").compare("x")) return "basename_from_filepath";
	char tmpbuf[hertsttbufsz];

	memset(tmpbuf, 0, hertsttbufsz);
	Vec4().serialize_into(tmpbuf, hertsttbufsz - 1, true);
	if (std::string(tmpbuf).compare("[-9.0000000,-9.0000000,-9.0000000,-9.0000000]")) return std::string("Vec4().serialize_into(...,true) -- <") + std::string(tmpbuf) + std::string(">");

	memset(tmpbuf, 0, hertsttbufsz);
	Vec4().serialize_into(tmpbuf, hertsttbufsz - 1, false);
	if (std::string(tmpbuf).compare("-9.0000000,-9.0000000,-9.0000000,-9.0000000")) return std::string("Vec4().serialize_into(...,false) -- <") + std::string(tmpbuf) + std::string(">");

	memset(tmpbuf, 0, hertsttbufsz);
	CamMatrix().serialize_into(tmpbuf, hertsttbufsz - 1);
	if (std::string(tmpbuf).compare("[-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000,-9.0000000]")) return std::string("Vec4().serialize_into(...,false) -- <") + std::string(tmpbuf) + std::string(">");

	return std::string("ok");
}
