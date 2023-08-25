#include "levels.h"

namespace callisto {
	namespace extractables {
		Levels::Levels(const Configuration& config, const fs::path& extracting_rom, size_t max_thread_count)
			: LunarMagicExtractable(config, extracting_rom), levels_folder(config.levels.getOrThrow()), 
			temp_folder(config.temporary_folder.getOrThrow() / "chunked_roms"), max_thread_count(max_thread_count) {

			fs::create_directories(temp_folder);

			if (!fs::exists(levels_folder)) {
				spdlog::debug("Levels folder {} does not existing, creating it now", levels_folder.string());
				try {
					fs::create_directories(levels_folder);
				}
				catch (const fs::filesystem_error&) {
					throw ExtractionException(fmt::format(colors::EXCEPTION, "Failed to create levels directory {}", levels_folder.string()));
				}
			}
		}

		void Levels::normalize() const {
			spdlog::debug("Stripping source pointers from MWLs in directory {}", levels_folder.string());
			std::vector<fs::path> mwls{};
			for (const auto& entry : fs::directory_iterator(levels_folder)) {
				if (entry.path().extension() == ".mwl") {
					mwls.push_back(entry.path());
				}
			}

			std::exception_ptr thread_exception{};
			std::for_each(std::execution::par, mwls.begin(), mwls.end(), [&](auto&& path) { 
				try {
					Level::normalize(path); 
				}
				catch (...) {
					thread_exception = std::current_exception();
				}
			});

			if (thread_exception != nullptr) {
				std::rethrow_exception(thread_exception);
			}
		}

		std::vector<size_t> Levels::determineModifiedOffsets(const fs::path& extracting_rom) {
			const auto rom_size{ fs::file_size(extracting_rom) };
			const auto header_size{ rom_size & 0x7FFF };

			std::ifstream rom_file{ extracting_rom };
			rom_file.seekg(header_size + LAYER_1_POINTERS_OFFSET + LAYER_1_POINTER_SIZE - 1);

			std::vector<size_t> modified_offsets{};
			auto curr_position{ header_size + LAYER_1_POINTERS_OFFSET };
			for (auto i{ 0 }; i != LAYER_1_POINTERS_AMOUNT; ++i) {
				char bank_byte;
				rom_file.read(&bank_byte, 1);
				if (static_cast<uint8_t>(bank_byte) >= 0x10) {
					modified_offsets.push_back(curr_position);
				}

				curr_position += LAYER_1_POINTER_SIZE;
				rom_file.seekg(LAYER_1_POINTER_SIZE - 1, std::ios_base::cur);
			}

			rom_file.close();

			return modified_offsets;
		}

		fs::path Levels::createChunkedRom(const fs::path& temp_folder, size_t chunk_idx, size_t chunk_amount, 
			const fs::path& extracting_rom, const std::vector<size_t>& offsets) {

			fs::path temp_rom_path{ (temp_folder / extracting_rom.stem()).string() + '_' 
				+ std::to_string(chunk_idx) + extracting_rom.extension().string()};

			fs::copy(extracting_rom, temp_rom_path, fs::copy_options::overwrite_existing);

			const auto levels_per_chunk{ (int)((double)offsets.size() / chunk_amount + 0.5) };  // truncating intentionally
			const auto skip_start_offset{ levels_per_chunk * chunk_idx };
			const auto skip_end_offset{ chunk_idx == chunk_amount - 1 ? offsets.size() : skip_start_offset + levels_per_chunk };
			size_t curr_offset{ 0 };

			std::ofstream rom_file(temp_rom_path, std::ios::in || std::ios::out || std::ios::binary);
			while (true) {
				if (curr_offset == skip_start_offset) {
					curr_offset = skip_end_offset;
				}

				if (curr_offset == offsets.size()) {
					break;
				}

				const auto modified_offset{ offsets[curr_offset++] };

				rom_file.seekp(modified_offset);
				for (size_t i{ 0 }; i != LAYER_1_POINTER_SIZE; ++i) {
					rom_file.put(0x00); // zero out layer 1 data pointer, seems to work fine even though this is of course not a valid pointer
				}
				rom_file.seekp(modified_offset + 0x600 + 2);
				rom_file.put(0xFF);
			}
			rom_file.close();

			return temp_rom_path;
		}

		void Levels::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting levels"));

			fs::path temporary_levels_folder{ levels_folder.string() + "_temp" };
			fs::remove_all(temporary_levels_folder);
			fs::create_directories(temporary_levels_folder);

			spdlog::debug("Exporting levels to temporary folder {} from ROM {}", levels_folder.string(), extracting_rom.string());

			const auto modified_offsets{ determineModifiedOffsets(extracting_rom) };
			std::exception_ptr thread_exception{};
			std::vector<std::jthread> export_threads{};
			std::vector<int> exit_codes(max_thread_count, 0);

			for (size_t i{ 0 }; i != max_thread_count; ++i) {
				export_threads.emplace_back([=, &exit_codes, &modified_offsets, &thread_exception] {
					try {
						const auto temp_rom{ createChunkedRom(temp_folder, i,
							max_thread_count, extracting_rom, modified_offsets) };
						const auto exit_code{ callLunarMagic("-ExportMultLevels",
						temp_rom.string(), (temporary_levels_folder / "level").string()) };
						exit_codes[i] = exit_code;
						fs::remove(temp_rom);
					}
					catch (...) {
						thread_exception = std::current_exception();
					}
				});
			}

			for (auto& thread : export_threads) {
				thread.join();
			}

			if (thread_exception != nullptr) {
				std::rethrow_exception(thread_exception);
			}

			if (std::all_of(exit_codes.begin(), exit_codes.end(), [](auto e) { return e == 0; })) {
				spdlog::debug("Copying temporary folder {} to {}", temporary_levels_folder.string(), levels_folder.string());

				fs::remove_all(levels_folder);
				fs::copy(temporary_levels_folder, levels_folder);
				fs::remove_all(temporary_levels_folder);

				if (strip_source_pointers) {
					normalize();
				}
				spdlog::info("Successfully exported levels!");
			}
			else {
				fs::remove_all(temporary_levels_folder);
				throw ExtractionException(fmt::format(
					"Failed to export levels from ROM {} to directory {}",
					extracting_rom.string(), levels_folder.string()
				));
			}
		}
	}
}
