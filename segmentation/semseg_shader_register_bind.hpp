// Copyright (C) 2023 Jason Bunk
#pragma once
#include <reshade.hpp>
#include <unordered_map>
#include "segmentation_app_data.hpp"

// Use this as a reshade event callback. Customizes shaders as they are being loaded
bool on_create_pipeline_add_semseg(reshade::api::device* device, reshade::api::pipeline_layout playout, uint32_t subobject_count, const reshade::api::pipeline_subobject* subobjects);

// Use this as a reshade event callback to register the above modified shader to the pipeline object that was created
void on_after_create_pipeline_register_semseg(reshade::api::device *device, reshade::api::pipeline_layout layout, uint32_t subobject_count, const reshade::api::pipeline_subobject *subobjects, reshade::api::pipeline pipeline);

// The register at which to bind is different for each shader/pipeline, so we keep track of this in segmentation_app_data
bool pipeline_bind_bonus_tex_for_a_draw(reshade::api::device* device, reshade::api::command_list* cmd_list, segmentation_app_data& mapp, reshade::api::resource_view tex_view, uint64_t& formerly_bound_rsc);

perdraw_metadata_type get_draw_metadata_including_shader_info(reshade::api::device* device, reshade::api::command_list* cmd_list, uint32_t draw_num_vertices);
