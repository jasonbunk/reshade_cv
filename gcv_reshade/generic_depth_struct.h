#pragma once
/*
 * Source: reshade addon examples: generic_depth
 * Copyright (C) 2021 Patrick Mours
 * SPDX-License-Identifier: BSD-3-Clause OR MIT
 * https://github.com/crosire/reshade/blob/8b52d368ea8a89b302e4e42f87c6654875f9596d/examples/09-depth/generic_depth.cpp#L116-L130
 */
#include <reshade.hpp>

// this UUID needs to match the generic_depth example
struct __declspec(uuid("7c6363c7-f94e-437a-9160-141782c44a98")) generic_depth_data
{
	// The depth-stencil resource that is currently selected as being the main depth target
	reshade::api::resource selected_depth_stencil = { 0 };

	// Resource used to override automatic depth-stencil selection
	reshade::api::resource override_depth_stencil = { 0 };

	// The current depth shader resource view bound to shaders
	// This can be created from either the selected depth-stencil resource (if it supports shader access) or from a backup resource
	reshade::api::resource_view selected_shader_resource = { 0 };

	// True when the shader resource view was created from the backup resource, false when it was created from the original depth-stencil
	bool using_backup_texture = false;
};
