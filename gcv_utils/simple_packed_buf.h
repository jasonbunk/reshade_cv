#pragma once
// Copyright (C) 2022 Jason Bunk
#include <vector>
#include <string>

enum BufPixelFormat {
	BUF_PIX_FMT_NONE,
	BUF_PIX_FMT_RGB24,
	BUF_PIX_FMT_RGBA,
	BUF_PIX_FMT_GRAYF32,
	BUF_PIX_FMT_GRAYU32,
};

// row accessors assume data is row-major
// otherwise this is a simple vector<uint8_t> intended for images
struct simple_packed_buf {
	BufPixelFormat pixfmt = BUF_PIX_FMT_NONE;
	size_t width = 0;
	size_t height = 0;
	std::vector<uint8_t> bytes;

	size_t rowstride_bytes() const; // number of bytes from one row to the next
	size_t bytes_per_pixel() const;
	size_t num_total_bytes() const;

	bool init_full(size_t width_, size_t height_, BufPixelFormat pixfmt_);
	bool set_pixfmt_and_alloc_bytes(BufPixelFormat pixfmt_);

	template<typename T> T *data() {
		return reinterpret_cast<T *>(bytes.data());
	}
	template<typename T> T *rowptr(size_t row) {
		if (row >= height || pixfmt == BUF_PIX_FMT_NONE) return nullptr;
		return reinterpret_cast<T *>(bytes.data() + rowstride_bytes() * row);
	}
	template<typename T> T *entryptr(size_t row, size_t col) {
		if (col >= width || row >= height || pixfmt == BUF_PIX_FMT_NONE) return nullptr;
		return reinterpret_cast<T *>(bytes.data() + bytes_per_pixel() * (width * row + col));
	}

	template<typename T> const T *cdata() const {
		return reinterpret_cast<const T *>(bytes.data());
	}
	template<typename T> const T *crowptr(size_t row) const {
		if (row >= height || pixfmt == BUF_PIX_FMT_NONE) return nullptr;
		return reinterpret_cast<const T *>(bytes.data() + rowstride_bytes() * row);
	}
	template<typename T> const T *centryptr(size_t row, size_t col) const {
		if (col >= width || row >= height || pixfmt == BUF_PIX_FMT_NONE) return nullptr;
		return reinterpret_cast<const T *>(bytes.data() + bytes_per_pixel() * (width * row + col));
	}
};
