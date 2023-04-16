// Copyright (C) 2023 Jason Bunk
#include "semseg_shader_register_bind.hpp"
#include "customize_dxbc.hpp"
#include "rdoc_utils.hpp"
#include "command_list_state.hpp"
#include "xxhash.h"
#include <sstream>
#include <fstream>
static constexpr bool verbose = false;
using namespace reshade::api;
using std::endl;

struct shader_entry_workspace {
	bytebuf* b = nullptr;
	custom_shader_layout_registers r;
	shader_hash_t hash = 0ull;
	shader_desc* s = nullptr;
	const void* backup_code = nullptr;
	size_t backup_code_size = 0;
	bool good = false;
};

static thread_local std::unordered_map<pipeline_subobject_type, shader_entry_workspace> shader_workspace;
static std::atomic<int> numvalidshadersseen = { 0 };
static std::atomic<int> numvalidshaderssuccessfullymodified = { 0 };


bool on_create_pipeline_add_semseg(device* device, pipeline_layout playout, uint32_t subobject_count, const pipeline_subobject* subobjects) {

	if (device->get_api() != device_api::d3d10 && device->get_api() != device_api::d3d11) return false; // these are the only supported APIs for now

	bool pipemodified = false;
	shader_workspace.clear();
	std::stringstream ss;
	if (verbose) ss << "on_create_pipeline: " << playout.handle << ", subobjcount " << subobject_count << ", subobjects " << reinterpret_cast<const void*>(subobjects) << endl;

	int num_pixl = 0;
	int num_vert = 0;
	int num_unwanted = 0;
	bool triedandfailed = false;

	for (uint32_t i = 0; i < subobject_count; ++i)
	{
		if (subobjects[i].data == nullptr || subobjects[i].type == pipeline_subobject_type::unknown || subobjects->type > pipeline_subobject_type::compute_shader) continue;
		const shader_desc* shdesc = static_cast<const shader_desc*>(subobjects[i].data);
		if (shdesc->code == nullptr || shdesc->code_size <= 4) continue;
		switch (subobjects[i].type) {
		case pipeline_subobject_type::pixel_shader:   num_pixl += subobjects[i].count; break;
		case pipeline_subobject_type::vertex_shader:  num_vert += subobjects[i].count; break;
		case pipeline_subobject_type::geometry_shader:num_unwanted += subobjects[i].count; break;
		case pipeline_subobject_type::hull_shader:    num_unwanted += subobjects[i].count; break;
		case pipeline_subobject_type::domain_shader:  num_unwanted += subobjects[i].count; break;
		}
	}
	if (verbose) ss << "  num shaders: pixel " << num_pixl << ", vertex " << num_vert << ", num geom/hull/domain: " << num_unwanted << endl;

	if ((num_pixl == 1 || num_vert == 1) && num_unwanted == 0) {
		auto& mapp = device->get_private_data<segmentation_app_data>();
		for (uint32_t i = 0; i < subobject_count; ++i)
		{
			if (subobjects[i].data == nullptr) continue;
			if (subobjects[i].type == pipeline_subobject_type::pixel_shader || subobjects[i].type == pipeline_subobject_type::vertex_shader) {
				{
					const shader_desc* shdesc = static_cast<const shader_desc*>(subobjects[i].data);
					if (shdesc->code == nullptr || shdesc->code_size <= 4) continue;
				}
				shader_entry_workspace& shws = shader_workspace[subobjects[i].type];
				shws.s = static_cast<shader_desc*>(subobjects[i].data);
				shws.backup_code = shws.s->code;
				shws.backup_code_size = shws.s->code_size;
				// TODO: this code considers a hash collision impossible... it's not, so this is wrong: a hash collision will provide the wrong shader
				shws.hash = XXH64(shws.s->code, shws.s->code_size, 0);
				bool modsucceeded = false;
				if (auto shentry = mapp.shader_hash_to_custom_shader_bytes.find(shws.hash); shentry != mapp.shader_hash_to_custom_shader_bytes.end()) {
					modsucceeded = true;
					shws.b = shentry->second;
				} else {
					shws.b = new bytebuf();
					shws.b->resize(shws.s->code_size);
					memcpy(shws.b->data(), shws.s->code, shws.s->code_size); // make a copy of the original shader
					mapp.shader_hash_to_custom_shader_bytes.emplace(shws.hash, shws.b);
					modsucceeded = customize_shader_dxbc_or_dxil(
						subobjects[i].type == pipeline_subobject_type::pixel_shader,
						device->get_api(), shws.b, shws.r, ss);
				}
				if (modsucceeded) {
					shws.good = true;
					shws.s->code = shws.b->data(); // replace the original shader with data stored in thread_local map "shader_workspace"
					shws.s->code_size = shws.b->size();
					if (verbose) ss << "we (temporarily?) customized the shader" << endl;
				} else {
					shws.good = false;
					triedandfailed = true;
				}
			}
		}
		if (!shader_workspace.empty()) {
			pipemodified = true;
			for (auto it = shader_workspace.cbegin(); it != shader_workspace.cend(); ++it) {
				if (!it->second.good) {
					pipemodified = false;
					break;
				}
			}
			numvalidshadersseen++;
			if (pipemodified) {
				numvalidshaderssuccessfullymodified++;
				if (verbose) ss << "we seem to have successfully customized the shader pipeline which has " << shader_workspace.size() << " shaders" << endl;
			} else {
				// roll back changes if any one shader broke
				for (auto it = shader_workspace.begin(); it != shader_workspace.end(); ++it) {
					it->second.s->code = it->second.backup_code;
					it->second.s->code_size = it->second.backup_code_size;
				}
				shader_workspace.clear();
				if (verbose) ss << "because one shader broke, aborting" << endl;
			}
		}
	}
	if (verbose && (num_pixl == 1 || num_vert == 1) && triedandfailed) {
		ss << endl;
		const std::string strstr = ss.str();
		if (!strstr.empty()) reshade::log_message(reshade::log_level::info, strstr.c_str());
	}
	return pipemodified;
}


void on_after_create_pipeline_register_semseg(device *device, pipeline_layout layout, uint32_t subobject_count, const pipeline_subobject *subobjects, pipeline pipeline) {
	if (shader_workspace.count(pipeline_subobject_type::pixel_shader) || shader_workspace.count(pipeline_subobject_type::vertex_shader)) {
		bool matchedpipeline = true;
		for(auto it = shader_workspace.cbegin(); it != shader_workspace.cend(); ++it) {
			bool this_shader_matched = false;
			for (uint32_t i = 0; i < subobject_count; ++i) {
				if (subobjects[i].type == it->first) {
					const shader_desc* shdesc = static_cast<const shader_desc*>(subobjects[i].data);
					if (shdesc->code_size == it->second.s->code_size && shdesc->code == it->second.s->code) {
						this_shader_matched = true;
						break;
					}
				}
			}
			if (!this_shader_matched) {
				matchedpipeline = false;
				break;
			}
		}
		if (matchedpipeline) {
			auto& mapp = device->get_private_data<segmentation_app_data>();
			if (shader_workspace.count(pipeline_subobject_type::pixel_shader)) {
				const shader_entry_workspace& shws_px = shader_workspace.at(pipeline_subobject_type::pixel_shader);
				mapp.pipeline_handle_to_shader_layout_registers[pipeline.handle] = shws_px.r;
				mapp.pipeline_handle_to_pipeline_layout_handle[pipeline.handle] = layout.handle;
				mapp.pipeline_handle_to_pixel_shader_hash[pipeline.handle] = shws_px.hash;
				if (verbose) reshade::log_message(reshade::log_level::info,
					std::string(std::string("successfully registered shader pipeline ")+std::to_string(pipeline.handle)+std::string(" of hash ")+std::to_string(shws_px.hash)+std::string(", with shader registers ") + shws_px.r.to_string()).c_str());
			}
			if (shader_workspace.count(pipeline_subobject_type::vertex_shader)) {
				mapp.pipeline_handle_to_vertex_shader_hash[pipeline.handle] = shader_workspace.at(pipeline_subobject_type::vertex_shader).hash;
			}
		}
	}
	shader_workspace.clear();
}


perdraw_metadata_type get_draw_metadata_including_shader_info(device* device, command_list* cmd_list, uint32_t draw_num_vertices) {
	auto& mapp = device->get_private_data<segmentation_app_data>();
	auto& cmdlst_state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	perdraw_metadata_type ret = { draw_num_vertices, 0ull, 0ull };
	for (const auto& [stages, pipeline] : cmdlst_state.pipelines) {
		if (pipeline.handle != 0ull) {
			if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(pipeline_stage::vertex_shader) && mapp.pipeline_handle_to_vertex_shader_hash.count(pipeline.handle))
				ret[1] = mapp.pipeline_handle_to_vertex_shader_hash.at(pipeline.handle);
			if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(pipeline_stage::pixel_shader) && mapp.pipeline_handle_to_pixel_shader_hash.count(pipeline.handle))
				ret[2] = mapp.pipeline_handle_to_pixel_shader_hash.at(pipeline.handle);
		}
	}
	return ret;
}


#include <d3d10_1.h>
#include <d3d11.h>

bool pipeline_bind_bonus_tex_for_a_draw(device* device, command_list* cmd_list, segmentation_app_data& mapp, resource_view tex_view, uint64_t &formerly_bound_rsc) {
	auto& cmdlst_state = cmd_list->get_private_data<segmentation_app_cmdlist_state>();
	if (cmdlst_state.pipelines.empty()) return false;
	std::unordered_set<uint64_t> pixshaderhandles;
	for (const auto& [stages, pipeline] : cmdlst_state.pipelines) {
		if (pipeline.handle != 0ull && static_cast<uint32_t>(stages) & static_cast<uint32_t>(pipeline_stage::pixel_shader))
			pixshaderhandles.insert(pipeline.handle);
	}
	if (pixshaderhandles.empty()) return false;
	bool foundahandle = false;
	uint64_t currpipehandle = 0ull;
	for (const uint64_t possiblehandle : pixshaderhandles) {
		if (mapp.pipeline_handle_to_shader_layout_registers.count(possiblehandle)
		      && mapp.pipeline_handle_to_pipeline_layout_handle.count(possiblehandle)) {
			foundahandle = true;
			currpipehandle = possiblehandle;
			break;
		}
	}
	if (!foundahandle || currpipehandle == 0ull || !mapp.pipeline_handle_to_shader_layout_registers.count(currpipehandle)) {
		if (mapp.logged_device_on_draw_bind_api_compatibility.exchange(1) == 0)
			reshade::log_message(reshade::log_level::warning, std::string(std::string("pipeline handle ") + std::to_string(currpipehandle)
				+ std::string(" not found in pipeline handle map of size ") + std::to_string(mapp.pipeline_handle_to_shader_layout_registers.size())).c_str());
		return false;
	}
	const custom_shader_layout_registers& shreg = mapp.pipeline_handle_to_shader_layout_registers.at(currpipehandle);

	if (device->get_api() == device_api::d3d10) {
		const int registerhere = shreg.perdrawbuf_tex_regL;
		if (registerhere >= 0) {
			ID3D10Device1* cmdl = reinterpret_cast<ID3D10Device1*>(cmd_list->get_native());
			cmdl->PSGetShaderResources(registerhere, 1, reinterpret_cast<ID3D10ShaderResourceView**>(&formerly_bound_rsc));
			cmdl->PSSetShaderResources(registerhere, 1, reinterpret_cast<ID3D10ShaderResourceView**>(&(tex_view.handle)));
			return true;
		}
	}
	else if (device->get_api() == device_api::d3d11) {
		const int registerhere = shreg.perdrawbuf_tex_regL;
		if (registerhere >= 0) {
			ID3D11DeviceContext* cmdl = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
			cmdl->PSGetShaderResources(registerhere, 1, reinterpret_cast<ID3D11ShaderResourceView**>(&formerly_bound_rsc));
			cmdl->PSSetShaderResources(registerhere, 1, reinterpret_cast<ID3D11ShaderResourceView**>(&(tex_view.handle)));
			return true;
		}
	} else if(mapp.logged_device_on_draw_bind_api_compatibility.exchange(1) == 0) {
		reshade::log_message(reshade::log_level::error,
			std::string(std::string("resource bind: unrecognized/unsupported device api ")
				+ std::to_string(static_cast<int>(device->get_api()))).c_str());
	}
	return false;
}
