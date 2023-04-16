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
	bool alreadyfloat = false;
	bool float_reverse_endian = false;
	bool debug_mode = false;
	bool more_verbose = false;
};

enum TextureInterpretation {
	TexInterp_RGB = 0,
	TexInterp_Depth,
	TexInterp_IndexedSeg,
};

bool copy_texture_image_needing_resource_barrier_into_packedbuf(
	GameInterface *gamehandle, simple_packed_buf &dstBuf,
	reshade::api::command_queue* queue, reshade::api::resource tex,
	TextureInterpretation tex_interp, const depth_tex_settings &debug_settings);
