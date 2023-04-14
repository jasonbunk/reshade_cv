// Copyright (C) 2022 Jason Bunk
#include "log_queue_thread_safe.h"

void logqueue::enqueue(reshade::log_level loglevel, const std::string& pstr) {
	logqueueitem* lqi = new logqueueitem(loglevel, pstr);
	errlogqueue.enqueue(lqi);
}

void logqueue::print_waiting_log_messages() {
	logqueueitem* lqi = nullptr;
	while (errlogqueue.try_dequeue(lqi)) {
		reshade::log_message(lqi->loglevel, lqi->msg.c_str());
		delete lqi;
		lqi = nullptr;
	}
}
