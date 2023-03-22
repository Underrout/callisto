#pragma once

#include <filesystem>
#include <optional>

#include <nlohmann/json.hpp>

#include "symbol.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

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

		Descriptor(const json& j)
			: symbol(j["symbol"]), name(j["name"]) {}

		json toJson() const {
			auto j{ json({
				{"symbol", symbol},
			}) };

			if (hasName()) {
				j["name"] = name.value();
			}
			else {
				j["name"] = nullptr;
			}

			return j;
		}
	};
}
