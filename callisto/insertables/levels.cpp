#include "levels.h"

namespace callisto {
	Levels::Levels(const Configuration& config)
		: LunarMagicInsertable(config), 
		levels_folder(registerConfigurationDependency(config.levels, Policy::REINSERT).getOrThrow())
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
				colors::EXCEPTION,
				"Level folder {} not found",
				levels_folder.string()
			));
		}
	}

	void Levels::normalizeMwls(const fs::path& levels_folder_path, bool allow_user_input) {
		for (const auto& entry : fs::directory_iterator(levels_folder_path)) {
			const auto path{ entry.path() };

			if (fs::is_directory(path)) {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Directory '{}' found inside levels directory, please remove this directory",
					path.filename().string()
				));
			}

			if (path.extension() != ".mwl") {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Non-MWL file '{}' found inside levels directory, please remove this file",
					path.filename().string()
				));
			}

			const auto mwl_level_number{ getInternalLevelNumber(path) };
			if (!mwl_level_number.has_value()) {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Could not determine source level number of level file '{}', this level file may be malformed",
					path.string()
				));
			}

			const auto file_level_number{ getExternalLevelNumber(path) };
			if (!file_level_number.has_value()) {
				if (allow_user_input) {
					char answer;
					spdlog::warn(
						fmt::format(colors::WARNING, 
						"Level file '{}' (pointing to level {:X}) has malformed filename, what should be done?\n\r"
						"(a) Rename file to 'level {:03X}.mwl', keep pointing at level {:X}\n\r"
						"(b) Repoint file to other level\n\r"
						"(c) Abort",
							path.string(), mwl_level_number.value(), mwl_level_number.value(), mwl_level_number.value()
					));
					std::cin >> answer;
					std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
					if (answer == 'a') {
						const auto target_path{ path.parent_path() / fmt::format("level {:03X}.mwl", mwl_level_number.value()) };
						if (fs::exists(target_path)) {
							throw InsertionException(fmt::format(
								colors::EXCEPTION,
								"Failed to rename '{}', '{}' already exists",
								path.filename().string(), target_path.filename().string()
							));
						}
						try {
							fs::rename(path, target_path);
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								colors::EXCEPTION,
								"Failed to rename level file '{}' to '{}' with exception:\n\r{}",
								path.string(), target_path.string(),
								e.what()
							));
						}
						spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, 
							"Successfully renamed '{}' to '{}'", path.filename().string(), target_path.filename().string()));
						continue;
					}
					else if (answer == 'b') {
						std::string s;
						while (true) {
							spdlog::warn(fmt::format(
								colors::WARNING,
								"What level number should '{}' be made to point to? Enter a level number from 0-1FF or enter 'x' to abort",
								path.string()
							));
							std::getline(std::cin, s);
							std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

							if (s == "x") {
								throw InsertionException(fmt::format(
									colors::EXCEPTION,
									"Level file '{}' (pointing to level {:X}) has malformed filename",
									path.string(), mwl_level_number.value()
								));
							}
							else {
								uint16_t level_number{};
								size_t parsed{};
								try {
									level_number = std::stoi(s, &parsed, 16);
								}
								catch (...) {
									spdlog::error(fmt::format(colors::EXCEPTION, "{} is not a valid level number", s));
									continue;
								}

								if (level_number >= 0x200 || parsed != s.size()) {
									spdlog::error(fmt::format(colors::EXCEPTION, "{} is not a valid level number", s));
									continue;
								}

								const auto target_path{ path.parent_path() / fmt::format("level {:03X}.mwl", level_number) };
								if (fs::exists(target_path)) {
									spdlog::error(fmt::format(colors::EXCEPTION, 
										"A level file for number {:03X} already exists at '{}'", level_number, target_path.string()));
									continue;
								}

								try {
									fs::rename(path, target_path);
								}
								catch (const std::exception& e) {
									throw InsertionException(fmt::format(
										colors::EXCEPTION,
										"Failed to rename level file '{}' to '{}' with exception:\n\r{}",
										path.string(), target_path.string(),
										e.what()
									));
								}
								spdlog::info(fmt::format(colors::PARTIAL_SUCCESS,
									"Successfully renamed '{}' to '{}'", path.filename().string(), target_path.filename().string()));

								try {
									setInternalLevelNumber(target_path, level_number);
								}
								catch (const std::exception& e) {
									throw InsertionException(fmt::format(
										colors::EXCEPTION,
										"Failed to set source level number of level file '{}' to {:X} with exception:\n\r{}",
										target_path.string(), level_number,
										e.what()
									));
								}
								spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, 
									"Successfully changed internal level number of '{}' to {:03X}", target_path.filename().string(), level_number));
								break;
							}
						}
					}
					else {
						throw InsertionException(fmt::format(
							colors::EXCEPTION,
							"Level file '{}' (pointing to level {:X}) has malformed filename",
							path.string(), mwl_level_number.value()
						));
					}
					continue;
				}
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Level file '{}' (pointing to level {:X}) has malformed filename",
					path.string(), mwl_level_number.value()
				));
			}

			if (file_level_number != mwl_level_number) {
				if (allow_user_input) {
					char answer;
					spdlog::warn(fmt::format(colors::WARNING, 
						"Level file '{}' points to level {:X} but filename says it should point to level {:X}, what should be done?\n\r"
						"(a) Keep filename '{}', but point it at level {:X}\n\r"
						"(b) Rename file to 'level {:03X}.mwl', keep pointing at level {:X}\n\r"
						"(c) Abort",
						path.string(), mwl_level_number.value(), file_level_number.value(),
						path.filename().string(), file_level_number.value(),
						mwl_level_number.value(), mwl_level_number.value()
					));
					std::cin >> answer;
					std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
					if (answer == 'a' || answer == 'A') {
						try {
							setInternalLevelNumber(entry, file_level_number.value());
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								colors::EXCEPTION,
								"Failed to set source level number of level file '{}' to {:X} with exception:\n\r{}",
								path.string(), file_level_number.value(),
								e.what()
							));
						}
						spdlog::info(fmt::format(colors::PARTIAL_SUCCESS,
							"Successfully set source level number of '{}' to {:X}", path.filename().string(), file_level_number.value()));
						continue;
					}
					else if (answer == 'b' || answer == 'B') {
						const auto target_path{ path.parent_path() / fmt::format("level {:03X}.mwl", mwl_level_number.value()) };
						if (fs::exists(target_path)) {
							throw InsertionException(fmt::format(
								colors::EXCEPTION,
								"Failed to rename '{}', '{}' already exists",
								path.filename().string(), target_path.filename().string()
							));
						}
						try {
							fs::rename(entry, target_path);
						}
						catch (const std::exception& e) {
							throw InsertionException(fmt::format(
								colors::EXCEPTION,
								"Failed to rename level file '{}' to '{}' with exception:\n\r{}",
								path.string(), target_path.string(),
								e.what()
							));
						}
						spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, 
							"Successfully renamed '{}' to '{}'", path.filename().string(), target_path.filename().string()));
						continue;
					}
				}
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Level file '{}' points to level {:X} but filename says it should point to level {:X}",
					path.string(), mwl_level_number.value(), file_level_number.value()
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

		if (mwl_name.size() != 9) {
			return {};
		}

		if (!std::all_of(mwl_name.begin() + 6, mwl_name.end(), [](const auto& e) { return std::isalnum(e); })) {
			return {};
		}

		try {
			size_t parsed{};
			const auto level_number{ std::stoi(mwl_name.substr(6, 3), &parsed, 16) };
			if (parsed != 3) {
				return {};
			}
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
		std::unordered_set<ResourceDependency> dependencies{};

		const auto mwl_file_dependencies{ getResourceDependenciesFor(levels_folder, Policy::REINSERT) };

		dependencies.insert(mwl_file_dependencies.begin(), mwl_file_dependencies.end());
		dependencies.emplace(levels_folder);

		return dependencies;
	}

	void Levels::insert() {
		spdlog::info(fmt::format(colors::RESOURCE, "Inserting levels"));

		bool at_least_one_level{ false };
		for (const auto& entry : fs::directory_iterator(levels_folder)) {
			if (entry.path().extension() == ".mwl") {
				at_least_one_level = true;
				break;
			}
		}

		if (!at_least_one_level) {
			spdlog::info(fmt::format(colors::NOTIFICATION, "No levels to insert, skipping level insertion"));
			return;
		}

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
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully inserted levels"));
			spdlog::debug(fmt::format(
				"Successfully inserted levels from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Failed to insert level from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
	}
}
