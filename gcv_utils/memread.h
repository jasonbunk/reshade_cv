#pragma once
// Copyright (C) 2022 Jason Bunk
#include <Windows.h>
#include <string>

bool tryreadmemory(std::string contextstr, std::string &returnederrstr, HANDLE hProcess,
				void *readfromherememloc, LPVOID returnwriteintobuf, SIZE_T bytes2read, SIZE_T *bytesread);
