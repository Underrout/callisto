#include "levels.h"

namespace callisto {
	Levels::Levels(const Configuration& config)
		: LunarMagicInsertable(config), 
		levels_folder(registerConfigurationDependency(config.levels, Policy::REINSERT).getOrThrow()),
		allow_user_input(config.allow_user_input)
	{
		registerConfigurationDependency(config.level_import_flag);

		if (config.level_import_flag.isSet()) {
			import_flag = config.level_import_flag.getOrThrow();
		}
		else {
			import_flag = std::nullopt;
		}

		if (!fs::exists(levels_folder)) {
			throw ResourceNotFoundException(fmt::format(
				"Level folder {} not found",
				levels_folder.string()
			));
		}
	}

	void Levels::normalizeMwls() {
		std::vector<fs::path> mwl_paths{};
		for (const auto& entry : fs::directory_iterator(levels_folder)) {
			if (entry.path().extension() == ".mwl") {
				mwl_paths.push_back(entry);
			}
		}

		for (const auto& entry : mwl_paths) {
			const auto mwl_level_number{ getInternalLevelNumber(entry) };
			if (!mwl_level_number.has_value()) {
				throw InsertionException(fmt::format(
					"Could not determine source level number of level file '{}', this level file may be malformed",
					entry.string()
				));
			}

			const auto file_level_number{ getExternalLevelNumber(entry) };
			if (!file_level_number.has_value()) {
				if (allow_user_input) {
					char answer;
					spdlog::warn(
						"Level file '{}' (pointing to level {:X}) has malformed filename, rename to 'level {:03X}.mwl'? y/n",
						entry.string(), mwl_level_number.value(), mwl_level_number.value()
					);
					std::cin >> answer;
					if (answer == 'y' || answer == 'Y') {
						const auto target_path{ entry.parent_path() / fmt::format("level {:03X}.mwl", mwl_level_number.value()) };
						if (fs::exists(target_path)) {
							throw InsertionException(fmt::format(
								"Failed to rename '{}', '{}' already exists",
								entry.filename().string(), target_path.filename().string()
							));
						}
						try {
							fs::rename(entry, target_path);
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								"Failed to rename level file '{}' to '{}' with exception:\n{}",
								entry.string(), target_path.string(),
								e.what()
							));
						}
						continue;
					}
				}
				throw InsertionException(fmt::format(
					"Level file '{}' (pointing to level {:X}) has malformed filename",
					entry.string(), mwl_level_number.value()
				));
			}

			if (file_level_number != mwl_level_number) {
				if (allow_user_input) {
					char answer;
					spdlog::warn(
						"Level file '{}' points to level {:X} but filename says it should point to level {:X}, what should be done?\n\r"
						"(a) Keep filename '{}', but point it at level {:X}\n\r"
						"(b) Rename file to 'level {:03X}.mwl', keep pointing at level {:X}\n\r"
						"(c) Abort",
						entry.string(), mwl_level_number.value(), file_level_number.value(),
						entry.filename().string(), file_level_number.value(),
						mwl_level_number.value(), mwl_level_number.value()
					);
					std::cin >> answer;
					if (answer == 'a' || answer == 'A') {
						try {
							setInternalLevelNumber(entry, file_level_number.value());
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								"Failed to set source level number of level file '{}' to {:X} with exception:\n{}",
								entry.string(), file_level_number.value(),
								e.what()
							));
						}
						continue;
					}
					else if (answer == 'b' || answer == 'B') {
						const auto target_path{ entry.parent_path() / fmt::format("level {:03X}.mwl", mwl_level_number.value()) };
						if (fs::exists(target_path)) {
							throw InsertionException(fmt::format(
								"Failed to rename '{}', '{}' already exists",
								entry.filename().string(), target_path.filename().string()
							));
						}
						try {
							fs::rename(entry, target_path);
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								"Failed to rename level file '{}' to '{}' with exception:\n{}",
								entry.string(), target_path.string(),
								e.what()
							));
						}
						continue;
					}
				}
				throw InsertionException(fmt::format(
					"Level file '{}' points to level {:X} but filename says it should point to level {:X}",
					entry.string(), mwl_level_number.value(), file_level_number.value()
				));
			}
		}
	}

	size_t Levels::readBytesAt(std::fstream& stream, size_t offset, size_t count) {
		size_t out{ 0 };

		stream.seekg(offset);
		for (auto i{ 0 }; i != count; ++i) {
			char c;
			stream.read(&c, 1);
			out |= static_cast<unsigned char>(c) << (8 * i);
		}

		return out;
	}

	std::optional<int> Levels::getInternalLevelNumber(const fs::path& mwl_path){
		try {
			std::fstream mwl_file(mwl_path, std::ios::binary | std::ios::in);

			const auto data_pointer_table_pointer{ readBytesAt(mwl_file, MWL_DATA_POINTER_TABLE_POINTER_OFFSET, 4) };
			const auto level_information_pointer{ readBytesAt(mwl_file, data_pointer_table_pointer, 4) };
			const auto level_number{ readBytesAt(mwl_file, level_information_pointer, 2) };

			if (level_number < 0x200) {
				return level_number;
			}
			else {
				return {};
			}
		}
		catch (...) {
			return {};
		}
	}

	std::optional<int> Levels::getExternalLevelNumber(const fs::path& mwl_path) {
		const auto mwl_name{ mwl_path.stem().string() };

		if (!mwl_name.starts_with("level ")) {
			return {};
		}

		if (!std::all_of(mwl_name.begin() + 6, mwl_name.end(), [](const auto& e) { return std::isalnum(e); })) {
			return {};
		}

		try {
			const auto level_number{ std::stoi(mwl_name.substr(6, 3), nullptr, 16) };
			if (level_number < 0x200) {
				return level_number;
			}
			return {};
		}
		catch (...) {
			// we failed to parse the hexadecimal level number in some way
			return {};
		}
	}

	void Levels::setInternalLevelNumber(const fs::path& mwl_path, int level_number) {
		std::fstream mwl_file(mwl_path, std::ios::binary | std::ios::in | std::ios::out);

		const auto data_pointer_table_pointer{ readBytesAt(mwl_file, MWL_DATA_POINTER_TABLE_POINTER_OFFSET, 4) };
		const auto level_information_pointer{ readBytesAt(mwl_file, data_pointer_table_pointer, 4) };
		
		mwl_file.seekg(level_information_pointer);
		const char bytes[2] = { level_number & 0xFF, level_number >> 8 };
		mwl_file.write(bytes, 2);
	}

	std::unordered_set<ResourceDependency> Levels::determineDependencies() {
		return { ResourceDependency(levels_folder) };
	}

	void Levels::insert() {
		spdlog::info("Inserting levels");

		bool at_least_one_level{ false };
		for (const auto& entry : fs::directory_iterator(levels_folder)) {
			if (entry.path().extension() == ".mwl") {
				at_least_one_level = true;
				break;
			}
		}

		if (!at_least_one_level) {
			spdlog::info("No levels to insert, skipping level insertion");
			return;
		}

		normalizeMwls();

		int exit_code;

		if (import_flag.has_value()) {
			exit_code = callLunarMagic(
				"-ImportMultLevels",
				temporary_rom_path.string(),
				levels_folder.string(),
				import_flag.value());
		}
		else {
			exit_code = callLunarMagic(
				"-ImportMultLevels",
				temporary_rom_path.string(),
				levels_folder.string());
		}

		if (exit_code == 0) {
			spdlog::info("Successfully inserted levels");
			spdlog::debug(fmt::format(
				"Successfully inserted levels from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert level from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
	}
}
