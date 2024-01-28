#pragma once

#include <thread>
#include <mutex>

namespace callisto {
	namespace globals {
		extern size_t MAX_THREAD_COUNT;
		extern bool ALLOW_USER_INPUT;
		extern std::mutex cin_lock;

		void setMaxThreadCount(size_t proposed_thread_count);
	}
}
