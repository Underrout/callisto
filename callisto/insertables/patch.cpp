#include "patch.h"

namespace callisto {
	Patch::Patch(const Configuration& config, const fs::path& patch_path,
		const std::vector<fs::path>& additional_include_paths)
		: RomInsertable(config), 
		project_relative_path(fs::relative(patch_path, registerConfigurationDependency(config.project_root).getOrThrow())),
		patch_path(patch_path) 
	{
		if (!fs::exists(patch_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Patch {} does not exist",
				patch_path.string()
			));
		}

		if (!asar_init()) {
			throw ToolNotFoundException(
				fmt::format(colors::EXCEPTION,
				"Asar library file not found, did you forget to copy it alongside callisto?"
			));
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
		const auto prev_folder{ fs::current_path() };
		fs::current_path(patch_path.parent_path());

		spdlog::info(fmt::format(colors::RESOURCE, "Applying patch {}", project_relative_path.string()));

		// delete potential previous dependency report
		fs::remove(patch_path.parent_path() / ".dependencies");

		std::ifstream rom_file(temporary_rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };
		const auto str_patch_path{ patch_path.string() };

		int unheadered_rom_size{ rom_size - header_size };

		spdlog::debug(fmt::format(
			"Applying patch {} to temporary ROM {}:\n\r"
			"\tROM size:\t\t{}\n\r"
			"\tROM header size:\t\t{}\n\r",
			patch_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		const definedata callisto_define{
			"CALLISTO_ASSEMBLING",
			"1"
		};

		const patchparams params{
			sizeof(struct patchparams),
			str_patch_path.c_str(),
			rom_bytes.data() + header_size,
			MAX_ROM_SIZE,
			&unheadered_rom_size,
			reinterpret_cast<const char**>(additional_include_paths.data()),
			static_cast<int>(additional_include_paths.size()),
			true,
			&callisto_define,
			1,
			nullptr,
			nullptr,
			nullptr,
			0,
			nullptr,
			0,
			true,
			false
		};

		if (!asar_init()) {
			throw ToolNotFoundException(
				fmt::format(colors::EXCEPTION,
				"Asar library file not found, did you forget to copy it alongside callisto?"
			));
		}

		const bool succeeded{ asar_patch_ex(&params) };

		int print_count;
		const auto prints{ asar_getprints(&print_count) };
		for (int i = 0; i != print_count; ++i) {
			spdlog::info(prints[i]);
		}

		fs::current_path(prev_folder);

		if (succeeded) {
			int warning_count;
			const auto warnings{ asar_getwarnings(&warning_count) };
			for (int i = 0; i != warning_count; ++i) {
				spdlog::warn(warnings[i].fullerrdata);
			}
			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();

			int written_block_count;
			const auto written_blocks{ asar_getwrittenblocks(&written_block_count) };

			for (size_t i{ 0 }; i != written_block_count; ++i) {
				const auto& block{ written_blocks[i] };
				if (block.pcoffset < 0x80000) {
					hijacks.push_back({ block.pcoffset, block.numbytes });
				}
			}

			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully applied patch {}!", project_relative_path.string()));
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
				colors::EXCEPTION,
				"Failed to apply patch {} with the following error(s):\n\r{}",
				project_relative_path.string(),
				error_string.str()
			));
		}
	}

	const std::vector<std::pair<size_t, size_t>>& Patch::getHijacks() const {
		return hijacks;
	}

	std::unordered_set<ResourceDependency> Patch::determineDependencies() {
		auto dependencies{ Insertable::extractDependenciesFromReport(
			patch_path.parent_path() / ".dependencies"
		) };
		dependencies.insert(ResourceDependency(patch_path, Policy::REINSERT));
		return dependencies;
	}
}
