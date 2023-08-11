#pragma once

#include <fmt/color.h>

namespace callisto {
	namespace colors {
		namespace build {
			constexpr auto HEADER{ fmt::fg(fmt::color::white) | fmt::emphasis::bold };
			constexpr auto REMARK{ fmt::fg(fmt::color::magenta) };
			constexpr auto SUCCESS{ fmt::fg(fmt::color::light_green) | fmt::emphasis::bold };
			constexpr auto PARTIAL_SUCCESS{ fmt::fg(fmt::color::light_green) };
			constexpr auto NOTIFICATION{ fmt::fg(fmt::color::yellow) };
			constexpr auto EXCEPTION{ fmt::fg(fmt::color::red) | fmt::emphasis::bold };
			constexpr auto WARNING{ fmt::fg(fmt::color::orange) };
			constexpr auto MISC{ fmt::fg(fmt::color::cyan) };
		}
	}
}
