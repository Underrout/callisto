#include "patch.h"

namespace callisto {
	Patch::Patch(const Configuration& config, const fs::path& patch_path,
		const std::vector<fs::path>& additional_include_paths)
		: RomInsertable(config), 
		project_relative_path(fs::relative(patch_path, registerConfigurationDependency(config.project_root).getOrThrow())),
		patch_path(patch_path),
		additional_include_paths(additional_include_paths),
		disable_deprecation_warnings(config.disable_deprecation_warnings.getOrDefault(false))
	{

	}

	void Patch::insert() {
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

		const auto prev_folder{ fs::current_path() };
		fs::current_path(patch_path.parent_path());

		spdlog::info(fmt::format(colors::RESOURCE, "Applying patch {}", project_relative_path.string()));

		// delete potential previous dependency report
		fs::remove(patch_path.parent_path() / ".dependencies");

		const auto str_patch_path{ patch_path.string() };

		const auto rom_size{ fs::file_size(temporary_rom_path) };
		const auto header_size{ (int)rom_size & 0x7FFF };
		int unheadered_rom_size{ (int)rom_size - header_size };

		std::vector<char> rom_bytes(MAX_ROM_SIZE);
		std::vector<char> header(header_size);
		std::ifstream rom_file(temporary_rom_path, std::ios::binary);
		rom_file.read(reinterpret_cast<char*>(header.data()), header_size);
		rom_file.read(reinterpret_cast<char*>(rom_bytes.data()), unheadered_rom_size);
		rom_file.close();

		spdlog::debug(fmt::format(
			"Applying patch {} to temporary ROM {}:\n\r"
			"\tROM size:\t\t{}\n\r"
			"\tROM header size:\t\t{}\n\r",
			patch_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		std::vector<definedata> defines{};
		defines.emplace_back("CALLISTO_ASSEMBLING", "1");
		defines.emplace_back("CALLISTO_INSERTION_TYPE", "Patch");

		std::vector<warnsetting> warn_settings{};

		warnsetting disable_relative_path_warning;

		if (asar_version() < 10900) {
			disable_relative_path_warning.warnid = "1001";
		}
		else {
			disable_relative_path_warning.warnid = "Wrelative_path_used";
		}
		disable_relative_path_warning.enabled = false;

		warn_settings.push_back(disable_relative_path_warning);

		warnsetting disable_deprecation;

		if (asar_version() >= 10900 && disable_deprecation_warnings) {
			disable_deprecation.warnid = "Wfeature_deprecated";
			disable_deprecation.enabled = false;
			warn_settings.push_back(disable_deprecation);
		}

		std::vector<const char*> as_c_strs{};
		for (const auto& path : additional_include_paths) {
			auto c_str{ new char[path.string().size() + 1] };
			std::strcpy(c_str, path.string().c_str());
			as_c_strs.push_back(c_str);
		}

		const patchparams params{
			sizeof(struct patchparams),
			str_patch_path.c_str(),
			rom_bytes.data(),
			MAX_ROM_SIZE,
			&unheadered_rom_size,
			reinterpret_cast<const char**>(as_c_strs.data()),
			static_cast<int>(as_c_strs.size()),
			true,
			defines.data(),
			defines.size(),
			nullptr,
			nullptr,
			warn_settings.data(),
			static_cast<int>(warn_settings.size()),
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

		for (auto c_str : as_c_strs) {
			delete[] c_str;
		}

		as_c_strs.clear();

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
			out_rom.write(header.data(), header_size);
			out_rom.write(rom_bytes.data(), unheadered_rom_size);
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
