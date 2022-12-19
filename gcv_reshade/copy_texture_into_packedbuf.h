#pragma once
// Copyright (C) 2022 Jason Bunk
#include <reshade.hpp>
#include <vector>
#include <string>
#include "gcv_games/game_interface.h"
#include "gcv_utils/simple_packed_buf.h"

struct depth_tex_settings {
	int depthbyteskeep = 0;
	int depthbytes = 0;
	int adjustpitchhack = 0;
	int depthnormalization = 0;
	bool alreadyfloat = false;
	bool float_reverse_endian = false;
	bool subtractfrom = true;
	bool reciprocal = true;
	bool debug_mode = false;
	bool more_verbose = true;
};

bool copy_texture_image_needing_resource_barrier_into_packedbuf(
	GameInterface *gamehandle, simple_packed_buf &dstBuf,
	reshade::api::command_queue *queue, reshade::api::resource tex,
	bool isdepth, const depth_tex_settings &debug_settings);
