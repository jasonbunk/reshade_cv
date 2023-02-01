#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Windows.h>
#include <string>
#include <vector>


class AllMemScanner {
	static constexpr uint64_t scanbufsize = 1000000ull;
	MEMORY_BASIC_INFORMATION mbi;
	uint64_t currmemloc;
	uint64_t bufminlen;
	uint64_t triggerbytes;
	bool has_triggerbytes;
	HANDLE hProcess;
	HMODULE hCamDLL;
	std::string& errstr;
	std::vector<uint8_t> databuf;

	template<bool hastriggerbytes_t, bool fastscan_t>
	bool impl_iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, const void* scanctx, bool (*checkpossiblebuf)(const void* ctx, const uint8_t* buf, uint64_t nbytes));
public:
	bool fastscan = true;
	AllMemScanner() = delete;
	AllMemScanner(HANDLE hProcHandle, uint64_t wantbufminlength, std::string& errorstr, uint64_t triggerpattern, bool has_triggerpattern)
		: currmemloc(0ull), bufminlen(wantbufminlength), triggerbytes(triggerpattern), has_triggerbytes(has_triggerpattern), hProcess(hProcHandle), errstr(errorstr) {
		if (hProcess == 0) {
			errstr += "AllMemScanner: error: hProcess == 0";
		} else {
			databuf.resize(scanbufsize + wantbufminlength);
		}
		reset_iterator_to_beginning();
	}
	void reset_iterator_to_beginning();
	bool iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, const void* scanctx, bool (*checkpossiblebuf)(const void* ctx, const uint8_t* buf, uint64_t nbytes));
};
