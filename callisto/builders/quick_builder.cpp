#include "quick_builder.h"

namespace callisto {
	QuickBuilder::QuickBuilder(const fs::path& project_root) {
		const auto build_report_path{ PathUtil::getBuildReportPath(project_root) };
		if (!fs::exists(build_report_path)) {
			throw MustRebuildException(fmt::format(
				colors::EXCEPTION,
				"No build report found at {}, must rebuild",
				build_report_path.string()
			));
		}

		std::ifstream build_report{ build_report_path };
		report = json::parse(build_report);
		build_report.close();
	}

	QuickBuilder::Result QuickBuilder::build(const Configuration& config) {
		const auto build_start{ std::chrono::high_resolution_clock::now() };

		spdlog::info(fmt::format(colors::ACTION_START, "Update started"));
		spdlog::info("");

		init(config);

		spdlog::info(fmt::format(colors::CALLISTO, "Checking whether ROM from previous build exists"));
		if (!fs::exists(config.output_rom.getOrThrow())) {
			throw MustRebuildException(fmt::format(colors::NOTIFICATION, "No ROM found at {}, must rebuild", config.output_rom.getOrThrow().string()));
		}
		spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "ROM from previous build found at '{}'", config.output_rom.getOrThrow().string()));
		spdlog::info("");

		spdlog::info(fmt::format(colors::CALLISTO, "Checking whether build report format has changed"));
		checkBuildReportFormat();
		spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Build report format has not changed"));
		spdlog::info("");

		spdlog::info(fmt::format(colors::CALLISTO, "Checking whether build order has changed"));
		checkBuildOrderChange(config);
		spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Build order has not changed"));
		spdlog::info("");

		if (config.levels.isSet()) {
			spdlog::info(fmt::format(colors::CALLISTO, "Checking whether level files have been removed since last build"));
			checkProblematicLevelChanges(config.levels.getOrThrow(), report["inserted_levels"]);
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "No level files have been removed"));
			spdlog::info("");
		}

		auto& json_dependencies{ report["dependencies"] };
		spdlog::info(fmt::format(colors::CALLISTO, "Checking whether any configuration changes require a rebuild"));
		checkRebuildConfigDependencies(json_dependencies, config);
		spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "No configuration changes require a rebuild"));
		spdlog::info("");

		const auto temporary_rom_path{ PathUtil::getTemporaryRomPath(
			config.temporary_folder.getOrThrow(), config.output_rom.getOrThrow()
		) };

		bool any_work_done{ false };
		bool anything_ran{ false };
		std::optional<Insertable::NoDependencyReportFound> failed_dependency_report;
		size_t i{ 0 };
		for (auto& entry : json_dependencies) {
			checkRebuildResourceDependencies(json_dependencies, config.project_root.getOrThrow(), i++);
			const auto descriptor{ Descriptor(entry["descriptor"]) };
			spdlog::info(fmt::format(colors::CALLISTO, "--- {} ---", descriptor.toString(config.project_root.getOrThrow())));

			const auto descriptor_string{ descriptor.toString(config.project_root.getOrThrow()) };
			const auto config_result{ 
				checkReinsertConfigDependencies(entry["configuration_dependencies"], config)};
			bool must_reinsert{ false };

			std::string term{ "reinserted" };
			if (descriptor.symbol == Symbol::EXTERNAL_TOOL) {
				bool uses_rom{ config.generic_tool_configurations.find(descriptor.name.value())->second.pass_rom.getOrDefault(true) };
				if (uses_rom) {
					term = "reapplied";
				}
				else {
					term = "run";
				}
			}

			if (config_result.has_value()) {
				spdlog::info(fmt::format(
					colors::NOTIFICATION,
					"{} must be {} due to change in configuration variable {}",
					descriptor_string,
					term,
					config_result.value().config_keys
				));
				must_reinsert = true;
			}
			else {
				const auto resource_result{
					checkReinsertResourceDependencies(entry["resource_dependencies"])
				};

				if (resource_result.has_value()) {
					spdlog::info(fmt::format(
						colors::NOTIFICATION,
						"{} must be {} due to change in resource '{}'",
						descriptor_string,
						term,
						(fs::relative(resource_result.value().dependent_path, config.project_root.getOrThrow())).string()
					));
					must_reinsert = true;
				}

				if (descriptor.symbol == Symbol::MODULE) {
					std::unordered_set<fs::path> current_output_paths{};
					for (const auto& [input_path, module_config ] : config.module_configurations) {
						if (input_path == descriptor.name) {
							const auto real_output_paths{ module_config.real_output_paths.getOrThrow() };
							current_output_paths.insert(real_output_paths.begin(), real_output_paths.end());
							break;
						}
					}

					const auto& old_output_paths{ report["module_outputs"][descriptor.name.value()] };

					if (old_output_paths.size() != current_output_paths.size()) {
						spdlog::info(fmt::format(
							colors::NOTIFICATION,
							"{} must be {} due to change in its output paths",
							descriptor_string,
							term
						));
						must_reinsert = true;
					}
					else {
						for (const auto& entry : old_output_paths) {
							if (!current_output_paths.contains(entry)) {
								spdlog::info(fmt::format(
									colors::NOTIFICATION,
									"{} must be {} due to change in its output paths",
									descriptor_string,
									term
								));
								must_reinsert = true;
								break;
							}
						}
					}
				}
			}

			if (must_reinsert) {
				if (!fs::exists(temporary_rom_path)) {
					fs::copy(config.output_rom.getOrThrow(), temporary_rom_path, fs::copy_options::overwrite_existing);
				}
				if (descriptor.symbol == Symbol::MODULE) {
					cleanModule(
						descriptor.name.value(),
						temporary_rom_path,
						config.project_root.getOrThrow()
					);
				}

				auto insertable{ descriptorToInsertable(descriptor, config) };

				insertable->init();
				if (!failed_dependency_report.has_value()) {
					std::unordered_set<ResourceDependency> resource_dependencies;
					try {
						resource_dependencies = insertable->insertWithDependencies();
					}
					catch (const Insertable::NoDependencyReportFound& e) {
						failed_dependency_report = e;
					}
					catch (...) {
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

					if (!failed_dependency_report.has_value()) {
						const auto config_dependencies{ insertable->getConfigurationDependencies() };
						entry["resource_dependencies"] = std::vector<json>();
						entry["configuration_dependencies"] = std::vector<json>();

						for (const auto& config_dep : config_dependencies) {
							entry["configuration_dependencies"].push_back(config_dep.toJson());
						}
						for (const auto& resource_dep : resource_dependencies) {
							entry["resource_dependencies"].push_back(resource_dep.toJson());
						}
					}
				}
				else {
					try {
						insertable->insert();
						spdlog::info("");
					}
					catch (...) {
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

				if (descriptor.symbol == Symbol::PATCH) {
					const auto& old_hijacks{ entry["hijacks"] };
					const auto patch{ static_pointer_cast<Patch>(insertable) };
					const auto& new_hijacks{ patch->getHijacks() };

					if (hijacksGoneBad(old_hijacks, new_hijacks)) {
						throw MustRebuildException(fmt::format(
							colors::NOTIFICATION,
							"Hijacks of patch {} have changed, must rebuild", patch->project_relative_path.string()));
					}
					else {
						entry["hijacks"] = new_hijacks;
					}
				}
				
				anything_ran = true;
				if (!any_work_done) {
					if (descriptor.symbol == Symbol::EXTERNAL_TOOL) {
						if (config.generic_tool_configurations.at(descriptor.name.value()).pass_rom.getOrDefault(true)) {
							any_work_done = true;
						}
					}
					else {
						any_work_done = true;
					}
				}
			}
			else {
				if (descriptor.symbol == Symbol::MODULE) {
					std::vector<fs::path> old_outputs{};
					for (const auto& entry : report["module_outputs"][descriptor.name.value()]) {
						old_outputs.push_back(entry);
					}
					copyOldModuleOutput(old_outputs, config.project_root.getOrThrow());
				}

				spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "{} already up to date", descriptor.toString(config.project_root.getOrThrow())));
				spdlog::info("");
			}
		}

		if (any_work_done || anything_ran) {
			if (!failed_dependency_report.has_value()) {
				writeBuildReport(config.project_root.getOrThrow(), createBuildReport(config, report["dependencies"]));
			}
			else {
				spdlog::warn(fmt::format(colors::WARNING, "{}, Update not applicable, read the documentation "
					"on details for how to set up Update correctly", failed_dependency_report.value().what()));
				removeBuildReport(config.project_root.getOrThrow());
			}

			if (any_work_done) {
				cacheModules(config.project_root.getOrThrow());
				Saver::writeMarkerToRom(temporary_rom_path, config);

				moveTempToOutput(config);
			}

			GraphicsUtil::linkOutputRomToProjectGraphics(config, false);
			GraphicsUtil::linkOutputRomToProjectGraphics(config, true);

			try {
				fs::remove_all(config.temporary_folder.getOrThrow());
			}
			catch (const std::runtime_error&) {
				spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary folder '{}'",
					config.temporary_folder.getOrThrow().string()));
			}

			const auto build_end{ std::chrono::high_resolution_clock::now() };

			if (any_work_done) {
				spdlog::info(fmt::format(colors::SUCCESS,
					"Update finished successfully in {} \\(^.^)/", TimeUtil::getDurationString(build_end - build_start)));
				return Result::SUCCESS;
			}
			else {
				spdlog::info(fmt::format(colors::NOTIFICATION, "Everything already up to date, no work for me to do (-.-)"));
				return Result::NO_WORK;
			}
		}
		else {
			try {
				fs::remove_all(config.temporary_folder.getOrThrow());
			}
			catch (const std::runtime_error&) {
				spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary folder '{}'",
					config.temporary_folder.getOrThrow().string()));
			}

			spdlog::info(fmt::format(colors::NOTIFICATION, "Everything already up to date, no work for me to do (-.-)"));
			return Result::NO_WORK;
		}
	}

	void QuickBuilder::checkBuildReportFormat() const {
		if (report["file_format_version"] != BUILD_REPORT_VERSION) {
			throw MustRebuildException(fmt::format(colors::NOTIFICATION, "Build report format has changed, must rebuild"));
		}
	}

	void QuickBuilder::checkBuildOrderChange(const Configuration& config) const {
		if (config.build_order.size() != report["build_order"].size()) {
			throw MustRebuildException(fmt::format(colors::NOTIFICATION, "Build order has changed, must rebuild"));
		}
		
		size_t i{ 0 };
		for (const auto& new_descriptor : config.build_order) {
			const auto old_descriptor{ Descriptor(report["build_order"].at(i++)) };
			if (old_descriptor != new_descriptor) {
				throw MustRebuildException(fmt::format(colors::NOTIFICATION, "Build order has changed, must rebuild"));
			}
		}
	}

	void QuickBuilder::checkProblematicLevelChanges(const fs::path& levels_path, const std::unordered_set<int>& old_level_numbers) {
		std::unordered_set<int> new_level_numbers{};
		
		if (!fs::exists(levels_path)) {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Configured levels folder at '{}' does not exist, but levels were previously inserted into this ROM, "
				"aborting build for safety, if you wish to no longer insert levels, unset the 'levels' path in your configuration",
				levels_path.string()
			));
		}

		for (const auto& entry : fs::directory_iterator(levels_path)) {
			if (entry.path().extension() == ".mwl") {
				try {
					new_level_numbers.insert(Levels::getInternalLevelNumber(entry).value());
				}
				catch (const std::exception& e) {
					throw InsertionException(fmt::format(
						colors::EXCEPTION,
						"Failed to determine source level number of level file '{}' with exception:\n\r{}",
						entry.path().string(), e.what()
					));
				}
			}
		}

		const auto old_missing_from_new{
			std::count_if(old_level_numbers.begin(), old_level_numbers.end(), [&](const auto& l) {
				return !new_level_numbers.contains(l);
			})
		};

		if (old_missing_from_new != 0) {
			throw MustRebuildException(fmt::format(
				colors::NOTIFICATION,
				"{} old level file{} {} been removed, must rebuild",
				old_missing_from_new, old_missing_from_new > 1 ? "s" : "", old_missing_from_new > 1 ? "have" : "has"
			));
		}
	}

	void QuickBuilder::checkRebuildConfigDependencies(const json& dependencies, const Configuration& config) const {
		for (const auto& entry : report["dependencies"]) {
			for (const auto& json_config_dependency : entry["configuration_dependencies"]) {
				const auto config_dependency{ ConfigurationDependency(json_config_dependency) };

				if (config_dependency.policy == Policy::REBUILD) {
					const auto previous_value{ config_dependency.value };
					const auto new_value{ config.getByKey(config_dependency.config_keys) };

					if (previous_value != new_value) {
						throw MustRebuildException(fmt::format(
							colors::NOTIFICATION,
							"Value of {} has changed, must rebuild",
							config_dependency.config_keys
						));
					}
				}
			}
		}
	}

	void QuickBuilder::checkRebuildResourceDependencies(const json& dependencies, const fs::path& project_root, size_t starting_index) const {
		auto entry{ dependencies.begin() + starting_index };
		while (entry != dependencies.end()) {
			for (const auto& json_resource_dependency : (*entry)["resource_dependencies"]) {
				const auto resource_dependency{ ResourceDependency(json_resource_dependency) };

				if (resource_dependency.policy == Policy::REBUILD) {
					std::optional<uint64_t> new_timestamp{
						fs::exists(resource_dependency.dependent_path) ?
						std::make_optional(fs::last_write_time(resource_dependency.dependent_path).time_since_epoch().count()) :
						std::nullopt };
					if (new_timestamp != resource_dependency.last_write_time) {
						throw MustRebuildException(fmt::format(
							colors::NOTIFICATION,
							"Dependency '{}' of '{}' has changed, must rebuild",
							resource_dependency.dependent_path.string(),
							Descriptor((*entry)["descriptor"]).toString(project_root)
						));
					}
				}
			}
			++entry;
		}
	}

	std::optional<ConfigurationDependency> QuickBuilder::checkReinsertConfigDependencies(const json& config_dependencies, 
		const Configuration& config) const {
		for (const auto& entry : config_dependencies) {
			const auto config_dependency{ ConfigurationDependency(entry) };
			if (config_dependency.policy == Policy::REINSERT) {
				const auto previous_value{ config_dependency.value };
				const auto new_value{ config.getByKey(config_dependency.config_keys) };

				if (previous_value != new_value) {
					return config_dependency;
				}
			}
		}
		return {};
	}

	std::optional<ResourceDependency> QuickBuilder::checkReinsertResourceDependencies(const json& resource_dependencies) const {
		for (const auto& entry : resource_dependencies) {
			const auto resource_dependency{ ResourceDependency(entry) };
			if (resource_dependency.policy == Policy::REINSERT) {
				std::optional<uint64_t> new_timestamp{
					fs::exists(resource_dependency.dependent_path) ?
					std::make_optional(fs::last_write_time(resource_dependency.dependent_path).time_since_epoch().count()) :
					std::nullopt };
				if (new_timestamp != resource_dependency.last_write_time) {
					return resource_dependency;
				}
			}
		}
		return {};
	}

	void QuickBuilder::cleanModule(const fs::path& module_source_path, const fs::path& temporary_rom_path, const fs::path& project_root) {
		const auto relative{ fs::relative(module_source_path, project_root) };
		const auto cleanup_file{ PathUtil::getModuleCleanupDirectoryPath(project_root) / 
			((relative.parent_path() / relative.stem()).string() + ".addr")
		};

		if (!fs::exists(cleanup_file)) {
			throw MustRebuildException(fmt::format(
				colors::NOTIFICATION,
				"Cannot clean module {} as its cleanup file is missing, must rebuild",
				module_source_path.string()
			));
		}

		std::string patch_path{ (boost::filesystem::temp_directory_path() / boost::filesystem::unique_path().string()).string() };
		std::ofstream temp_patch{ patch_path };

		std::ifstream module_cleanup_file{ cleanup_file };
		std::string line;
		while (std::getline(module_cleanup_file, line)) {
			const auto address{ std::stoi(line) };
			temp_patch << fmt::format("autoclean ${:06X}\n", address);
		}
		temp_patch.close();

		std::ifstream rom_file(temporary_rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		const patchparams params{
			sizeof(patchparams),
			patch_path.c_str(),
			rom_bytes.data() + header_size,
			MAX_ROM_SIZE,
			&unheadered_rom_size,
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
			true,
			false
		};

		if (!asar_init()) {
			throw ToolNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Asar library file not found, did you forget to copy it alongside callisto?"
			));
		}

		const bool succeeded{ asar_patch_ex(&params) };

		if (succeeded) {
			spdlog::debug(
				"Successfully cleaned module {}",
				module_source_path.string()
			);
			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
		}
		else {
			throw MustRebuildException(fmt::format(
				colors::NOTIFICATION,
				"Failed to clean module {}, must rebuild",
				module_source_path.string()
			));
		}
	}

	void QuickBuilder::copyOldModuleOutput(const std::vector<fs::path>& module_output_paths, const fs::path& project_root) {
		for (const auto& output_path : module_output_paths) {
			const auto relative{ fs::relative(output_path, PathUtil::getUserModuleDirectoryPath(project_root)) };
			const auto source{ PathUtil::getModuleOldSymbolsDirectoryPath(project_root) / relative };

			if (!fs::exists(source)) {
				throw MustRebuildException(fmt::format(
					colors::NOTIFICATION,
					"Previously created module output {} is missing, must rebuild",
					source.string()
				));
			}

			const auto target{ PathUtil::getUserModuleDirectoryPath(project_root) / relative };
			fs::create_directories(target.parent_path());
			fs::copy_file(source, target, fs::copy_options::overwrite_existing);
		}
	}

	bool QuickBuilder::hijacksGoneBad(const std::vector<std::pair<size_t, size_t>>& old_hijacks,
		const std::vector<std::pair<size_t, size_t>>& new_hijacks) {
		std::unordered_set<size_t> new_written_addresses{};

		for (const auto& [address, number] : new_hijacks) {
			for (size_t i{ 0 }; i != number; ++i) {
				new_written_addresses.insert(address + i);
			}
		}

		for (const auto& [address, number] : old_hijacks) {
			for (size_t i{ 0 }; i != number; ++i) {
				if (new_written_addresses.count(address + i) == 0) {
					return true;
				}
			}
		}
		
		return false;
	}
}
 