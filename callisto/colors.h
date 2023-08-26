#pragma once

#include <array>
#include <optional>

#include <fmt/color.h>
#include <fmt/format.h>

#include <algorithm>
#include <string>

#include "callisto_exception.h"

namespace callisto {
	namespace colors {
		extern fmt::text_style ACTION_START;
		constexpr fmt::text_style DEFAULT_ACTION_START{ fmt::fg(static_cast<fmt::color>(0x7BADE2)) | fmt::emphasis::bold };

		extern fmt::text_style RESOURCE;
		constexpr fmt::text_style DEFAULT_RESOURCE{ fmt::fg(static_cast<fmt::color>(0xC384D7)) };

		extern fmt::text_style SUCCESS;
		constexpr fmt::text_style DEFAULT_SUCCESS{ fmt::fg(static_cast<fmt::color>(0x26CEAA)) | fmt::emphasis::bold };

		extern fmt::text_style PARTIAL_SUCCESS;
		constexpr fmt::text_style DEFAULT_PARTIAL_SUCCESS{ fmt::fg(static_cast<fmt::color>(0x9AE8C1)) };

		extern fmt::text_style NOTIFICATION;
		constexpr fmt::text_style DEFAULT_NOTIFICATION{ fmt::fg(static_cast<fmt::color>(0xFF9856)) };

		extern fmt::text_style EXCEPTION;
		constexpr fmt::text_style DEFAULT_EXCEPTION{ fmt::fg(static_cast<fmt::color>(0xA50162)) };

		extern fmt::text_style WARNING;
		constexpr fmt::text_style DEFAULT_WARNING{ fmt::fg(static_cast<fmt::color>(0xEF7427)) };

		extern fmt::text_style CALLISTO;
		constexpr fmt::text_style DEFAULT_CALLISTO{ fmt::fg(static_cast<fmt::color>(0xA359BB)) };

		void resetStyles();

		void addFgColor(fmt::text_style& style, uint32_t rgb);
		void addBgColor(fmt::text_style& style, uint32_t rgb);
		void addEmphasis(fmt::text_style& style, const std::string& emphasis_string);

		fmt::color getColor(uint32_t rgb);

		fmt::text_style& configurableColorStringToStyle(const std::string& style_name);

		extern std::array<std::string, 8> configurable_colors;
		extern std::array<std::string, 141> named_html_colors;
		extern std::array<std::string, 8> emphasis_names;
	}
}
