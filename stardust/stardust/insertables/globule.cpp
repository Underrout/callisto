#include "globule.h"

namespace stardust {
	Globule::Globule(const fs::path& project_root_path, const fs::path& temporary_rom_path,
		const fs::path& globule_path, const fs::path& imprint_directory, const fs::path& globule_call_file,
		const std::vector<fs::path>& other_globule_paths,
		const std::optional<fs::path> globule_header_file,
		const std::vector<fs::path>& additional_include_paths) :
		RomInsertable(temporary_rom_path), project_relative_path(fs::relative(globule_path, project_root_path)),
		globule_path(globule_path), imprint_directory(imprint_directory), globule_call_file(globule_call_file),
		globule_header_file(globule_header_file)
	{
		if (!fs::exists(globule_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Globule {} does not exist",
				globule_path.string()
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
				char* copy{ new char[path.string().size()] };
		std::strcpy(copy, path.string().c_str());
		return copy;
			});

		for (const auto& other_globule_path : other_globule_paths) {
			other_globule_names.insert(globulePathToName(other_globule_path));
		}
	}

	// TODO this whole thing is pretty much the same as the Patch.insert() method, probably merge 
	//      them into a single static method or something later
	void Globule::insert() {
		std::string patch_path{ (boost::filesystem::temp_directory_path() / boost::filesystem::unique_path().string()).string() };
		std::ofstream temp_patch{ patch_path };

		// delete potential previous dependency report
		fs::remove(globule_path.parent_path() / ".dependencies");

		if (globule_path.extension() == ".asm") {
			spdlog::info(fmt::format("Inserting ASM globule {}", project_relative_path.string()));
			
			temp_patch << "warnings disable W1011" << std::endl << std::endl
				<< "warnings disable W1007" << std::endl << std::endl
				<< "if read1($00FFD5) == $23\nsa1rom\nendif" << std::endl << std::endl
				<< "freecode cleaned" << std::endl << std::endl;

			if (globule_header_file.has_value()) {
				temp_patch << "incsrc " << globule_header_file.value() << std::endl << std::endl;
			}

			temp_patch << "incsrc \"" << globule_path.string() << '"' << std::endl;
		}
		else {
			spdlog::info(fmt::format("Inserting binary globule {}", project_relative_path.string()));

			auto label_name{ globule_path.stem().string()};
			std::replace(label_name.begin(), label_name.end(), ' ', '_');

			temp_patch << fmt::format("incbin \"{}\" -> {}", globule_path.string(), label_name ) << std::endl;
		}

		temp_patch.close();

		std::ifstream rom_file(temporary_rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		spdlog::debug(fmt::format(
			"Applying globule {} to temporary ROM {}:\n"
			"\tROM size:\t\t{}\n"
			"\tROM header size:\t\t{}\n",
			globule_path.string(),
			temporary_rom_path.string(),
			rom_size,
			header_size
		));

		const patchparams params{
			sizeof(patchparams),
			patch_path.c_str(),
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

		fs::remove(patch_path);

		if (succeeded) {
			int warning_count;
			const auto warnings{ asar_getwarnings(&warning_count) };
			for (int i = 0; i != warning_count; ++i) {
				spdlog::warn(warnings[i].fullerrdata);
			}
			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
			spdlog::info(fmt::format("Successfully applied globule {}!", project_relative_path.string()));

			emitImprintFile();
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
				"Failed to apply globule {} with the following error(s):\n{}",
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

		std::ofstream imprint{ imprint_directory / globule_path.filename() };

		imprint << fmt::format("incsrc \"{}\"", globule_call_file.string()) << std::endl << std::endl;

		int label_number{};
		const auto labels{ asar_getalllabels(&label_number) };

		const auto globule_name{ globulePathToName(globule_path) };

		for (int i{ 0 }; i != label_number; ++i) {
			const auto& label{ labels[i] };
			const auto name{ std::string(label.name) };

			if (name.at(0) == ':') {
				// it's a relative label (+, -, ++, ...), skip it
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

		// fixAsarMemoryLeak();

		imprint.close();
	}

	std::unordered_set<Dependency> Globule::determineDependencies() {
		if (globule_path.extension() == ".asm") {
			const auto dependencies{ Insertable::extractDependenciesFromReport(
				globule_path.parent_path() / ".dependencies"
			) };
			return dependencies;
		}
		return {};
	}

	void Globule::fixAsarMemoryLeak() const {
		// apparently labels will leak unless you patch a file 
		// without labels and then get them again, so I guess we'll do this?

		const memoryfile empty_patch{ "empty.asm", "", 0 };
		int size;

		const patchparams params{
			sizeof(patchparams),
			"empty.asm",
			nullptr,
			0,
			&size,
			nullptr,
			0,
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

		if (!asar_patch_ex(&params)) {
			spdlog::warn("Failed to clean up asar labels, stardust might leak memory.");
			return;
		}
		int labels;
		asar_getalllabels(&labels);
	}
}
