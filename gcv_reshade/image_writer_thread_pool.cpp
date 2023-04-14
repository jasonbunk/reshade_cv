// Copyright (C) 2022 Jason Bunk
#include "image_writer_thread_pool.h"
#include "gcv_utils/miscutils.h"
#include "gcv_games/game_interface_factory.h"
#include <filesystem>
using moodycamel::ConcurrentQueue;

std::string image_writer_thread_pool::output_filepath_creates_outdir_if_needed(const std::string & base_filename) {
	wchar_t file_prefix[MAX_PATH] = L"";
	GetModuleFileNameW(nullptr, file_prefix, ARRAYSIZE(file_prefix));
	std::filesystem::path dump_path = file_prefix;
	dump_path = dump_path.parent_path();
	dump_path /= images_save_dir;
	if (std::filesystem::exists(dump_path) == false)
		std::filesystem::create_directory(dump_path);
	dump_path /= base_filename;
	return dump_path.string();
}

void image_writer_thread_pool::flush_queue_by_deleting_waiting() {
	queue_item_image2write *img2write = nullptr;
	while (images2writequeue.try_dequeue(img2write)) {
		delete img2write;
		img2write = nullptr;
	}
}

void image_writer_thread_pool::cleanup_clear_all() {
	change_num_threads(0);
	flush_queue_by_deleting_waiting();
	print_waiting_log_messages();
}

image_writer_thread_pool::~image_writer_thread_pool() {
	cleanup_clear_all();
}

void image_writer_thread_loop(ConcurrentQueue<queue_item_image2write *> *images2writequeue,
		logqueue *errlogqueue,
		std::atomic<int> *keeplooping) {
	queue_item_image2write *img2write = nullptr;
	while (keeplooping->load() > 0) {
		img2write = nullptr;
		if (images2writequeue->try_dequeue(img2write) && img2write != nullptr) {
			std::string logdesc(std::string(" img \'") + img2write->filepath_noexten
				+ std::string("\' of type ") + std::to_string(img2write->mybuf.pixfmt)
				+ std::string(" with writer(s) ") + std::to_string(img2write->writers)+std::string(" "));
			if (!img2write->write_to_disk(logdesc)) {
				errlogqueue->enqueue(reshade::log_level::error, std::string("FAILED to save") + logdesc);
			}
			else {
				errlogqueue->enqueue(reshade::log_level::info, std::string("Saved") + logdesc);
			}
			delete img2write;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void image_writer_thread_pool::create_threads(size_t howmany) {
	if ((num_threads()+howmany) > 9000) {
		reshade::log_message(reshade::log_level::error,
			std::string(std::string("create_threads() bad num threads ")
			+std::to_string(num_threads()+howmany)).c_str());
		return;
	}
	for (size_t ii = 0; ii < howmany; ++ii) {
		threadkeepalives.push_back(new std::atomic<int>(1));
		workthreads.emplace_back(image_writer_thread_loop, &images2writequeue, this, threadkeepalives.back());
	}
	reshade::log_message(reshade::log_level::info, std::string(std::string("created ") + std::to_string(howmany) + std::string(" image writer threads")).c_str());
}

void image_writer_thread_pool::join_and_delete_threads(size_t howmany) {
	if (workthreads.empty() || howmany == 0)
		return;
	for (int64_t ii = static_cast<int64_t>(std::min(howmany, workthreads.size())) - 1; ii >= 0; --ii) {
		threadkeepalives.back()->store(0);
		workthreads.back().join();
		workthreads.pop_back();
		delete threadkeepalives.back();
		threadkeepalives.pop_back();
	}
}

void image_writer_thread_pool::change_num_threads(size_t new_num) {
	if (new_num == workthreads.size())
		return;
	if (new_num > workthreads.size()) {
		create_threads(new_num - workthreads.size());
	} else {
		join_and_delete_threads(workthreads.size() - new_num);
	}
}


bool image_writer_thread_pool::init_on_startup() {
	if (game != nullptr) return true;
	game = GameInterfaceFactory::get().getGameInterface(lowercasenameofcurrentprocessexe());
	if (game == nullptr)
		return false;
	return game->init_on_startup();
}
bool image_writer_thread_pool::init_in_game() {
	if (!init_on_startup()) return false;
	return game->init_in_game();
}
bool image_writer_thread_pool::game_knows_depthbuffer() {
	if (!init_on_startup()) return false;
	return game->can_interpret_depth_buffer();
}
std::string image_writer_thread_pool::gamename_simpler() {
	if (!init_on_startup()) return "";
	return game->gamename_simpler();
}
std::string image_writer_thread_pool::gamename_verbose() {
	if (!init_on_startup()) return "";
	return game->gamename_verbose();
}
bool image_writer_thread_pool::get_camera_matrix(CamMatrixData &rcam, std::string &errstr) {
	if (!init_on_startup()) {
		errstr += std::string("failed to init/recognize game ") + lowercasenameofcurrentprocessexe();
		return false;
	}
	if (!init_in_game()) {
		errstr += "failed to init in game?";
		return false;
	}
	return game->get_camera_matrix(rcam, errstr);
}

bool image_writer_thread_pool::save_texture_image_needing_resource_barrier_copy(
	const std::string &base_filename, uint64_t image_writers,
	reshade::api::command_queue *queue, reshade::api::resource tex,
	bool isdepth)
{
	if (tex == 0) {
		reshade::log_message(reshade::log_level::error, std::string(std::string("depth texture null: failed to save ")+base_filename).c_str());
		return false;
	}
	if (num_threads() == 0) {
		change_num_threads(3);
	}
	if (num_threads() == 0) return false;
	init_in_game();
	queue_item_image2write *qume = new queue_item_image2write(image_writers,
		output_filepath_creates_outdir_if_needed(base_filename));
	if (!qume) {
		reshade::log_message(reshade::log_level::error, "failed to allocate new queue entry");
		return false;
	}
	if (!copy_texture_image_needing_resource_barrier_into_packedbuf(
				game, qume->mybuf, queue, tex, isdepth, depth_settings)) {
		delete qume;
		return false;
	}
	if (!images2writequeue.enqueue(qume)) {
		delete qume;
		return false;
	}
	return true;
}
