#pragma once
// Copyright (C) 2023 Jason Bunk
#include <vector>

// row-major 2d array
template<typename T>
struct typed_2d_array {
	size_t width = 0;
	size_t height = 0;
	std::vector<T> data;

	// number of bytes from one row to the next
	inline size_t rowstride_bytes() const { return width * sizeof(T); }
	inline size_t bytes_per_pixel() const { return sizeof(T); }
	inline size_t num_total_bytes() const { return height * width * sizeof(T); }

	inline void init(size_t width_, size_t height_) {
		width = width_;
		height = height_;
		data.resize(width * height);
	}

	inline T *rowptr(size_t row) {
		if (row >= height) return nullptr;
		return &data[width * row];
	}
	inline T *entryptr(size_t row, size_t col) {
		if (col >= width || row >= height) return nullptr;
		return &data[width * row + col];
	}

	inline const T* crowptr(size_t row) const {
		if (row >= height) return nullptr;
		return &data[width * row];
	}
	inline const T* centryptr(size_t row, size_t col) const {
		if (col >= width || row >= height) return nullptr;
		return &data[width * row + col];
	}

};
