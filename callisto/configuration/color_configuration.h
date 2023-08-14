#pragma once

#include <toml.hpp>

#include "config_variable.h"
#include "config_exception.h"

namespace callisto {
	class ColorConfiguration {
	public:
		IntegerConfigVariable fg_color;
		IntegerConfigVariable bg_color;

		StringVectorConfigVariable emphases;

		ColorConfiguration(const std::string& color_name) :
			fg_color({ "colors", "terminal", color_name, "fg" }),
			bg_color({ "colors", "terminal", color_name, "bg" }),
			emphases({ "colors", "terminal", color_name, "emphasis" })
		{}
	};
}
