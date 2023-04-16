// Copyright (C) 2023 Jason Bunk
#pragma once
#include <array>

typedef std::array<uint64_t, 3> perdraw_metadata_type; // (#vertices, vertexshaderhash, pixelshaderhash)
constexpr size_t perdraw_metadata_type_size_bytes = sizeof(uint64_t) * 3ull;

typedef uint64_t shader_hash_t; // to reduce possibility of shader hash collision, consider a 128-bit hash
