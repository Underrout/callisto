#include "rebuilder.h"

namespace callisto {
	void Rebuilder::build(const Configuration& config) {
		const auto build_start{ std::chrono::high_resolution_clock::now() };

		spdlog::info(fmt::format(colors::ACTION_START, "Rebuild started"));
		spdlog::info("");

		spdlog::info(fmt::format(colors::CALLISTO, "Checking clean ROM"));
		spdlog::info("");
		checkCleanRom(config.clean_rom.getOrThrow());

		init(config);

		DependencyVector dependencies{};
		PatchHijacksVector patch_hijacks{};

		const auto temp_rom_path{ PathUtil::getTemporaryRomPath(config.temporary_folder.getOrThrow(),
	config.output_rom.getOrThrow()) };
		fs::copy_file(config.clean_rom.getOrThrow(), temp_rom_path, fs::copy_options::overwrite_existing);

		auto insertables{ buildOrderToInsertables(config) };

		std::jthread init_thread;
		std::jthread conflict_thread;
		std::exception_ptr init_thread_exception{};
		std::exception_ptr conflict_thread_exception{};
		bool conflict_thread_created{ false };
		if (!insertables.empty()) {
			init_thread = std::jthread([&] { 
				try {
					insertables[0].second->init(); 
				}
				catch (...) {
					init_thread_exception = std::current_exception();
				}
			});
		}

		std::shared_ptr<WriteMap> write_map{ std::make_shared<WriteMap>() };
		auto old_rom{ std::make_shared<std::vector<char>>() };
		auto new_rom{ std::make_shared<std::vector<char>>() };
		Conflicts check_conflicts_policy{ determineConflictCheckSetting(config) };

		if (check_conflicts_policy != Conflicts::NONE) {
			*old_rom = getRom(temp_rom_path);
		}

		size_t i{ 0 };
		std::optional<Insertable::NoDependencyReportFound> failed_dependency_report{};
		for (const std::pair<const Descriptor&, std::shared_ptr<Insertable>> pair : insertables) {
			init_thread.join();
			if (init_thread_exception != nullptr) {
				std::rethrow_exception(init_thread_exception);
			}
			if (i != insertables.size() - 1) {
				init_thread = std::jthread([&] { 
					try {
						insertables[++i].second->init(); 
					}
					catch (...) {
						init_thread_exception = std::current_exception();
					}
				});
			}

			const auto insertable{ pair.second };
			const auto& descriptor{ pair.first };

			spdlog::info(fmt::format(colors::CALLISTO, "--- {} ---", descriptor.toString(config.project_root.getOrThrow())));

			if (!failed_dependency_report.has_value()) {
				const auto curr_path{ fs::current_path() };
				std::unordered_set<ResourceDependency> resource_dependencies;
				try {
					resource_dependencies = insertable->insertWithDependencies();
				}
				catch (const Insertable::NoDependencyReportFound& e) {
					failed_dependency_report = e;
				}
				catch (...) {
					fs::current_path(curr_path);
					try {
						fs::remove_all(config.temporary_folder.getOrThrow());
					}
					catch (const std::runtime_error& e) {
						spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary folder '{}'",
							config.temporary_folder.getOrThrow().string()));
					}
					std::rethrow_exception(std::current_exception());
				}
				spdlog::info("");

				if (descriptor.symbol == Symbol::PATCH) {
					patch_hijacks.push_back(static_pointer_cast<Patch>(insertable)->getHijacks());
				}
				else {
					patch_hijacks.push_back({});
				}

				if (!failed_dependency_report.has_value()) {
					const auto config_dependencies{ insertable->getConfigurationDependencies() };
					dependencies.push_back({ descriptor, { resource_dependencies, config_dependencies } });
				}
			}
			else {
				const auto curr_path{ fs::current_path() };
				try {
					insertable->insert();
					spdlog::info("");
				}
				catch (...) {
					fs::current_path(curr_path);
					try {
						fs::remove_all(config.temporary_folder.getOrThrow());
					}
					catch (const std::runtime_error& e) {
						spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary folder '{}'",
							config.temporary_folder.getOrThrow().string()));
					}
					std::rethrow_exception(std::current_exception());
				}
			}

			if (check_conflicts_policy != Conflicts::NONE) {
				if (conflict_thread_created) {
					conflict_thread.join();
				}
				else {
					conflict_thread_created = true;
				}

				if (conflict_thread_exception != nullptr) {
					std::rethrow_exception(conflict_thread_exception);
				}

				*new_rom = getRom(temp_rom_path);
				const fs::path project_root{ config.project_root.getOrThrow() };
				conflict_thread = std::jthread([old_rom, new_rom, check_conflicts_policy, write_map, descriptor, project_root, &conflict_thread_exception] {
					try {
						updateWrites(old_rom, new_rom, check_conflicts_policy, write_map,
							descriptor.toString(project_root));
						old_rom->swap(*new_rom);
					}
					catch (...) {
						conflict_thread_exception = std::current_exception();
					}
				});
				// conflict_thread.join();
			}
		}

		if (conflict_thread_created) {
			conflict_thread.join();
		}

		if (check_conflicts_policy != Conflicts::NONE) {
			conflict_thread_created = true;
			const auto conflict_log_file{ config.conflict_log_file.isSet() ?
						std::make_optional(config.conflict_log_file.getOrThrow()) :
						std::nullopt };
			const auto &ignored_symbols{ config.ignored_conflict_symbols };
			const auto& project_root{ config.project_root.getOrThrow() };
			conflict_thread = std::jthread([&conflict_thread_exception, write_map, conflict_log_file, check_conflicts_policy, ignored_symbols, project_root] {
				try {
					reportConflicts(write_map, conflict_log_file, check_conflicts_policy, conflict_thread_exception, 
						ignored_symbols, project_root);
				}
				catch (...) {
					conflict_thread_exception = std::current_exception();
				}
			});
		}

		if (!failed_dependency_report.has_value()) {
			try {
				const auto insertion_report{ getJsonDependencies(dependencies, patch_hijacks) };

				writeBuildReport(config.project_root.getOrThrow(), createBuildReport(config, insertion_report));
			}
			catch (const std::exception& e) {
				spdlog::warn(fmt::format(colors::WARNING, "Failed to write build report with following exception:\n\r{}", e.what()));
			}
		}
		else {
			spdlog::info(fmt::format(colors::NOTIFICATION, "{}, Update not applicable, read the documentation "
					"on details for how to set up Update correctly", failed_dependency_report.value().what()));
			removeBuildReport(config.project_root.getOrThrow());
		}

		GraphicsUtil::linkOutputRomToProjectGraphics(config, false);
		GraphicsUtil::linkOutputRomToProjectGraphics(config, true);

		cacheModules(config.project_root.getOrThrow());

		Saver::writeMarkerToRom(temp_rom_path, config);
		moveTempToOutput(config);

		const auto build_end{ std::chrono::high_resolution_clock::now() };

		if (conflict_thread_created) {
			conflict_thread.join();
		}

		if (conflict_thread_exception != nullptr) {
			try {
				std::rethrow_exception(conflict_thread_exception);
			}
			catch (const std::exception& e) {
				spdlog::warn(fmt::format(colors::WARNING, "The following error occurred while attempting to report conflicts:\n\r{}", e.what()));
			}
		}
		
		try {
			fs::remove_all(config.temporary_folder.getOrThrow());
		}
		catch (const std::runtime_error&) {
			spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary folder '{}'",
				config.temporary_folder.getOrThrow().string()));
		}

		spdlog::info(fmt::format(colors::SUCCESS, "Rebuild finished successfully in {} \\(^.^)/", 
			TimeUtil::getDurationString(build_end - build_start)));
	}

	json Rebuilder::getJsonDependencies(const DependencyVector& dependencies, const PatchHijacksVector& hijacks) {
		json j;

		std::unordered_set<Descriptor> seen{};
		size_t i{ dependencies.size() - 1 };
		for (const auto& [descriptor, pair] : boost::adaptors::reverse(dependencies)) {
			if (seen.count(descriptor) != 0) {
				--i;
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

			const auto& patch_hijacks{ hijacks[i--] };
			 
			if (patch_hijacks.has_value()) {
				block["hijacks"] = patch_hijacks.value();
			}

			j.push_back(block);
		}

		std::reverse(j.begin(), j.end());

		return j;
	}

	void Rebuilder::reportConflicts(std::shared_ptr<WriteMap> write_map, const std::optional<fs::path>& log_file_path,
		Conflicts conflict_policy, std::exception_ptr conflict_exception, const std::unordered_set<Descriptor>& ignored_descriptors, 
		const fs::path& project_root) {
		if (conflict_policy == Conflicts::NONE) {
			spdlog::info(fmt::format(colors::NOTIFICATION, "Not set to detect conflicts"));
			return;
		}

		if (conflict_exception != nullptr) {
			try {
				std::rethrow_exception(conflict_exception);
			}
			catch (const std::exception& e) {
				spdlog::warn(fmt::format(colors::WARNING, "The following exception prevented conflict detection:\n\r{}", e.what()));
				return;
			}
		}

		std::unordered_set<std::string> ignored_names{};
		for (const auto& descriptor : ignored_descriptors) {
			ignored_names.insert(descriptor.toString(project_root));
		}

		std::ostringstream log{};
		int conflicts{ 0 };
		const auto log_to_file{ log_file_path.has_value() };

		auto current{ write_map->begin() };
		bool one_logged{ false };
		while (current != write_map->end()) {
			auto& pc_offset{ current->first };
			auto writes{ current->second };

			if (!writesAreIdentical(writes, ignored_names)) {
				auto writers{ getWriters(writes) };
				ConflictVector written_bytes{};
				for (const auto& writer : writers) {
					written_bytes.push_back({ writer, {} });
				}
				int conflict_size{ 0 };
				do {
					for (int i{ 0 }; i != writers.size(); ++i) {
						const auto written_byte{ current->second.at(i).second };
						written_bytes.at(i).second.push_back(written_byte);
					}
					++conflict_size;
					++current;
				} while (current != write_map->end() && writers == getWriters(current->second) && !writesAreIdentical(current->second, ignored_names));
				const auto conflict_string{ getConflictString(
					written_bytes, pc_offset, conflict_size, !log_to_file) };
				++conflicts;
				if (log_to_file) {
					if (one_logged) {
						log << '\n';
					}
					else {
						one_logged = true;
					}
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
		
		if (conflicts == 0) {
			if (log_to_file && fs::exists(log_file_path.value())) {
				fs::remove(log_file_path.value());
			}
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "No conflicts found"));
		}
		else if (log_to_file) {
			std::ofstream log_file{ log_file_path.value() };
			log_file << log.str();
			spdlog::warn(fmt::format(colors::WARNING, "{} conflict(s) logged to {}", 
				conflicts, log_file_path.value().string()));
		}
	}

	std::string Rebuilder::getConflictString(const ConflictVector& conflict_vector, int pc_start_offset, int conflict_size, bool for_console) {
		std::ostringstream output{};
		const auto byte_or_bytes{ conflict_size == 1 ? "byte" : "bytes" };
		const auto line_end{ for_console ? "\n\r" : "\n" };
		output << fmt::format(
			"Conflict - 0x{:X} {} at SNES: ${:06X} (unheadered), PC: 0x{:06X} (headered):{}",
			conflict_size, byte_or_bytes,
			pcToSnes(pc_start_offset), pc_start_offset + 0x200,  // idk if the + 0x200 is controversial
			line_end
		);

		for (const auto& [writer, written_bytes] : conflict_vector) {
			output << '\t' << writer << ':';
			int i{ 0 };
			while (i != written_bytes.size()) {
				if (for_console && i == 0x100) {
					output << "...";
					break;
				}
				if (i % 0x10 == 0) {
					output << line_end << "\t\t";
				}
				output << fmt::format("{:02X} ", written_bytes.at(i++));
			}
			output << line_end;
		}

		return output.str();
	}

	bool Rebuilder::writesAreIdentical(const Writes& writes, const std::unordered_set<std::string>& ignored_names) {
		if (writes.size() == 1) {
			return true;
		}
		std::optional<unsigned char> byte_to_match{};
		for (auto it{ writes.begin() + 1 }; it != writes.end(); ++it) {
			if (ignored_names.contains(it->first)) {
				continue;
			}

			if (!byte_to_match.has_value()) {
				byte_to_match = it->second;
			}
			else if (byte_to_match.value() != it->second) {
				return false;
			}
		}

		return true;
	}

	int Rebuilder::pcToSnes(int address) {
		int snes{ ((address << 1) & 0x7F0000) | (address & 0x7FFF) | 0x8000 };
		return snes;
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

	void Rebuilder::updateWrites(std::shared_ptr<std::vector<char>> old_rom, std::shared_ptr<std::vector<char>> new_rom,
		Conflicts conflict_policy, std::shared_ptr<WriteMap> write_map, const std::string& descriptor_string) {
		if (conflict_policy == Conflicts::NONE) {
			return;
		}
		int size{ static_cast<int>(std::min(old_rom->size(), new_rom->size())) };

		if (conflict_policy == Conflicts::HIJACKS) {
			size = std::min(size, 0x80000);
		}

		for (int i{ 0 }; i != size; ++i) {
			if (i == 0x07FDC) {
				// skip checksum and inverse checksum
				i += 3;
				continue;
			}

			if (i >= old_rom->size() || old_rom->at(i) != new_rom->at(i)) {
				if (write_map->find(i) == write_map->end() && i < old_rom->size()) {
					(*write_map)[i].push_back({ "Original bytes", old_rom->at(i)});
				}
				(*write_map)[i].push_back({ descriptor_string, new_rom->at(i) });
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
			throw CallistoException(fmt::format(
				"Unknown settings.check_conflicts setting '{}'",
				setting
			));
		}
	}
}
