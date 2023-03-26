#include "rebuilder.h"

namespace stardust {
	void Rebuilder::build(const Configuration& config) {
		DependencyVector dependencies{};

		fs::copy_file(config.clean_rom.getOrThrow(), config.temporary_rom.getOrThrow(), fs::copy_options::overwrite_existing);

		if (config.initial_patch.isSet()) {
			InitialPatch initial_patch{ config };
			const auto initial_patch_resource_dependencies{ initial_patch.insertWithDependencies() };
			const auto initial_patch_config_dependencies{ initial_patch.getConfigurationDependencies() };
			dependencies.push_back({ Descriptor(Symbol::INITIAL_PATCH), 
				{ initial_patch_resource_dependencies, initial_patch_config_dependencies } });
		}

		expandRom(config);

		WriteMap write_map{};
		std::vector<char> old_rom;

		const auto check_conflicts_policy{ determineConflictCheckSetting(config) };
		if (check_conflicts_policy != Conflicts::NONE) {
			old_rom = getRom(config.temporary_rom.getOrThrow());
		}

		auto insertables{ buildOrderToInsertables(config) };
		for (auto& [descriptor, insertable] : insertables) {
			const auto resource_dependencies{ insertable->insertWithDependencies() };
			const auto config_dependencies{ insertable->getConfigurationDependencies() };

			dependencies.push_back({ descriptor, { resource_dependencies, config_dependencies } });

			if (check_conflicts_policy != Conflicts::NONE) {
				auto new_rom{ getRom(config.temporary_rom.getOrThrow()) };
				updateWrites(old_rom, new_rom, check_conflicts_policy, write_map,
					descriptor.toString(config.project_root.getOrThrow()));
				old_rom.swap(new_rom);
			}
		}

		reportConflicts(write_map, config.conflict_log_file.isSet() ?
			std::make_optional(config.conflict_log_file.getOrThrow()) : 
			std::nullopt);

		const auto insertion_report{ getJsonDependencies(dependencies) };

		writeBuildReport(config.project_root.getOrThrow(), createBuildReport(config, insertion_report));

		cacheGlobules(config.project_root.getOrThrow());

		moveTempToOutput(config);
	}

	json Rebuilder::getJsonDependencies(const DependencyVector& dependencies) {
		json j;

		std::unordered_set<Descriptor> seen{};

		for (const auto& [descriptor, pair] : boost::adaptors::reverse(dependencies)) {
			if (seen.count(descriptor) != 0) {
				continue;
			}

			seen.insert(descriptor);

			auto block{ json({
				{"descriptor", descriptor.toJson()},
				{"resource_dependencies", std::vector<json>()},
				{"configuration_dependencies", std::vector<json>()}
			}) };

			for (const auto& resource_dependency : pair.first) {
				block["resource_dependencies"].push_back(resource_dependency.toJson());
			}

			for (const auto& config_dependency : pair.second) {
				block["configuration_dependencies"].push_back(config_dependency.toJson());
			}

			j.push_back(block);
		}

		std::reverse(j.begin(), j.end());

		return j;
	}

	void Rebuilder::reportConflicts(const WriteMap& write_map, const std::optional<fs::path>& log_file) {
		std::ofstream log;
		int conflicts{ 0 };
		const auto log_to_file{ log_file.has_value() };
		if (log_to_file) {
			log.open(log_file.value());
		}

		auto current{ write_map.begin() };
		while (current != write_map.end()) {
			auto& pc_offset{ current->first };
			auto writes{ current->second };

			if (!writesAreIdentical(writes)) {
				auto writers{ getWriters(writes) };
				ConflictVector written_bytes{};
				for (const auto& writer : writers) {
					written_bytes.push_back({ writer, {} });
				}
				int conflict_size{ 0 };
				do {
					for (int i{ 0 }; i != writers.size(); ++i) {
						const auto written_byte{ writes.at(i).second };
						written_bytes.at(i).second.push_back(written_byte);
					}
					++conflict_size;
					++current;
				} while (current != write_map.end() && writers == getWriters(current->second) && !writesAreIdentical(current->second));
				const auto conflict_string{ getConflictString(
					written_bytes, pc_offset, conflict_size, !log_to_file) };
				++conflicts;
				if (log_to_file) {
					log << conflict_string;
				}
				else {
					spdlog::warn(conflict_string);
				}
			}
			else {
				++current;
			}
		}

		if (log_to_file) {
			if (conflicts == 0) {
				spdlog::info("No conflicts logged to {}", log_file.value().string());
			}
			else {
				spdlog::warn("{} conflict(s) logged to {}", conflicts, log_file.value().string());
			}
		}
		else if (conflicts == 0) {
			spdlog::info("No conflicts found");
		}
	}

	std::string Rebuilder::getConflictString(const ConflictVector& conflict_vector, int pc_start_offset, int conflict_size, bool limit_lines) {
		std::ostringstream output{};
		const auto byte_or_bytes{ conflict_size == 1 ? "byte" : "bytes" };
		output << fmt::format(
			"Conflict - 0x{:X} {} at SNES: ${:06X} (unheadered), PC: 0x{:06X} (headered):\n",
			conflict_size, byte_or_bytes,
			pcToSnes(pc_start_offset), pc_start_offset + 0x200  // idk if the + 0x200 is controversial
		);

		for (const auto& [writer, written_bytes] : conflict_vector) {
			output << '\t' << writer << ':';
			int i{ 0 };
			while (i != written_bytes.size()) {
				if (limit_lines && i == 0x100) {
					output << "...";
					break;
				}
				if (i % 0x10 == 0) {
					output << std::endl << "\t\t";
				}
				output << fmt::format("{:02X} ", written_bytes.at(i++));
			}
			output << std::endl;
		}

		return output.str();
	}

	bool Rebuilder::writesAreIdentical(const Writes& writes) {
		auto first{ writes.at(1).second };
		return std::all_of(writes.begin() + 1, writes.end(), [&](const auto& entry) {
			return entry.second == first;
		});
	}

	// just assuming lorom here, hope nobody gets mad (read: someone will be mad, but it's ok because I saw it coming)
	int Rebuilder::pcToSnes(int address) {
		int snes{ ((address << 1) & 0x7F0000) | (address & 0x7FFF) | 0x8000 };
		return snes | 0x800000;
	}

	std::vector<std::string> Rebuilder::getWriters(const Writes& writes) {
		std::vector<std::string> writers{};
		for (const auto& [writer, _] : writes) {
			writers.push_back(writer);
		}
		return writers;
	}

	std::vector<char> Rebuilder::getRom(const fs::path& rom_path) {
		std::ifstream rom_file(rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		std::vector<char> unheadered(rom_bytes.begin() + header_size, rom_bytes.end());

		return unheadered;
	}

	void Rebuilder::updateWrites(const std::vector<char>& old_rom, const std::vector<char>& new_rom, 
		Conflicts conflict_policy, WriteMap& write_map, const std::string& descriptor_string) {
		if (conflict_policy == Conflicts::NONE) {
			return;
		}
		int size{ static_cast<int>(std::max(old_rom.size(), new_rom.size())) };

		if (conflict_policy == Conflicts::HIJACKS) {
			size = std::min(size, 0x78000);
		}

		for (int i{ 0 }; i != size; ++i) {
			if (i == 0x07FDC) {
				// skip checksum and inverse checksum
				i += 3;
				continue;
			}

			if (i >= old_rom.size() || old_rom.at(i) != new_rom.at(i)) {
				if (write_map.find(i) == write_map.end() && i < old_rom.size()) {
					write_map[i].push_back({ "Original bytes", old_rom.at(i)});
				}
				write_map[i].push_back({ descriptor_string, new_rom.at(i) });
			}
		}
	}

	Rebuilder::Conflicts Rebuilder::determineConflictCheckSetting(const Configuration& config) {
		const auto setting{ config.check_conflicts.getOrDefault("hijacks") };
		if (setting == "all") {
			return Conflicts::ALL;
		}
		else if (setting == "hijacks") {
			return Conflicts::HIJACKS;
		}
		else if (setting == "none") {
			return Conflicts::NONE;
		}
		else {
			throw StardustException(fmt::format(
				"Unknown settings.check_conflicts setting '{}'",
				setting
			));
		}
	}

	void Rebuilder::expandRom(const Configuration& config) {
		if (config.rom_size.isSet()) {
			
			const auto exit_code{ bp::system(
				config.lunar_magic_path.getOrThrow().string(),
				"-ExpandROM",
				config.temporary_rom.getOrThrow().string(),
				config.rom_size.getOrThrow()
			) };
		}
	}
}
