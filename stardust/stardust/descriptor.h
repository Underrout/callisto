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

		bool operator==(const Descriptor& other) const {
			return symbol == other.symbol && name == other.name;
		}

		bool operator!=(const Descriptor& other) const {
			return !(*this == other);
		}

		Descriptor(Symbol symbol, const std::optional<std::string>& name = {})
			: symbol(symbol), name(name) {}

		Descriptor(const json& j)
			: symbol(j["symbol"]), name(j["name"].is_null() ? std::nullopt : std::make_optional(j["name"])) {}

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

		std::string toString(const fs::path& project_root) const {
			switch (symbol) {
			case Symbol::INITIAL_PATCH:
				return "Initial Patch";
			case Symbol::GRAPHICS:
				return "Graphics";
			case Symbol::EX_GRAPHICS:
				return "ExGraphics";
			case Symbol::MAP16:
				return "Map16";
			case Symbol::SHARED_PALETTES:
				return "Shared Palettes";
			case Symbol::OVERWORLD:
				return "Overworld";
			case Symbol::TITLE_SCREEN:
				return "Title Screen";
			case Symbol::TITLE_SCREEN_MOVEMENT:
				return "Title Screen Movement";
			case Symbol::CREDITS:
				return "Credits";
			case Symbol::GLOBAL_EX_ANIMATION:
				return "Global ExAnimation";
			case Symbol::LEVEL:
				return fmt::format("Level ({})", fs::relative(name.value(), project_root).string());
			case Symbol::PATCH:
				return fmt::format("Patch ({})", fs::relative(name.value(), project_root).string());
			case Symbol::GLOBULE:
				return fmt::format("Globule ({})", fs::relative(name.value(), project_root).string());
			case Symbol::EXTERNAL_TOOL:
				return name.value();
			}
		}
	};
}

namespace std {
	template<>
	struct hash<stardust::Descriptor> {
		size_t operator()(const stardust::Descriptor& descriptor) const {
			return hash<stardust::Symbol>{}(descriptor.symbol) ^
				hash<std::string>{}(descriptor.name.value_or(""));
		}
	};
}
