#include "globals.h"

namespace callisto {
	namespace globals {
		// leaving one thread free by default
		size_t MAX_THREAD_COUNT = std::jthread::hardware_concurrency() == 1
			? 1
			: std::jthread::hardware_concurrency() - 1;

		void setMaxThreadCount(size_t proposed_thread_count) {
			MAX_THREAD_COUNT = proposed_thread_count > std::jthread::hardware_concurrency()
				? std::jthread::hardware_concurrency()
				: proposed_thread_count;
		}
	}
}
