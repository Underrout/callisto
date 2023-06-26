#pragma once

#include <sstream>
#include <chrono>

namespace callisto {
	class TimeUtil {
	public:
		template<typename T, typename V>
		static std::string getDurationString(const std::chrono::duration<T, V>& duration) {
			const auto minutes{ std::chrono::duration_cast<std::chrono::minutes>(duration) };
			const auto seconds{ std::chrono::duration_cast<std::chrono::seconds>(duration - minutes) };
			std::ostringstream duration_string{};
			if (minutes.count() != 0) {
				duration_string << minutes.count() << "m ";
			}
			duration_string << seconds.count() << "s";
			return duration_string.str();
		}
	};
}
