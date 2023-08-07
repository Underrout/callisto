#include "module.h"

namespace callisto {
	Module::Module(const Configuration& config,
		const fs::path& input_path,
		const fs::path& callisto_asm_file,
		const std::unordered_set<int>& other_module_addresses,
		const std::vector<fs::path>& additional_include_paths) :
		RomInsertable(config), 
		input_path(input_path),
		output_paths(registerConfigurationDependency(config.module_configurations.at(input_path).real_output_paths, Policy::REINSERT).getOrThrow()),
		project_relative_path(fs::relative(input_path, registerConfigurationDependency(config.project_root).getOrThrow())),
		callisto_asm_file(callisto_asm_file),
		real_module_folder_location(PathUtil::getUserModuleDirectoryPath(config.project_root.getOrThrow())),
		cleanup_folder_location(PathUtil::getModuleCleanupDirectoryPath(config.project_root.getOrThrow())),
		other_module_addresses(other_module_addresses),
		module_header_file(registerConfigurationDependency(config.module_header, Policy::REINSERT).isSet() ? 
			std::make_optional(config.module_header.getOrThrow()) : std::nullopt)
	{
		if (!fs::exists(input_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Module {} does not exist",
				input_path.string()
			));
		}

		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside callisto?"
			);
		}

		for (const auto& output_path : output_paths) {
			fs::create_directories(output_path.parent_path());
		}

		this->additional_include_paths.reserve(additional_include_paths.size());
		std::transform(additional_include_paths.begin(), additional_include_paths.end(),
			std::back_inserter(this->additional_include_paths),
			[](const fs::path& path) {
				char* copy{ new char[path.string().size()] };
		std::strcpy(copy, path.string().c_str());
		return copy;
			});
	}

	void Module::init() {
		std::ostringstream temp_patch{};

		temp_patch << "warnings disable W1011\n"
			<< "if read1($00FFD5) == $23\nsa1rom\nelse\nlorom\nendif\n";

		if (input_path.extension() == ".asm") {
			if (module_header_file.has_value()) {
				temp_patch << "incsrc \"" << PathUtil::convertToPosixPath(module_header_file.value()).string() << '"' << std::endl << std::endl;
			}

			temp_patch << "incsrc \"" << PathUtil::convertToPosixPath(input_path).string() << '"' << std::endl;
		}
		else {
			temp_patch << fmt::format("incbin \"{}\" -> {}", PathUtil::convertToPosixPath(input_path).string(), PLACEHOLDER_LABEL) << std::endl;
		}

		patch_string = temp_patch.str();
	}

	// TODO this whole thing is pretty much the same as the Patch.insert() method, probably merge 
	//      them into a single static method or something later
	void Module::insert() {
		const auto prev_folder{ fs::current_path() };
		fs::current_path(input_path.parent_path());

		// delete potential previous dependency report
		fs::remove(input_path.parent_path() / ".dependencies");

		spdlog::info(fmt::format("Inserting module {}", project_relative_path.string()));

		memoryfile patch;
		patch.path = "temp.asm";
		patch.buffer = patch_string.c_str();
		patch.length = patch_string.size();

		std::ifstream rom_file(temporary_rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		spdlog::debug(fmt::format(
			"Applying module {} to temporary ROM {}:\n\r"
			"\tROM size:\t\t{}\n\r"
			"\tROM header size:\t\t{}\n\r",
			input_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		warnsetting disable_relative_path_warning;
		disable_relative_path_warning.warnid = "1001";
		disable_relative_path_warning.enabled = false;

		const patchparams params{
			sizeof(struct patchparams),
			"temp.asm",
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
			&disable_relative_path_warning,
			1,
			&patch,
			1,
			true,
			false
		};

		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside callisto?"
			);
		}

		asar_reset();
		const bool succeeded{ asar_patch_ex(&params) };

		int print_count;
		const auto prints{ asar_getprints(&print_count) };
		for (int i = 0; i != print_count; ++i) {
			spdlog::info(prints[i]);
		}

		if (succeeded) {
			int warning_count;
			const auto warnings{ asar_getwarnings(&warning_count) };
			bool missing_org_or_freespace{ false };
			for (int i = 0; i != warning_count; ++i) {
				spdlog::warn(warnings[i].fullerrdata);
				if (warnings[i].errid == 1008) {
					missing_org_or_freespace = true;
				}
			}

			if (missing_org_or_freespace) {
				throw InsertionException(fmt::format(
					"Module {} is missing a freespace command",
					project_relative_path.string()
				));
			}

			recordOurAddresses();
			verifyNonHijacking();
			verifyWrittenBlockCoverage();

			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
			spdlog::info(fmt::format("Successfully applied module {}!", project_relative_path.string()));

			emitOutputFiles();
			emitPlainAddressFile();

			fs::current_path(prev_folder);
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

			fs::current_path(prev_folder);
			throw InsertionException(fmt::format(
				"Failed to apply module {} with the following error(s):\n\r{}",
				project_relative_path.string(),
				error_string.str()
			));
		}
	}

	std::string Module::modulePathToName(const fs::path& path) {
		auto prefix{ path.stem().string() };
		std::replace(prefix.begin(), prefix.end(), ' ', '_');

		return prefix;
	}

	void Module::emitOutputFiles() const {
		for (const auto& output_path : output_paths) {
			emitOutputFile(output_path);
		}
	}

	void Module::emitOutputFile(const fs::path& output_path) const {
		std::ofstream real_output_file{ output_path };

		real_output_file << "includeonce\n\n";

		real_output_file << fmt::format("incsrc \"{}\"\n\n", PathUtil::convertToPosixPath(callisto_asm_file).string());

		int label_number{};
		const auto labels{ asar_getalllabels(&label_number) };

		const auto module_name{ modulePathToName(output_path) };

		if (label_number == 0) {
			throw InsertionException(fmt::format(
				"Module {} contains no labels, this will cause a freespace leak, please ensure your module contains at least one label",
				module_name
			));
		}

		if (input_path.extension() != ".asm") {
			if (label_number > 1) {
				throw InsertionException(fmt::format(
					"Binary module {} unexpectedly contains more than one label",
					module_name
				));
			}

			const auto& label{ labels[0] };
			const auto name{ std::string(label.name) };

			real_output_file << fmt::format("{} = ${:06X}", module_name, label.location) << std::endl;
			real_output_file << fmt::format("!{} = ${:06X}", module_name, label.location) << std::endl;
		}
		else {
			for (int i{ 0 }; i != label_number; ++i) {
				const auto& label{ labels[i] };
				const auto name{ std::string(label.name) };

				if (name.at(0) == ':') {
					// it's a relative label (+, -, ++, ...), skip it
					continue;
				}

				if (name.find('.') != std::string::npos) {
					// it's a struct field (Struct.field), skip it
					continue;
				}

				if (other_module_addresses.contains(label.location)) {
					// label belongs to imported module, skip it
					continue;
				}

				real_output_file << fmt::format("{}_{} = ${:06X}", module_name, name, label.location) << std::endl;
				real_output_file << fmt::format("!{}_{} = ${:06X}", module_name, name, label.location) << std::endl;
			}
		}
	}

	void Module::emitPlainAddressFile() const {
		const auto cleanup_file_path{ 
			cleanup_folder_location / ((project_relative_path.parent_path() / project_relative_path.stem()).string() + ".addr")};

		fs::create_directories(cleanup_file_path.parent_path());

		std::ofstream cleanup_file{ cleanup_file_path };

		for (const auto& address : our_module_addresses) {
			cleanup_file << fmt::format("{}\n", address);
		}
	}

	std::unordered_set<ResourceDependency> Module::determineDependencies() {
		if (input_path.extension() == ".asm") {
			auto dependencies{ Insertable::extractDependenciesFromReport(
				input_path.parent_path() / ".dependencies"
			) };
			if (module_header_file.has_value()) {
				dependencies.insert(ResourceDependency(module_header_file.value(), Policy::REINSERT));
			}
			dependencies.insert(ResourceDependency(input_path, Policy::REINSERT));
			return dependencies;
		}
		else {
			fs::remove(input_path.parent_path() / ".dependencies");
			return { ResourceDependency(input_path, Policy::REINSERT) };
		}
		return {};
	}

	void Module::recordOurAddresses() {
		int label_count{};
		const auto labels{ asar_getalllabels(&label_count) };

		for (int i{ 0 }; i != label_count; ++i) {
			our_module_addresses.insert(labels[i].location);
		}
	}

	void Module::verifyWrittenBlockCoverage() const {
		int label_count{};
		const auto labels{ asar_getalllabels(&label_count) };

		int block_count{};
		const auto written_blocks{ asar_getwrittenblocks(&block_count) };
		for (int i{ 0 }; i != block_count; ++i) {
			const auto start{ written_blocks[i].snesoffset };

			// this is probably imprecise and includes parts of the RATS tag, but who cares
			const auto end{ start + written_blocks[i].numbytes };

			bool is_covered{ false };
			for (int j{ 0 }; j != label_count; ++j) {
				// uggo but labels can have the bank byte be | $80 or not depending on how the user does things I think?
				// not sure how this affects sa1 ROMs but I'm guessing it's a niche issue if anything (hopefully not wrong)
				const auto location_low{ labels[j].location };
				const auto location_high{ labels[j].location | 0x800000 };
				if ((location_low >= start && location_low < end) || (location_high >= start && location_high < end)) {
					is_covered = true;
					break;
				}
			}

			if (!is_covered) {
				throw InsertionException(fmt::format(
					"Module {} contains at least one freespace block that does not contain any labels and thus cannot be cleaned up, "
					"please ensure every freespace block in your module contains at least one label",
					project_relative_path.string()
				));
			}
		}
	}

	void Module::verifyNonHijacking() const {
		int block_count{};
		const auto written_blocks{ asar_getwrittenblocks(&block_count) };
		for (int i{ 0 }; i != block_count; ++i) {
			const auto start{ written_blocks[i].pcoffset };

			if (start < 0x80000) {
				throw InsertionException(fmt::format(
					"Module {} targets SNES address ${:06X} (unheadered), if this is not a mistake consider using a patch instead "
					"as modules are not intended to modify original game code",
					project_relative_path.string(), written_blocks[i].snesoffset
				));
			}
		}
	}
}
