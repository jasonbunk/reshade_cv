// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/simple_packed_buf.h"

size_t simple_packed_buf::bytes_per_pixel() const {
	switch (pixfmt) {
	case BUF_PIX_FMT_RGB24: return 3;
	case BUF_PIX_FMT_RGBA: return 4;
	case BUF_PIX_FMT_GRAYU32: return 4;
	case BUF_PIX_FMT_GRAYF32: return sizeof(float);
	}
	// TODO: raise error!
	return 0;
}

size_t simple_packed_buf::rowstride_bytes() const {
	return width * bytes_per_pixel();
}

bool simple_packed_buf::set_pixfmt_and_alloc_bytes(BufPixelFormat pixfmt_) {
	pixfmt = pixfmt_;
	size_t bytesperpix = bytes_per_pixel();
	if (bytesperpix == 0) {
		bytes.clear();
		return false;
	}
	bytes.resize(width * height * bytesperpix);
	return true;
}

bool simple_packed_buf::init_full(size_t width_, size_t height_, BufPixelFormat pixfmt_) {
	width = width_;
	height = height_;
	return set_pixfmt_and_alloc_bytes(pixfmt_);
}
