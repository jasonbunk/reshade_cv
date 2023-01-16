#pragma once
// Copyright (C) 2022 Jason Bunk
#include <reshade.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <string>
#include <Windows.h>
#include <concurrentqueue.h>
#include "gcv_games/game_interface.h"
#include "gcv_utils/image_queue_entry.h"
#include "copy_texture_into_packedbuf.h"

struct logqueueitem {
	int loglevel = 3;
	std::string msg;
	logqueueitem(int loglevel_, const std::string &pstr) : loglevel(loglevel_), msg(pstr) {}
};

class __declspec(uuid("1c636132-f34e-937a-9160-141782c70a12")) image_writer_thread_pool {
	std::vector<std::thread> workthreads;
	std::vector<std::atomic<int> *> threadkeepalives;
	moodycamel::ConcurrentQueue<queue_item_image2write *> images2writequeue;
	moodycamel::ConcurrentQueue<logqueueitem *> errlogqueue;
	GameInterface *game = nullptr;

	void create_threads(size_t howmany);
	void join_and_delete_threads(size_t howmany);
	void flush_queue_by_deleting_waiting();
public:
	std::chrono::steady_clock::time_point init_time;
	depth_tex_settings depth_settings;
	std::wstring images_save_dir = L"cv_saved";

	bool camcoordsinitialized = false;
	bool grabcamcoords = false;

	// methods from GameInterface
	bool init_on_startup();
	bool init_in_game();
	bool game_knows_depthbuffer();
	std::string gamename_simpler();
	std::string gamename_verbose();
	uint8_t get_camera_matrix(CamMatrix &rcam, std::string &errstr);

	std::string output_filepath_creates_outdir_if_needed(const std::string &base_filename);

	~image_writer_thread_pool();
	void cleanup_clear_all();

	size_t num_threads() const { return workthreads.size(); }
	void change_num_threads(size_t new_num);

	void print_waiting_log_messages();

	bool save_texture_image_needing_resource_barrier_copy(
		const std::string &base_filename, uint64_t image_writers,
		reshade::api::command_queue *queue, reshade::api::resource tex,
		bool allow_alpha_channel);
};
