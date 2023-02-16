#include "text_map16.h"

namespace stardust {
	TextMap16::TextMap16(const fs::path& lunar_magic_path, 
		const fs::path& temporary_rom_path, const fs::path& map16_folder_path, const fs::path& conversion_tool_path)
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), map16_folder_path(map16_folder_path),
		conversion_tool_path(conversion_tool_path)
	{
		if (!fs::exists(map16_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Map16 folder not found at {}",
				map16_folder_path.string()
			));
		}

		if (!fs::exists(conversion_tool_path)) {
			throw ToolNotFoundException(fmt::format(
				"Human Readable Map16 CLI not found at {}",
				conversion_tool_path.string()
			));
		}
	}

	fs::path TextMap16::getTemporaryMap16FilePath() const
	{
		return map16_folder_path.string() + ".map16";
	}

	fs::path TextMap16::generateTemporaryMap16File() const
	{
		spdlog::info("Generating Map16 file");
		spdlog::debug(fmt::format(
			"Generating temporary map16 file from map16 folder {}",
			map16_folder_path.string(),
			temporary_rom_path.string()
		));

		const auto temp_map16{ getTemporaryMap16FilePath() };

		const auto exit_code{ bp::system(
			conversion_tool_path.string(),
			"--to-map16",
			map16_folder_path.string(),
			temp_map16.string()
		)};

		if (exit_code == 0) {
			spdlog::info("Succesfully generated Map16 file");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to generate temporary map16 file at {} from map16 folder at {} using conversion tool at {}",
				temp_map16.string(),
				map16_folder_path.string(),
				conversion_tool_path.string()
			));
		}

		return temp_map16;
	}

	void TextMap16::deleteTemporaryMap16File() const
	{
		try {
			fs::remove(getTemporaryMap16FilePath());
		}
		catch (const fs::filesystem_error&) {
			spdlog::warn(fmt::format(
				"WARNING: Failed to delete temporary Map16 file {}",
				getTemporaryMap16FilePath().string()
			));
		}
	}

	void TextMap16::insert() {
		spdlog::info("Inserting Map16");
		spdlog::debug(fmt::format(
			"Generating and inserting map16 into temporary ROM at {}",
			map16_folder_path.string(),
			temporary_rom_path.string()
		));

		const auto temp_map16{ generateTemporaryMap16File() };

		const auto exit_code{ callLunarMagic(
			"-ImportAllMap16",
			temporary_rom_path.string(),
			temp_map16.string())
		};

		if (exit_code == 0) {
			spdlog::info("Successfully inserted Map16!");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert generated Map16 file {} into temporary ROM {}",
				temp_map16.string(),
				temporary_rom_path.string()
			));
		}

		deleteTemporaryMap16File();
	}
}
