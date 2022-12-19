// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/memread.h"

bool tryreadmemory(std::string contextstr, std::string &returnederrstr, HANDLE hProcess,
			void *readfromherememloc, LPVOID returnwriteintobuf, SIZE_T bytes2read, SIZE_T *bytesread)
{
	if (hProcess == 0) {
		returnederrstr = contextstr + std::string("tryreadmemory: hProcess == 0");
		return false;
	}
	if (readfromherememloc == nullptr) {
		returnederrstr = contextstr + std::string("tryreadmemory: readfromherememloc == null");
		return false;
	}
	if (returnwriteintobuf == 0 || bytes2read == 0) {
		returnederrstr = contextstr + std::string("tryreadmemory: buf empty or no bytes to read");
		return false;
	}
	if (bytesread == nullptr) {
		returnederrstr = contextstr + std::string("tryreadmemory: bytesread == null");
		return false;
	}
	if (ReadProcessMemory(hProcess, (void *)(readfromherememloc), returnwriteintobuf, bytes2read, bytesread)) {
		if (*bytesread == bytes2read) {
			return true;
		}
		else {
			returnederrstr = contextstr + std::string("failed to read ") + std::to_string((int)bytes2read) + std::string(" bytes, instead read ") + std::to_string((int)(*bytesread)) +
				std::string(" bytes from process memory: ") + std::to_string(((uint64_t)(hProcess))) + std::string(", memloc ") + std::to_string(((uint64_t)(readfromherememloc)))
				+ std::string(", err ") + std::to_string((int64_t)(GetLastError()));
			return false;
		}
	}
	returnederrstr = contextstr + std::string("failed to read process memory: ") + std::to_string(((uint64_t)(hProcess))) + std::string(", memloc ")
		+ std::to_string(((uint64_t)(readfromherememloc))) + std::string(", err ") + std::to_string((int64_t)(GetLastError()));
	return false;
}
