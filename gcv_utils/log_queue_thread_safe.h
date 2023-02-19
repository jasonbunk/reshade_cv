#pragma once
// Copyright (C) 2022 Jason Bunk
#include <concurrentqueue.h>

struct logqueueitem {
	int loglevel = 3;
	std::string msg;
	logqueueitem(int loglevel_, const std::string& pstr) : loglevel(loglevel_), msg(pstr) {}
};

class logqueue {
	moodycamel::ConcurrentQueue<logqueueitem*> errlogqueue;
public:
	void enqueue(int loglevel_, const std::string& pstr);
	void print_waiting_log_messages();
};