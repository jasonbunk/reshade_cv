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

template<typename AnyT>
std::string to_string(const AnyT& value) {
	std::stringstream strstr;
	strstr << value;
	return strstr.str();
}
