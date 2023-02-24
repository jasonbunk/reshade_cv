// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/image_queue_entry.h"
#include <cnpy.h>
#include <fpzip/fpzip.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <queue>

#define RobustNth 50

template<typename FT>
bool pack_32bitgray_into_8bitrgb(const simple_packed_buf& srcBuf, simple_packed_buf & dstBuf) {
	if (!dstBuf.init_full(srcBuf.width, srcBuf.height, BUF_PIX_FMT_RGB24)) return false;
	int ii, jj;
	const FT* rowsrc;
	uint8_t* rowdst;
	std::priority_queue<FT, std::vector<FT>, std::greater<FT> > kthLargest;
	std::priority_queue<FT, std::vector<FT>, std::less<FT> > kthSmallest;
	for (ii = 0; ii < srcBuf.height; ++ii) {
		rowsrc = srcBuf.crowptr<FT>(ii);
		for (jj = 0; jj < srcBuf.width; ++jj) {
			if (kthLargest.size() < RobustNth) {
				kthLargest.push(rowsrc[jj]);
				kthSmallest.push(rowsrc[jj]);
			}
			else {
				if (rowsrc[jj] > kthLargest.top()) {
					kthLargest.pop();
					kthLargest.push(rowsrc[jj]);
				}
				if (rowsrc[jj] < kthSmallest.top()) {
					kthSmallest.pop();
					kthSmallest.push(rowsrc[jj]);
				}
			}
		}
	}
	const double fmax = static_cast<double>(kthLargest.top());
	const double fmin = static_cast<double>(kthSmallest.top());
	const double frescale = 255.0 / std::max(0.000000000001, fmax - fmin);
	double dblval;
	uint8_t thiscolor;
	for (ii = 0; ii < srcBuf.height; ++ii) {
		rowsrc = srcBuf.crowptr<FT>(ii);
		rowdst = dstBuf.rowptr<uint8_t>(ii);
		for (jj = 0; jj < srcBuf.width; ++jj) {
			dblval = static_cast<double>(rowsrc[jj]);
			thiscolor = std::clamp(std::lround((dblval - fmin) * frescale), 0l, 255l);
			rowdst[jj * 3] = thiscolor;
			rowdst[jj * 3 + 1] = thiscolor;
			rowdst[jj * 3 + 2] = thiscolor;
		}
	}
	return true;
}

bool save_packedbuf_as_8bit_png_image(const std::string &filepath,
	const simple_packed_buf &srcBuf, std::string &errstr)
{
	if (srcBuf.pixfmt == BUF_PIX_FMT_RGB24 || srcBuf.pixfmt == BUF_PIX_FMT_RGBA) {
		return stbi_write_png(filepath.c_str(), srcBuf.width, srcBuf.height,
			srcBuf.bytes_per_pixel(), srcBuf.cdata<uint8_t>(), srcBuf.rowstride_bytes()) != 0;
	}
	simple_packed_buf dstBuf;
	if (srcBuf.pixfmt == BUF_PIX_FMT_GRAYF32) {
		if (!pack_32bitgray_into_8bitrgb<float>(srcBuf, dstBuf)) return false;
	} else if(srcBuf.pixfmt == BUF_PIX_FMT_GRAYU32) {
		if (!pack_32bitgray_into_8bitrgb<uint32_t>(srcBuf, dstBuf)) return false;
	} else {
		errstr += std::string("save_8bitpng: unrecognized buf format ") + std::to_string(srcBuf.pixfmt);
		return false;
	}
	return stbi_write_png(filepath.c_str(), dstBuf.width, dstBuf.height, 3, dstBuf.data<uint8_t>(), dstBuf.rowstride_bytes()) != 0;
}

bool save_packedbuf_f32_using_fpzip(const std::string &filepath,
	const simple_packed_buf &srcBuf, std::string &errstr) {
	if (srcBuf.pixfmt != BUF_PIX_FMT_GRAYF32) {
		errstr += std::string("fpzip: only writes floating point data; refusing ")
			+ filepath + std::string(" of type ") + std::to_string(srcBuf.pixfmt);
		return false;
	}
	FILE *file = fopen(filepath.c_str(), "wb");
	if (!file) {
		errstr += std::string("fpzip: failed to open file ") + filepath;
		return false;
	}
	FPZ *fpz = fpzip_write_to_file(file);
	fpz->type = 0;
	fpz->prec = 0;
	fpz->nx = srcBuf.width;
	fpz->ny = srcBuf.height;
	fpz->nz = 1;
	fpz->nf = 1;
	if (!fpzip_write_header(fpz)) {
		errstr += std::string("fpzip: cannot write header: ") + std::string(fpzip_errstr[fpzip_errno]);
		return false;
	}
	if (!fpzip_write(fpz, srcBuf.cdata<void>())) {
		errstr += std::string("fpzip: compression failed when writing ") + filepath
			+ std::string(": ") + std::string(fpzip_errstr[fpzip_errno]);
		return false;
	}
	fpzip_write_close(fpz);
	fclose(file);
	return true;
}

bool queue_item_image2write::write_to_disk(std::string &errstr) const {
	if (writers == ImageWriter_none || writers >= ImageWriter_end) return false;
	bool allgood = true;
	if (writers & ImageWriter_STB_png) {
		allgood &= save_packedbuf_as_8bit_png_image(filepath_noexten + std::string(".png"), mybuf, errstr);
	}
	if (writers & ImageWriter_numpy) {
		switch (mybuf.pixfmt) {
		case BUF_PIX_FMT_RGBA: case BUF_PIX_FMT_RGB24: {
			cnpy::npy_save<uint8_t>(filepath_noexten + std::string(".npy"),
				mybuf.cdata<uint8_t>(), { static_cast<size_t>(mybuf.height), static_cast<size_t>(mybuf.width) });
			break;
		}
		case BUF_PIX_FMT_GRAYU32: {
			cnpy::npy_save<uint32_t>(filepath_noexten + std::string(".npy"),
				mybuf.cdata<uint32_t>(), { static_cast<size_t>(mybuf.height), static_cast<size_t>(mybuf.width) });
			break;
		}
		case BUF_PIX_FMT_GRAYF32: {
			cnpy::npy_save<float>(filepath_noexten + std::string(".npy"),
				mybuf.cdata<float>(), { static_cast<size_t>(mybuf.height), static_cast<size_t>(mybuf.width) });
			break;
		}
		default: allgood = false;
		}
	}
	if (writers & ImageWriter_fpzip) {
		allgood &= save_packedbuf_f32_using_fpzip(filepath_noexten + std::string(".fpzip"),
			mybuf, errstr);
	}
	return allgood;
}
