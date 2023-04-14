// Copyright (C) 2022 Jason Bunk
#include "scan_for_camera_matrix.h"
#include <reshade.hpp>

void AllMemScanner::reset_iterator_to_beginning() {
	currmemloc = 0ull;
	mbi.BaseAddress = 0ull;
	mbi.RegionSize = 0ull;
}

static constexpr uint64_t sizeofscannablememory = 0x7FFFFFFFFFFFull;

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERTLOGTORESHADERETURNFALSE(x) if(!(x)){reshade::log_message(reshade::log_level::error, "scan assertion error: " __FILE__ " line " STRINGIZE(__LINE__)); currmemloc = sizeofscannablememory; return false;}


template<bool hastriggerbytes_t, bool fastscan_t>
bool AllMemScanner::impl_iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, const void* scanctx, bool (*checkpossiblebuf)(const void* ctx, const uint8_t* buf, uint64_t nbytes)) {
	ASSERTLOGTORESHADERETURNFALSE(!databuf.empty() && bufminlen > 8 && scanbufsize >= 8);
	SIZE_T queryregionsuccess = 0;
	uint64_t bytestoread;
	uint64_t bytescopiedoffset;
	uint64_t localblockend = ((uint64_t)(mbi.BaseAddress)) + ((uint64_t)(mbi.RegionSize));
	uint64_t ii;
	const uint8_t* dptr;
	uint64_t llast;
	while (currmemloc < sizeofscannablememory) {
		// check usability of memory region, every time we start at a block beginning (= the end of the last block)... skips large unused/unreadable areas of memory
		if (currmemloc == localblockend) {
			mbi.BaseAddress = 0ull;
			mbi.RegionSize = 0ull;
			queryregionsuccess = VirtualQueryEx(hProcess, (LPCVOID)(currmemloc), &mbi, sizeof(mbi));
			if (!queryregionsuccess) {
				// VirtualQueryEx fails when we reach (nearly) the end of the scannable memory space
				currmemloc = sizeofscannablememory;
				return false;
			}
			localblockend = ((uint64_t)(mbi.BaseAddress)) + ((uint64_t)(mbi.RegionSize));
			if (!queryregionsuccess || mbi.State != MEM_COMMIT || (fastscan_t ? (mbi.Protect != PAGE_READWRITE) : (mbi.Protect == PAGE_NOACCESS))) {
				ASSERTLOGTORESHADERETURNFALSE(currmemloc < localblockend);
				currmemloc = localblockend;
				continue;
			}
		}
		ASSERTLOGTORESHADERETURNFALSE(currmemloc < localblockend);
		if (mbi.RegionSize <= databuf.size()) {
			bytestoread = localblockend - currmemloc;
			ASSERTLOGTORESHADERETURNFALSE(bytestoread <= mbi.RegionSize);
			if(bytestoread >= bufminlen) {
				if (ReadProcessMemory(hProcess, (LPCVOID)(currmemloc), (LPVOID)(databuf.data()), bytestoread, nullptr)) {
					llast = bytestoread - bufminlen;
					dptr = databuf.data();
					for (ii = 0; ii <= llast; ii += (fastscan_t?4:1)) {
						if ((!hastriggerbytes_t || *reinterpret_cast<const uint64_t*>(dptr + ii) == triggerbytes) && checkpossiblebuf(scanctx, dptr + ii, bytestoread - ii)) {
							foundmemloc = currmemloc + ii;
							foundbuf = dptr + ii;
							foundbuflen = llast - ii;
							currmemloc = foundmemloc + 8ull;
							return true;
						}
					}
				}
			}
			currmemloc = localblockend;
		} else {
			bytescopiedoffset = 0ull;
			while (currmemloc < localblockend) {
				ASSERTLOGTORESHADERETURNFALSE(bytescopiedoffset < databuf.size());
				bytestoread = std::min(databuf.size() - bytescopiedoffset, localblockend - currmemloc);
				if ((bytescopiedoffset + bytestoread) >= bufminlen) {
					if(ReadProcessMemory(hProcess, (LPCVOID)(currmemloc), (LPVOID)(databuf.data() + bytescopiedoffset), bytestoread, nullptr)) {
						llast = bytescopiedoffset + bytestoread - bufminlen;
						dptr = databuf.data();
						for (ii = 0; ii <= llast; ii += (fastscan_t?4:1)) {
							if ((!hastriggerbytes_t || *reinterpret_cast<const uint64_t*>(dptr + ii) == triggerbytes) && checkpossiblebuf(scanctx, dptr + ii, bytescopiedoffset + bytestoread - ii)) {
								foundmemloc = (currmemloc + ii) - bytescopiedoffset;
								foundbuf = dptr + ii;
								foundbuflen = llast - ii;
								currmemloc = foundmemloc + 8ull;
								return true;
							}
						}
					}
				}
				currmemloc += bytestoread;
				ASSERTLOGTORESHADERETURNFALSE(currmemloc <= localblockend);
				if(currmemloc == localblockend) {
					break;
				}
				if(bytestoread <= bufminlen) {
					memmove(databuf.data(), databuf.data() + bytescopiedoffset, bytestoread);
					bytescopiedoffset = bytestoread;
				} else {
					memmove(databuf.data(), databuf.data() + bytescopiedoffset + bytestoread - bufminlen, bufminlen);
					bytescopiedoffset = bufminlen;
				}
			}
		}
	}
	return false;
}

bool AllMemScanner::iterate_next(uint64_t& foundmemloc, const uint8_t*& foundbuf, uint64_t& foundbuflen, const void* scanctx, bool (*checkpossiblebuf)(const void* ctx, const uint8_t* buf, uint64_t nbytes)) {
    if (has_triggerbytes)
        return fastscan ? impl_iterate_next<true, true>(foundmemloc, foundbuf, foundbuflen, scanctx, checkpossiblebuf)
                        : impl_iterate_next<true,false>(foundmemloc, foundbuf, foundbuflen, scanctx, checkpossiblebuf);
    return fastscan ? impl_iterate_next<false, true>(foundmemloc, foundbuf, foundbuflen, scanctx, checkpossiblebuf)
                    : impl_iterate_next<false,false>(foundmemloc, foundbuf, foundbuflen, scanctx, checkpossiblebuf);
}
