#pragma once

#include <thread>

namespace callisto {
	namespace globals {
		extern size_t MAX_THREAD_COUNT;
		extern bool ALLOW_USER_INPUT;

		void setMaxThreadCount(size_t proposed_thread_count);
	}
}
