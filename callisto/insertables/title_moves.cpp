#include "title_moves.h"

namespace callisto {
	TitleMoves::TitleMoves(const Configuration& config)
		: LunarMagicInsertable(config), title_moves_path(registerConfigurationDependency(config.title_moves).getOrThrow())
	{
		if (!fs::exists(title_moves_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::build::EXCEPTION,
				"Title Moves savestate not found at {}",
				title_moves_path.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> TitleMoves::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(title_moves_path);
		return dependencies;
	}

	void TitleMoves::insert() {
		spdlog::info(fmt::format(colors::build::REMARK, "Inserting Title Moves"));
		spdlog::debug(fmt::format(
			"Inserting Title Moves from {} into temporary ROM at {}",
			title_moves_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic(
			"-ImportTitleMoves", 
			temporary_rom_path.string(), 
			title_moves_path.string()) 
		};

		if (exit_code == 0) {
			spdlog::info(fmt::format(colors::build::PARTIAL_SUCCESS, "Successfully inserted Title Moves!"));
		}
		else {
			throw InsertionException(fmt::format(
				colors::build::EXCEPTION,
				"Failed to insert Title Moves from {} into temporary ROM at {}",
				title_moves_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
