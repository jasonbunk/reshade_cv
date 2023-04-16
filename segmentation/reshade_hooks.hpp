// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>

void register_segmentation_app_hooks();
void unregister_segmentation_app_hooks();

// return true if capture successful and ready for saving
bool segmentation_app_update_on_finish_effects(reshade::api::effect_runtime* runtime, bool requested_draw);
