#include "title_moves.h"

namespace stardust {
	TitleMoves::TitleMoves(const fs::path& lunar_magic_path,
		const fs::path& temporary_rom_path, const fs::path& title_moves_path)
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), title_moves_path(title_moves_path)
	{
		if (!fs::exists(title_moves_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Title Moves savestate not found at {}",
				title_moves_path.string()
			));
		}
	}

	std::unordered_set<Dependency> TitleMoves::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(title_moves_path);
		return dependencies;
	}

	void TitleMoves::insert() {
		spdlog::info("Inserting Title Moves");
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
			spdlog::info("Successfully inserted Title Moves!");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert Title Moves from {} into temporary ROM at {}",
				title_moves_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
