#include "patch.h"

namespace stardust {
	Patch::Patch(const fs::path& project_root_path, const fs::path& temporary_rom_path, const fs::path& patch_path,
		const std::vector<fs::path>& additional_include_paths)
		: RomInsertable(temporary_rom_path), project_relative_path(fs::relative(patch_path, project_root_path)),
		patch_path(patch_path) 
	{
		// delete potential previous dependency report
		fs::remove(patch_path.parent_path() / ".dependencies");

		if (!fs::exists(patch_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Patch {} does not exist",
				patch_path.string()
			));
		}

		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside stardust?"
			);
		}

		this->additional_include_paths.reserve(additional_include_paths.size());
		std::transform(additional_include_paths.begin(), additional_include_paths.end(),
			std::back_inserter(this->additional_include_paths),
			[](const fs::path& path) {
				char* copy{ new char[path.string().size()]};
				std::strcpy(copy, path.string().c_str());
				return copy;
		});
	}

	void Patch::insert() {
		spdlog::info(fmt::format("Applying patch {}", project_relative_path.string()));

		std::ifstream rom_file(temporary_rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };
		const auto str_patch_path{ patch_path.string() };

		int unheadered_rom_size{ rom_size - header_size };

		spdlog::debug(fmt::format(
			"Applying patch {} to temporary ROM {}:\n"
			"\tROM size:\t\t{}\n"
			"\tROM header size:\t\t{}\n",
			patch_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		const patchparams params{
			sizeof(patchparams),
			str_patch_path.c_str(),
			rom_bytes.data() + header_size,
			MAX_ROM_SIZE,
			&unheadered_rom_size,
			reinterpret_cast<const char**>(additional_include_paths.data()),
			static_cast<int>(additional_include_paths.size()),
			true,
			nullptr,
			0,
			nullptr,
			nullptr,
			nullptr,
			0,
			nullptr,
			0,
			false,
			true
		};

		// need to call this again since PIXI might have called asar_close
		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside stardust?"
			);
		}

		asar_reset();
		const bool succeeded{ asar_patch_ex(&params) };

		if (succeeded) {
			int warning_count;
			const auto warnings{ asar_getwarnings(&warning_count) };
			for (int i = 0; i != warning_count; ++i) {
				spdlog::warn(warnings[i].fullerrdata);
			}
			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
			spdlog::info(fmt::format("Successfully applied patch {}!", project_relative_path.string()));

			std::vector<writtenblockdata> blocks{};
			int num_blocks;
			const auto asar_blocks{ asar_getwrittenblocks(&num_blocks) };
			for (int i{ 0 }; i != num_blocks; ++i) {
				blocks.push_back(asar_blocks[i]);
			}
			written_blocks = blocks;
		}
		else {
			int error_count;
			const auto errors{ asar_geterrors(&error_count) };
			std::ostringstream error_string{};
			for (int i = 0; i != error_count; ++i) {
				if (i != 0) {
					error_string << std::endl;
				}

				error_string << errors[i].fullerrdata;
			}
			throw InsertionException(fmt::format(
				"Failed to apply patch {} with the following error(s):\n{}",
				project_relative_path.string(),
				error_string.str()
			));
		}
	}

	const std::vector<writtenblockdata>& Patch::getWrittenBlocks() const {
		return written_blocks;
	}

	std::unordered_set<Dependency> Patch::determineDependencies() {
		const auto dependencies{ Insertable::extractDependenciesFromReport(
			patch_path.parent_path() / ".dependencies"
		) };
		return dependencies;
	}
}
