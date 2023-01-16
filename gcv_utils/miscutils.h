#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Windows.h>
#include <string>
#include <sstream>

std::string string_from_wstring(const std::wstring &input);
std::wstring wstring_from_string(const std::string& str);
std::string string_from_TCHARptr(TCHAR *wcharptr);
std::string string_lowercase(const std::string &input);
std::string pathofcurrentprocessexe();
std::string basename_from_filepath(const std::string &input);
std::string lowercasenameofcurrentprocessexe();

std::string get_datestr_yyyy_mm_dd();

// return error string if test failed; empty string means ok
std::string run_utils_tests();

// https://stackoverflow.com/questions/4915462/how-should-i-do-floating-point-comparison
template<typename FT>
bool floats_nearly_equal(FT a, FT b, FT epsmult = FT(128)) {
	if (a == b) return true;
	const FT diff = std::abs(a - b);
	const FT norm = std::min((std::abs(a) + std::abs(b)), std::numeric_limits<FT>::max());
	return diff < std::max(std::numeric_limits<FT>::min(), (epsmult * std::numeric_limits<FT>::epsilon()) * norm);
}
