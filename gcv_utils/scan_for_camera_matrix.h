#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Windows.h>
#include <string>
#include <vector>


class AllMemScanner {
	static constexpr uint64_t scanbufsize = 16384ull;
	MEMORY_BASIC_INFORMATION mbi;
	uint64_t currmemloc;
	uint64_t bufminlen;
	uint64_t triggerbytes;
	HANDLE hProcess;
	HMODULE hCamDLL;
	std::string& errstr;
	std::vector<uint8_t> databuf;
public:
	AllMemScanner() = delete;
	AllMemScanner(HANDLE hProcHandle, uint64_t wantbufminlength, std::string& errorstr, uint64_t triggerpattern)
		: currmemloc(8ull), bufminlen(wantbufminlength), triggerbytes(triggerpattern), hProcess(hProcHandle), errstr(errorstr) {
		if (hProcess == 0) {
			errstr += "AllMemScanner: error: hProcess == 0";
		} else {
			databuf.resize(scanbufsize + wantbufminlength);
		}
	}
	bool iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, bool (*checkpossiblebuf)(const uint8_t* buf, uint64_t nbytes));
};
