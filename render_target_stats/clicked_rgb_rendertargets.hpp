#pragma once
// Copyright (C) 2023 Jason Bunk
#include <unordered_set>

// An instance of this is created for the device.
// The user can click on resources to capture draws.
struct __declspec(uuid("894d6808-7e3e-4a91-8c2e-aff9a6a09cb3")) clicked_rgb_rendertargets {
	// This is a set of reshade::api::resource handles.
	// If the user hasn't clicked on anything specifically,
	// then this will hold the recommended automatically detected resource.
	std::unordered_set<uint64_t> clicked_resources;
};
