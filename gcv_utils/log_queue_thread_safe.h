#pragma once
// Copyright (C) 2022 Jason Bunk
#include <concurrentqueue.h>
#include <reshade.hpp>

struct logqueueitem {
	reshade::log_level loglevel = reshade::log_level::info;
	std::string msg;
	logqueueitem(reshade::log_level loglevel_, const std::string& pstr) : loglevel(loglevel_), msg(pstr) {}
};

class logqueue {
	moodycamel::ConcurrentQueue<logqueueitem*> errlogqueue;
public:
	void enqueue(reshade::log_level loglevel_, const std::string& pstr);
	void print_waiting_log_messages();
};
