#include "module.h"

namespace callisto {
	Module::Module(const Configuration& config,
		const fs::path& input_path,
		const fs::path& callisto_asm_file,
		std::shared_ptr<std::unordered_set<int>> current_module_addresses,
		int id,
		const std::vector<fs::path>& additional_include_paths) :
		RomInsertable(config), 
		input_path(input_path),
		output_paths(config.module_configurations.at(input_path).real_output_paths.getOrThrow()),
		project_relative_path(fs::relative(input_path, registerConfigurationDependency(config.project_root).getOrThrow())),
		callisto_asm_file(callisto_asm_file),
		real_module_folder_location(PathUtil::getUserModuleDirectoryPath(config.project_root.getOrThrow())),
		cleanup_folder_location(PathUtil::getModuleCleanupDirectoryPath(config.project_root.getOrThrow())),
		current_module_addresses(current_module_addresses),
		additional_include_paths(additional_include_paths),
		id(id),
		module_header_file(registerConfigurationDependency(config.module_header, Policy::REINSERT).isSet() ? 
			std::make_optional(config.module_header.getOrThrow()) : std::nullopt)
	{
		for (const auto& output_path : output_paths) {
			fs::create_directories(output_path.parent_path());
		}
	}

	void Module::init() {
		std::ostringstream temp_patch{};

		temp_patch << "if !assembler_ver < 10900\nwarnings disable W1011\nelse\nwarnings disable Wfreespace_leaked\nendif\n"
			<< "if read1($00FFD5) == $23\nsa1rom\nelse\nlorom\nendif\n";

		if (input_path.extension() == ".asm") {
			if (module_header_file.has_value()) {
				temp_patch << "incsrc \"" << PathUtil::sanitizeForAsar(
					PathUtil::convertToPosixPath(module_header_file.value())).string() << '"' << std::endl << std::endl;
			}

			temp_patch << "incsrc \"" << PathUtil::sanitizeForAsar(PathUtil::convertToPosixPath(input_path)).string() << '"' << std::endl;
		}
		else {
			temp_patch << fmt::format("incbin \"{}\" -> {}", PathUtil::sanitizeForAsar(
				PathUtil::convertToPosixPath(input_path)).string(), PLACEHOLDER_LABEL) << std::endl;
		}

		patch_string = temp_patch.str();
	}

	// TODO this whole thing is pretty much the same as the Patch.insert() method, probably merge 
	//      them into a single static method or something later
	void Module::insert() {
		if (!fs::exists(input_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Module {} does not exist",
				input_path.string()
			));
		}

		if (!asar_init()) {
			throw ToolNotFoundException(
				fmt::format(colors::EXCEPTION,
					"Asar library file not found, did you forget to copy it alongside callisto?"
				));
		}

		const auto prev_folder{ fs::current_path() };
		fs::current_path(temporary_rom_path.parent_path());

		// delete potential previous dependency report
		fs::remove(temporary_rom_path.parent_path() / ".dependencies");

		spdlog::info(fmt::format(colors::RESOURCE, "Inserting module {}", project_relative_path.string()));

		memoryfile patch;
		patch.path = "temp.asm";
		patch.buffer = patch_string.c_str();
		patch.length = patch_string.size();

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
			"Applying module {} to temporary ROM {}:\n\r"
			"\tROM size:\t\t{}\n\r"
			"\tROM header size:\t\t{}\n\r",
			input_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		warnsetting disable_relative_path_warning;

		if (asar_version() < 10900) {
			disable_relative_path_warning.warnid = "1001";
		}
		else {
			disable_relative_path_warning.warnid = "Wrelative_path_used";
		}
		disable_relative_path_warning.enabled = false;

		std::vector<definedata> defines{};
		defines.emplace_back("CALLISTO_ASSEMBLING", "1");
		defines.emplace_back("CALLISTO_INSERTION_TYPE", "Module");

		std::vector<const char*> as_c_strs{};
		for (const auto& path : additional_include_paths) {
			auto c_str{ new char[path.string().size() + 1] };
			std::strcpy(c_str, path.string().c_str());
			as_c_strs.push_back(c_str);
		}

		const patchparams params{
			sizeof(struct patchparams),
			"temp.asm",
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
			&disable_relative_path_warning,
			1,
			&patch,
			1,
			true,
			false
		};

		if (!asar_init()) {
			throw ToolNotFoundException(
				fmt::format(colors::EXCEPTION,
				"Asar library file not found, did you forget to copy it alongside callisto?"
			));
		}

		asar_reset();
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

		if (succeeded) {
			int warning_count;
			const auto warnings{ asar_getwarnings(&warning_count) };
			bool missing_org_or_freespace{ false };
			for (int i = 0; i != warning_count; ++i) {
				spdlog::warn(fmt::format(colors::WARNING, warnings[i].fullerrdata));
				if (warnings[i].errid == 1008) {
					missing_org_or_freespace = true;
				}
			}

			if (missing_org_or_freespace) {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Module '{}' is missing a freespace command",
					project_relative_path.string()
				));
			}

			recordOurAddresses();
			verifyNonHijacking();
			
			verifyWrittenBlockCoverage(rom_bytes);

			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(header.data(), header_size);
			out_rom.write(rom_bytes.data(), unheadered_rom_size);
			out_rom.close();
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully applied module {}!", project_relative_path.string()));

			emitOutputFiles();
			emitPlainAddressFile();

			current_module_addresses->insert(our_module_addresses.begin(), our_module_addresses.end());

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
				colors::EXCEPTION,
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

		auto name{ output_path.string() };
		real_output_file << fmt::format("if not(defined(\"CALLISTO_MODULE_{}\"))\n\n!CALLISTO_MODULE_{} = 1\n\n", id, id);

		real_output_file << fmt::format("incsrc \"{}\"\n\n", PathUtil::sanitizeForAsar(PathUtil::convertToPosixPath(callisto_asm_file)).string());

		int label_number{};
		const auto labels{ asar_getalllabels(&label_number) };

		const auto module_name{ modulePathToName(output_path) };

		if (label_number == 0) {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Module {} contains no labels, this will cause a freespace leak, please ensure your module contains at least one label",
				module_name
			));
		}

		if (input_path.extension() != ".asm") {
			if (label_number > 1) {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
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

				if (current_module_addresses->contains(label.location)) {
					// label belongs to imported module, skip it
					continue;
				}

				real_output_file << fmt::format("{}_{} = ${:06X}", module_name, name, label.location) << std::endl;
				real_output_file << fmt::format("!{}_{} = ${:06X}", module_name, name, label.location) << std::endl;
			}
		}

		real_output_file << "\nendif\n";
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
				temporary_rom_path.parent_path() / ".dependencies"
			) };
			fs::remove(temporary_rom_path.parent_path() / ".dependencies");
			if (module_header_file.has_value()) {
				dependencies.insert(ResourceDependency(module_header_file.value(), Policy::REINSERT));
			}
			dependencies.insert(ResourceDependency(input_path, Policy::REINSERT));
			return dependencies;
		}
		else {
			fs::remove(temporary_rom_path.parent_path() / ".dependencies");
			return { ResourceDependency(input_path, Policy::REINSERT) };
		}
		return {};
	}

	void Module::recordOurAddresses() {
		int label_count{};
		const auto labels{ asar_getalllabels(&label_count) };

		for (int i{ 0 }; i != label_count; ++i) {
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

			if (current_module_addresses->contains(label.location)) {
				// label belongs to imported module, skip it
				continue;
			}

			our_module_addresses.insert(label.location);
		}
	}

	void Module::verifyWrittenBlockCoverage(const std::vector<char>& rom) const {
		int label_count{};
		const auto labels{ asar_getalllabels(&label_count) };

		int block_count{};
		const auto written_blocks{ asar_getwrittenblocks(&block_count) };
		const auto as_structs{ convertToWrittenBlockVector(written_blocks, block_count) };
		const auto freespace_areas{ convertToFreespaceAreas(as_structs, rom) };
		for (const auto& freespace_area : freespace_areas) {
			bool is_covered{ false };
			for (const auto& written_block : freespace_area) {
				for (int j{ 0 }; j != label_count; ++j) {
					// uggo but labels can have the bank byte be | $80 or not depending on how the user does things I think?
					// not sure how this affects sa1 ROMs but I'm guessing it's a niche issue if anything (hopefully not wrong)
					const auto location_low{ labels[j].location };
					const auto location_high{ labels[j].location | 0x800000 };
					if ((location_low >= written_block.start_snes && location_low < written_block.end_snes) 
						|| (location_high >= written_block.start_snes && location_high < written_block.end_snes)) {
						is_covered = true;
						break;
					}
				}
				if (is_covered) {
					break;
				}
			}

			if (!is_covered) {
				auto freespace_start{ freespace_area.front().start_snes + RATS_TAG_SIZE };
				if ((freespace_start & 0xFFFF) < 0x8000) {
					freespace_start += 0x8000;
				}

				const auto freespace_size{ std::accumulate(freespace_area.begin(), freespace_area.end(), 0, 
					[](int val1, const WrittenBlock& val2) {
					return val1 + val2.size;
				}) - RATS_TAG_SIZE };

				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Module {} contains a freespace block of size 0x{:04X} starting at ${:06X} (unheadered), which does not "
					"contain any labels, please ensure every freespace block in your modules contains at least one label so that they can be cleaned up by callisto",
					project_relative_path.string(),
					freespace_size, freespace_start
				));
			}
		}
	}

	std::optional<uint16_t> Module::determineFreespaceBlockSize(size_t pc_address, const std::vector<char>& rom) {
		char potential_tag[5];
		strncpy(potential_tag, rom.data() + pc_address, 4);
		potential_tag[4] = '\0';

		if (std::string(potential_tag) != std::string(RATS_TAG_TEXT)) {
			return {};
		}

		const auto potential_size{ (((unsigned char)rom[pc_address + 5]) << 8) | ((unsigned char)rom[pc_address + 4]) };
		const auto potential_size_complement{ (((unsigned char)rom[pc_address + 7]) << 8) | ((unsigned char)rom[pc_address + 6]) };

		if (potential_size + 1 != (potential_size_complement ^ 0xFFFF) + 1) {
			return {};
		}

		return potential_size + 1;
	}

	std::vector<Module::WrittenBlock> Module::convertToWrittenBlockVector(const writtenblockdata* const written_blocks, int block_count) {
		std::vector<WrittenBlock> written_block_vec{};
		for (int i{ 0 }; i != block_count; ++i) {
			const auto& written_block{ written_blocks[i] };
			if (written_block.pcoffset == 0x07FD7 && written_block.numbytes == 1) {
				// ROM size byte in header, ignore write since it's usually gonna be Asar expanding the ROM
				continue;
			}
			written_block_vec.push_back(WrittenBlock(written_block.pcoffset, written_block.snesoffset, written_block.numbytes));
		}
		std::sort(written_block_vec.begin(), written_block_vec.end(), [](const WrittenBlock& block1, const WrittenBlock& block2) {
			// written blocks can't (shouldn't?) overlap, so this is probably fine
			return block1.start_pc < block2.start_pc;
		});
		return written_block_vec;
	}

	std::vector<Module::FreespaceArea> Module::convertToFreespaceAreas(const std::vector<WrittenBlock>& written_blocks, const std::vector<char>& rom) {
		std::vector<FreespaceArea> freespace_areas{};

		auto curr_block{ written_blocks.begin() };
		size_t curr_freespace_size_left{ 0 };
		size_t curr_pc_offset{ 0 };
		size_t curr_snes_offset{ 0 };
		while (curr_block != written_blocks.end()) {
			curr_pc_offset = curr_block->start_pc;
			curr_snes_offset = curr_block->start_snes;

			auto size_left_in_curr_block{ curr_block->size };

			while (size_left_in_curr_block != 0) {
				if (curr_freespace_size_left == 0) {
					freespace_areas.push_back(std::vector<WrittenBlock>());
					const auto freespace_size{ determineFreespaceBlockSize(curr_pc_offset, rom) };
					if (!freespace_size.has_value()) {
						throw InsertionException(fmt::format(colors::EXCEPTION, "Freespace area at ${:06X} has invalid/no RATS tag", curr_pc_offset));
					}
					curr_freespace_size_left = freespace_size.value() + RATS_TAG_SIZE;
				}

				const auto served_by_block{ std::min(size_left_in_curr_block, curr_freespace_size_left) };

				freespace_areas.back().emplace_back(curr_pc_offset, curr_snes_offset, served_by_block);

				curr_freespace_size_left -= served_by_block;
				size_left_in_curr_block -= served_by_block;
				curr_pc_offset += served_by_block;
				curr_snes_offset += served_by_block;
			}
			if (curr_freespace_size_left != 0) {
				if ((curr_block->end_snes & 0xFFFF) != 0) {
					// dropping "unused" freespace at end of a written block since
					// asar apparently *will* reserve more space than it writes for
					// some reason sometimes if, on the other hand, we just crossed 
					// a bank border, we *do* need to keep track of the freespace 
					// still, fun times
					curr_freespace_size_left = 0;
				}
			}

			++curr_block;
		}

		return freespace_areas;
	}

	void Module::verifyNonHijacking() const {
		int block_count{};
		const auto written_blocks{ asar_getwrittenblocks(&block_count) };
		for (int i{ 0 }; i != block_count; ++i) {
			const auto& written_block{ written_blocks[i] };
			const auto start{ written_block.pcoffset };

			if (start == 0x07FD7 && written_block.numbytes == 1) {
				// Write to the ROM size byte of the SNES header
				// This is written by Asar if it needs to expand the ROM, so ignore writes here
				continue;
			}

			if (start < 0x80000) {
				throw InsertionException(fmt::format(
					colors::EXCEPTION,
					"Module {} targets SNES address ${:06X} (unheadered), if this is not a mistake consider using a patch instead "
					"as modules are not intended to modify original game code",
					project_relative_path.string(), written_block.snesoffset
				));
			}
		}
	}
}
