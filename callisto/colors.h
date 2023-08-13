#pragma once

#include <fmt/color.h>

namespace callisto {
	namespace colors {
		namespace build {
			constexpr auto HEADER{ fmt::fg(fmt::color::white) | fmt::emphasis::bold };
			constexpr auto REMARK{ fmt::fg(fmt::color::orchid) };
			constexpr auto SUCCESS{ fmt::fg(fmt::color::aquamarine) | fmt::emphasis::bold };
			constexpr auto PARTIAL_SUCCESS{ fmt::fg(fmt::color::medium_aquamarine) };
			constexpr auto NOTIFICATION{ fmt::fg(fmt::color::light_coral) };
			constexpr auto EXCEPTION{ fmt::fg(fmt::color::medium_violet_red) };
			constexpr auto WARNING{ fmt::fg(fmt::color::coral) };
			constexpr auto MISC{ fmt::fg(fmt::color::medium_orchid) };
		}
	}
}
