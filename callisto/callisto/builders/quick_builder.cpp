#include "quick_builder.h"

namespace callisto {
	QuickBuilder::QuickBuilder(const fs::path& project_root) {
		const auto build_report_path{ PathUtil::getBuildReportPath(project_root) };
		if (!fs::exists(build_report_path)) {
			throw MustRebuildException(fmt::format(
				"No build report found at {}, must rebuild",
				build_report_path.string()
			));
		}

		std::ifstream build_report{ build_report_path };
		report = json::parse(build_report);
		build_report.close();
	}

	void QuickBuilder::build(const Configuration& config) {
		spdlog::info("Quick Build started");

		init(config);

		if (!fs::exists(config.project_rom.getOrThrow())) {
			throw MustRebuildException(fmt::format("No ROM found at {}, must rebuild", config.project_rom.getOrThrow().string()));
		}

		checkRebuildRomSize(config);
		checkBuildReportFormat();
		checkBuildOrderChange(config);

		auto& json_dependencies{ report["dependencies"] };

		checkRebuildConfigDependencies(json_dependencies, config);

		bool any_work_done{ false };
		std::optional<Insertable::NoDependencyReportFound> failed_dependency_report;
		size_t i{ 0 };
		for (auto& entry : json_dependencies) {
			checkRebuildResourceDependencies(json_dependencies, config.project_root.getOrThrow(), i++);
			const auto descriptor{ Descriptor(entry["descriptor"]) };
			const auto descriptor_string{ descriptor.toString(config.project_root.getOrThrow()) };
			const auto config_result{ 
				checkReinsertConfigDependencies(entry["configuration_dependencies"], config)};
			bool must_reinsert{ false };
			if (config_result.has_value()) {
				spdlog::info(
					"{} must be reinserted due to change in configuration variable {}",
					descriptor_string,
					config_result.value().config_keys
				);
				must_reinsert = true;
			}
			else {
				const auto resource_result{
					checkReinsertResourceDependencies(entry["resource_dependencies"])
				};

				if (resource_result.has_value()) {
					spdlog::info(
						"{} must be reinserted due to change in resource {}",
						descriptor_string,
						(fs::relative(resource_result.value().dependent_path, config.project_root.getOrThrow())).string()
					);
					must_reinsert = true;
				}
			}

			if (must_reinsert) {
				if (!any_work_done) {
					any_work_done = true;
					fs::copy(config.project_rom.getOrThrow(), config.temporary_rom.getOrThrow(), fs::copy_options::overwrite_existing);
				}
				if (descriptor.symbol == Symbol::GLOBULE) {
					cleanGlobule(
						descriptor.name.value(),
						config.temporary_rom.getOrThrow(),
						config.project_root.getOrThrow()
					);
				}

				auto insertable{ descriptorToInsertable(descriptor, config) };

				if (!failed_dependency_report.has_value()) {
					std::unordered_set<ResourceDependency> resource_dependencies;
					try {
						resource_dependencies = insertable->insertWithDependencies();
					}
					catch (const Insertable::NoDependencyReportFound& e) {
						failed_dependency_report = e;
					}

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
					insertable->insert();
				}

				if (descriptor.symbol == Symbol::PATCH) {
					const auto& old_hijacks{ entry["hijacks"] };
					const auto patch{ static_pointer_cast<Patch>(insertable) };
					const auto& new_hijacks{ patch->getHijacks() };

					if (hijacksGoneBad(old_hijacks, new_hijacks)) {
						throw MustRebuildException(fmt::format(
							"Hijacks of patch {} have changed, must rebuild", patch->project_relative_path.string()));
					}
					else {
						entry["hijacks"] = new_hijacks;
					}
				}
			}
			else {
				if (descriptor.symbol == Symbol::GLOBULE) {
					copyGlobule(descriptor.name.value(), config.project_root.getOrThrow());
				}

				spdlog::info("{} already up to date", descriptor.toString(config.project_root.getOrThrow()));
			}
		}

		if (any_work_done) {
			if (!failed_dependency_report.has_value()) {
				writeBuildReport(config.project_root.getOrThrow(), createBuildReport(config, report["dependencies"]));
			}
			else {
				spdlog::warn("{}, Quickbuild not applicable, read the documentation "
					"on details for how to set up Quickbuild correctly", failed_dependency_report.value().what());
				removeBuildReport(config.project_root.getOrThrow());
			}

			cacheGlobules(config.project_root.getOrThrow());
			Saver::writeMarkerToRom(config.temporary_rom.getOrThrow(), config);

			moveTempToOutput(config);

			spdlog::info("Quickbuild finished successfully!");
		}
		else {
			spdlog::info("Everything already up to date!");
		}
	}

	void QuickBuilder::checkBuildReportFormat() const {
		if (report["file_format_version"] != BUILD_REPORT_VERSION) {
			throw MustRebuildException("Build report format has changed, must rebuild");
		}
	}

	void QuickBuilder::checkBuildOrderChange(const Configuration& config) const {
		if (config.build_order.size() != report["build_order"].size()) {
			throw MustRebuildException("Build order has changed, must rebuild");
		}
		
		size_t i{ 0 };
		for (const auto& new_descriptor : config.build_order) {
			const auto old_descriptor{ Descriptor(report["build_order"].at(i++)) };
			if (old_descriptor != new_descriptor) {
				throw MustRebuildException("Build order has changed, must rebuild");
			}
		}
	}

	void QuickBuilder::checkRebuildRomSize(const Configuration& config) const {
		if ((report["rom_size"] == nullptr && config.rom_size.isSet()) || report["rom_size"] != config.rom_size.getOrThrow()) {
			throw MustRebuildException(fmt::format("{} has changed, must rebuild", config.rom_size.name));
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

	void QuickBuilder::cleanGlobule(const fs::path& globule_source_path, const fs::path& temporary_rom_path, const fs::path& project_root) {
		const auto imprint_file{ PathUtil::getInsertedGlobulesDirectoryPath(project_root) / 
			(globule_source_path.stem().string() + ".asm")
		};

		if (!fs::exists(imprint_file)) {
			throw MustRebuildException(fmt::format(
				"Cannot clean globule {} as its imprint is missing, must rebuild",
				globule_source_path.string()
			));
		}

		std::string patch_path{ (boost::filesystem::temp_directory_path() / boost::filesystem::unique_path().string()).string() };
		std::ofstream temp_patch{ patch_path };

		std::ifstream globule_imprint{ imprint_file };
		std::string line;
		while (std::getline(globule_imprint, line)) {
			const auto equal_index{ line.find('=') };
			if (equal_index != -1) {
				const auto address{ line.substr(equal_index + 1) };
				temp_patch << "autoclean " << address << std::endl;
			}
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
			false,
			true
		};

		if (!asar_init()) {
			throw ToolNotFoundException(
				"Asar library file not found, did you forget to copy it alongside callisto?"
			);
		}

		const bool succeeded{ asar_patch_ex(&params) };

		if (succeeded) {
			spdlog::debug(
				"Successfully cleaned globule {}",
				globule_source_path.string()
			);
			std::ofstream out_rom{ temporary_rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
		}
		else {
			throw MustRebuildException(fmt::format(
				"Failed to clean globule {}, must rebuild",
				globule_source_path.string()
			));
		}
	}

	void QuickBuilder::copyGlobule(const fs::path& globule_source_path, const fs::path& project_root) {
		const auto imprint_name{ (globule_source_path.stem().string() + ".asm") };
		const auto imprint_file{ PathUtil::getInsertedGlobulesDirectoryPath(project_root) / imprint_name };

		if (!fs::exists(imprint_file)) {
			throw MustRebuildException(fmt::format(
				"Imprint of globule {} is missing, must rebuild",
				globule_source_path.string()
			));
		}

		const auto target{ PathUtil::getGlobuleImprintDirectoryPath(project_root) / imprint_name };
		fs::copy_file(imprint_file, target, fs::copy_options::overwrite_existing);
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
 