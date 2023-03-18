#pragma once

#include <filesystem>
#include <optional>

#include "symbol.h"

namespace fs = std::filesystem;

namespace stardust {
	class Descriptor {

	public:
		Symbol symbol;
		std::optional<std::string> name;

		bool hasName() const {
			return name.has_value();
		}

		bool operator==(const Descriptor& other) {
			return symbol == other.symbol && name == other.name;
		}

		Descriptor(Symbol symbol, const std::optional<std::string>& name = {})
			: symbol(symbol), name(name) {}
	};
}
