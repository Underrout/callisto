#include "rebuilder.h"

namespace stardust {
	void Rebuilder::build(const Configuration& config) {
		DependencyVector dependencies{};

		auto insertables{ buildOrderToInsertables(config) };

		if (config.initial_patch.isSet()) {
			InitialPatch initial_patch{ config };
			const auto initial_patch_resource_dependencies{ initial_patch.insertWithDependencies() };
			const auto initial_patch_config_dependencies{ initial_patch.getConfigurationDependencies() };
			dependencies.push_back({ Descriptor(Symbol::INITIAL_PATCH), 
				{ initial_patch_resource_dependencies, initial_patch_config_dependencies } });
		}
		else {
			fs::copy_file(config.clean_rom.getOrThrow(), config.temporary_rom.getOrThrow(), fs::copy_options::overwrite_existing);
		}

		WriteMap write_map{};
		std::vector<char> old_rom;

		const auto check_conflicts_policy{ determineConflictCheckSetting(config) };
		if (check_conflicts_policy != Conflicts::NONE) {
			old_rom = getRom(config.temporary_rom.getOrThrow());
		}

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

		reportConflicts(write_map);

		const auto insertion_report{ getJsonDependencies(dependencies) };

		writeBuildReport(config.project_root.getOrThrow(), insertion_report);
	}

	json Rebuilder::getJsonDependencies(const DependencyVector& dependencies) {
		json j;

		for (const auto& [descriptor, pair] : dependencies) {
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

		return j;
	}

	void Rebuilder::reportConflicts(const WriteMap& write_map) {
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
					writes = current->second;
				} while (current != write_map.end() && writers == getWriters(writes) && !writesAreIdentical(writes));
				outputConflict(written_bytes, pc_offset, conflict_size);
			}
			else {
				++current;
			}
		}
	}

	void Rebuilder::outputConflict(const ConflictVector& conflict_vector, int pc_start_offset, int conflict_size) {
		std::ostringstream output{};
		const auto byte_or_bytes{ conflict_size == 1 ? "byte" : "bytes" };
		output << fmt::format(
			"\nConflict - 0x{:X} {} at SNES: ${:06X} (unheadered), PC: 0x{:06X} (headered):\n",
			conflict_size, byte_or_bytes,
			pcToSnes(pc_start_offset), pc_start_offset + 0x200  // idk if the + 0x200 is controversial
		);

		for (const auto& [writer, written_bytes] : conflict_vector) {
			output << '\t' << writer << ':';
			int i{ 0 };
			while (i != written_bytes.size()) {
				if (i == 0x100) {
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

		spdlog::warn(output.str());
	}

	bool Rebuilder::writesAreIdentical(const Writes& writes) {
		auto first{ writes.at(0).second };
		return std::all_of(writes.begin(), writes.end(), [&](const auto& entry) {
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
}
