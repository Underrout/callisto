#include "globule.h"

namespace callisto {
	Globule::Globule(const Configuration& config,
		const fs::path& globule_path, const fs::path& imprint_directory,
		const fs::path& callisto_asm_file,
		const std::vector<fs::path>& other_globule_paths,
		const std::vector<fs::path>& additional_include_paths) :
		RomInsertable(config), 
		project_relative_path(fs::relative(globule_path, registerConfigurationDependency(config.project_root).getOrThrow())),
		globule_path(globule_path), imprint_directory(imprint_directory), callisto_asm_file(callisto_asm_file),
		globule_header_file(registerConfigurationDependency(config.globule_header, Policy::REINSERT).isSet() ? std::make_optional(config.globule_header.getOrThrow()) : std::nullopt)
	{
		if (!fs::exists(globule_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Globule {} does not exist",
				globule_path.string()
			));
		}

		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside callisto?"
			);
		}

		this->additional_include_paths.reserve(additional_include_paths.size());
		std::transform(additional_include_paths.begin(), additional_include_paths.end(),
			std::back_inserter(this->additional_include_paths),
			[](const fs::path& path) {
				char* copy{ new char[path.string().size()] };
		std::strcpy(copy, path.string().c_str());
		return copy;
			});

		for (const auto& other_globule_path : other_globule_paths) {
			other_globule_names.insert(globulePathToName(other_globule_path));
		}
	}

	void Globule::init() {
		std::ostringstream temp_patch{};

		temp_patch << "warnings disable W1011\n"
			<< "warnings disable W1007\n"
			<< "if read1($00FFD5) == $23\nsa1rom\nelse\nlorom\nendif\n";

		if (globule_path.extension() == ".asm") {
			if (globule_header_file.has_value()) {
				temp_patch << "incsrc \"" << globule_header_file.value() << '"' << std::endl << std::endl;
			}

			temp_patch << "incsrc \"" << globule_path.string() << '"' << std::endl;
		}
		else {
			temp_patch << "freedata" << std::endl << std::endl;

			auto label_name{ globule_path.stem().string() };
			std::replace(label_name.begin(), label_name.end(), ' ', '_');

			temp_patch << fmt::format("incbin \"{}\" -> {}", globule_path.string(), label_name) << std::endl;
		}

		patch_string = temp_patch.str();
	}

	// TODO this whole thing is pretty much the same as the Patch.insert() method, probably merge 
	//      them into a single static method or something later
	void Globule::insert() {
		const auto prev_folder{ fs::current_path() };
		fs::current_path(globule_path.parent_path());

		// delete potential previous dependency report
		fs::remove(globule_path.parent_path() / ".dependencies");

		spdlog::info(fmt::format("Inserting globule {}", project_relative_path.string()));

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
			"Applying globule {} to temporary ROM {}:\n\r"
			"\tROM size:\t\t{}\n\r"
			"\tROM header size:\t\t{}\n\r",
			globule_path.string(),
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
			false,
			true
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
					"Globule {} is missing a freespace command",
					project_relative_path.string()
				));
			}

			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
			spdlog::info(fmt::format("Successfully applied globule {}!", project_relative_path.string()));

			emitImprintFile();

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
				"Failed to apply globule {} with the following error(s):\n\r{}",
				project_relative_path.string(),
				error_string.str()
			));
		}
	}

	std::string Globule::globulePathToName(const fs::path& path) {
		auto prefix{ path.stem().string() };
		std::replace(prefix.begin(), prefix.end(), ' ', '_');

		return prefix;
	}

	void Globule::emitImprintFile() const {
		fs::create_directories(imprint_directory);

		std::ofstream imprint{ imprint_directory / (globule_path.stem().string() + ".asm")};

		imprint << "warnings disable W1007\n\n";
		imprint << fmt::format("incsrc \"{}\"", callisto_asm_file.string()) << std::endl << std::endl;

		int label_number{};
		const auto labels{ asar_getalllabels(&label_number) };

		const auto globule_name{ globulePathToName(globule_path) };

		if (label_number == 0) {
			throw InsertionException(fmt::format(
				"Globule {} contains no labels, this will cause a freespace leak, please ensure your globule contains at least one label",
				globule_name
			));
		}

		if (globule_path.extension() != ".asm") {
			if (label_number > 1) {
				throw InsertionException(fmt::format(
					"Binary globule {} unexpectedly contains more than one label",
					globule_name
				));
			}

			const auto& label{ labels[0] };
			const auto name{ std::string(label.name) };

			imprint << fmt::format("{} = ${:06X}", globule_name, label.location) << std::endl;
			imprint << fmt::format("!{} = ${:06X}", globule_name, label.location) << std::endl;

			return;
		}

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

			const auto underscore_idx{ name.find('_', 0) };
			if (other_globule_names.count(name) != 0 || 
				(underscore_idx != -1 && other_globule_names.count(name.substr(0, underscore_idx)) != 0)) {
				if (name.substr(0, underscore_idx) != globule_name) {
					// label belongs to imported globule, skip it
					continue;
				}
			}

			imprint << fmt::format("{}_{} = ${:06X}", globule_name, name, label.location) << std::endl;
			imprint << fmt::format("!{}_{} = ${:06X}", globule_name, name, label.location) << std::endl;
		}
	}

	std::unordered_set<ResourceDependency> Globule::determineDependencies() {
		
		if (globule_path.extension() == ".asm") {
			auto dependencies{ Insertable::extractDependenciesFromReport(
				globule_path.parent_path() / ".dependencies"
			) };
			if (globule_header_file.has_value()) {
				dependencies.insert(ResourceDependency(globule_header_file.value(), Policy::REINSERT));
			}
			dependencies.insert(ResourceDependency(globule_path, Policy::REINSERT));
			return dependencies;
		}
		else {
			fs::remove(globule_path.parent_path() / ".dependencies");
			return { ResourceDependency(globule_path, Policy::REINSERT) };
		}
		return {};
	}
}
