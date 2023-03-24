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

		std::unordered_map<Descriptor, std::vector<Interval>> written_intervals{};
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
				written_intervals.insert({ descriptor, getDifferences(old_rom, new_rom, check_conflicts_policy) });
				old_rom.swap(new_rom);
			}
		}

		checkConflicts(written_intervals, config.project_root.getOrThrow());

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

	void Rebuilder::checkConflicts(const std::unordered_map<Descriptor, std::vector<Interval>>& written_intervals, const fs::path& project_root) {

		auto current{ written_intervals.begin() };
		while (current != written_intervals.end()) {
			auto next{ std::next(current) };

			const auto our_descriptor{ current->first };
			const auto our_intervals{ current->second };

			while (next != written_intervals.end()) {
				const auto other_descriptor{ next->first };
				const auto other_intervals{ next->second };

				for (const auto& our_interval : our_intervals) {
					for (const auto& other_interval : other_intervals) {
						if (our_interval.overlaps(other_interval)) {
							const auto our_overlap{ our_interval.overlap(other_interval) };

							const auto other_overlap{ other_interval.overlap(our_interval) };

							if (our_overlap != other_overlap) {
								const auto byte_or_bytes{ our_overlap.length() == 1 ? "byte" : "bytes" };
								spdlog::warn(
									"${:06X} (PC: 0x{:06X}) - {} {}: Written to by '{}' and '{}'",
									our_overlap.snes_lower,
									our_overlap.pc_lower,
									our_overlap.length(),
									byte_or_bytes,
									our_descriptor.toString(project_root),
									other_descriptor.toString(project_root)
								);
							}
						}
					}
				}
				++next;
			}
			++current;
		}
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

	std::vector<Interval> Rebuilder::getDifferences(const std::vector<char> old_rom, const std::vector<char> new_rom, Conflicts conflict_policy) {
		if (conflict_policy == Conflicts::NONE) {
			return {};
		}
		int smaller_size{ static_cast<int>(std::min(old_rom.size(), new_rom.size())) };

		if (conflict_policy == Conflicts::HIJACKS) {
			smaller_size = std::min(smaller_size, 0x78000);
		}

		std::vector<Interval> differences{};
		bool parsing_diff{ false };
		int diff_start{ 0 };
		std::vector<char> diff{};
		for (size_t i{ 0 }; i != smaller_size; ++i) {
			if (i == 0x07FDC) {
				if (parsing_diff) {
					parsing_diff = false;
					differences.push_back(Interval(diff_start, i, diff));
					diff.clear();
				}
				i += 4;
			}

			if (old_rom.at(i) != new_rom.at(i)) {
				if (!parsing_diff) {
					parsing_diff = true;
					diff_start = i;
				}
				diff.push_back(new_rom.at(i));
			}
			else {
				if (parsing_diff) {
					parsing_diff = false;
					differences.push_back(Interval(diff_start, i, diff));
					diff.clear();
				}
			}
		}
		if (parsing_diff) {
			differences.push_back(Interval(diff_start, smaller_size, diff));
		}

		return differences;
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
