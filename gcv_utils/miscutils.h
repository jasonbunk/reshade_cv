#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Windows.h>
#include <string>
#include <sstream>

std::string string_from_wstring(const std::wstring &input);
std::string string_from_TCHARptr(TCHAR *wcharptr);
std::string string_lowercase(const std::string &input);
std::string pathofcurrentprocessexe();
std::string basename_from_filepath(const std::string &input);
std::string lowercasenameofcurrentprocessexe();

std::string get_datestr_yyyy_mm_dd();

// return error string if test failed; empty string means ok
std::string run_utils_tests();

template<typename T>
std::string print_lots_of_items(T const*const input, size_t nelems, size_t batchsize) {
	if (input == nullptr || nelems == 0) return "";
	std::stringstream strstr;
	for (size_t ii = 0; ii <= (nelems / batchsize); ++ii) {
		if (ii > 0) strstr << "; ";
		strstr << "[";
		for (size_t jj = ii * batchsize; jj < std::min((ii + 1) * batchsize, nelems); ++jj) {
			if (jj > 0) strstr << ", ";
			strstr << input[jj];
		}
		strstr << "]";
	}
	return strstr.str();
}
