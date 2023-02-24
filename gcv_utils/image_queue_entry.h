#pragma once
// Copyright (C) 2022 Jason Bunk
#include "gcv_utils/simple_packed_buf.h"
#include <string>

enum ImageWriterType {
	ImageWriter_none    = 0,
	ImageWriter_STB_png = (1 << 0),
	ImageWriter_numpy   = (1 << 1),
	ImageWriter_fpzip   = (1 << 2),
	ImageWriter_end     = (1 << 3),
};

struct queue_item_image2write {
	uint64_t writers = ImageWriter_none;
	simple_packed_buf mybuf;
	std::string filepath_noexten;

	queue_item_image2write(uint64_t image_writers,
		const std::string &filepath_noextension)
		: writers(image_writers), filepath_noexten(filepath_noextension) {}

	bool write_to_disk(std::string &errstr) const;
};
