// Copyright (C) 2022 Jason Bunk
#include "scan_for_camera_matrix.h"


bool AllMemScanner::iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, const void* scanctx, bool (*checkpossiblebuf)(const void* ctx, const uint8_t* buf, uint64_t nbytes)) {
	if (databuf.empty()) return false;
	SIZE_T nbytesread = 0;
	uint64_t bytescopiedoffset;
	uint64_t localblockend;
	uint64_t ii;
	const uint8_t* dptr;
	uint64_t llast;
	while (currmemloc < 0x7FFFFFFFFFFFull) {
		// check usability of memory region... skip large unused areas of memory
		if (!VirtualQueryEx(hProcess, (LPCVOID)(currmemloc), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) {
			localblockend = ((uint64_t)(mbi.BaseAddress)) + ((uint64_t)(mbi.RegionSize));
			if (currmemloc >= localblockend) {
				if (mbi.RegionSize == 0) {
					errstr = std::string("failed(?) VirtualQueryEx: ") + std::to_string(currmemloc) + std::string(" --> ") + std::to_string(localblockend) + std::string(", where mbi.BaseAddress ") + std::to_string((uint64_t)(mbi.BaseAddress)) + std::string(", mbi.RegionSize ") + std::to_string(mbi.RegionSize);
					return false;
				}
				// Somehow we asked to check memory region containing "currmemloc", yet it returns info about memory region (from baseaddr to baseaddr+regionsize) which doesnt contain "currmemloc".
				// Not sure what to do here; this might jump too far and miss relevant data. In practice I haven't seen it miss the camera matrix.
				currmemloc += ((uint64_t)(mbi.RegionSize));
			} else {
				currmemloc = localblockend;
			}
			continue;
		}
		if (mbi.RegionSize <= databuf.size()) {
			if (ReadProcessMemory(hProcess, (LPCVOID)(currmemloc), (LPVOID)(databuf.data()), mbi.RegionSize, &nbytesread) && nbytesread >= bufminlen) {
				llast = nbytesread - bufminlen;
				dptr = databuf.data();
				for (ii = 0; ii <= llast; ii += 4) {
					if ((!has_triggerbytes || *reinterpret_cast<const uint64_t*>(dptr + ii) == triggerbytes) && checkpossiblebuf(scanctx, dptr + ii, llast - ii)) {
						foundmemloc = currmemloc + ii;
						foundbuf = dptr + ii;
						foundbuflen = llast - ii;
						currmemloc += (ii + 8ull);
						return true;
					}
				}
			}
			currmemloc += mbi.RegionSize;
		} else {
			localblockend = ((uint64_t)(mbi.BaseAddress)) + ((uint64_t)(mbi.RegionSize));
			if (currmemloc >= localblockend) {
				errstr = std::string("failed(?) region walking: currmemloc ") + std::to_string(currmemloc) + std::string(" >= mbi.BaseAddress ") + std::to_string((uint64_t)(mbi.BaseAddress)) + std::string(" + mbi.RegionSize ") + std::to_string(mbi.RegionSize);
				return false;
			}
			bytescopiedoffset = 0;
			while (true) {
				if (ReadProcessMemory(hProcess, (LPCVOID)(currmemloc), (LPVOID)(databuf.data() + bytescopiedoffset), scanbufsize, &nbytesread) && nbytesread >= bufminlen) {
					llast = nbytesread + bytescopiedoffset - bufminlen;
					dptr = databuf.data();
					for (ii = 0; ii <= llast; ii += 4) {
						if ((!has_triggerbytes || *reinterpret_cast<const uint64_t*>(dptr + ii) == triggerbytes) && checkpossiblebuf(scanctx, dptr + ii, llast - ii)) {
							foundmemloc = currmemloc + ii - bytescopiedoffset;
							foundbuf = dptr + ii;
							foundbuflen = llast - ii;
							currmemloc = (currmemloc + ii + 8ull) - bytescopiedoffset;
							return true;
						}
					}
				}
				currmemloc += scanbufsize;
				if (currmemloc >= localblockend) {
					break;
				} else {
					if (nbytesread == 0) {
						bytescopiedoffset = 0;
					} else if(nbytesread <= bufminlen) {
						memmove(databuf.data(), databuf.data() + bytescopiedoffset, nbytesread);
						bytescopiedoffset = nbytesread;
					} else {
						memmove(databuf.data(), databuf.data() + bytescopiedoffset + nbytesread - bufminlen, bufminlen);
						bytescopiedoffset = bufminlen;
					}
				}
			}
		}
	}
	return false;
}
