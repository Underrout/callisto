#include "cli_handler.h"

namespace callisto {
	int CLIHandler::run(int argc, char** argv) {
		const auto plain_path{ fs::path(argv[0]) };
		const auto callisto_path{ fs::canonical(plain_path) };
		const auto callisto_directory{ callisto_path.parent_path() };
		ConfigurationManager config_manager{ callisto_directory };

		LunarMagicWrapper lunar_magic_wrapper{};

		spdlog::set_level(spdlog::level::info);
		spdlog::set_pattern("%v");

		CLI::App app;
		app.require_subcommand(1, 1);

		std::optional<size_t> max_thread_count;
		bool allow_user_input{ true };
		bool check_for_pending_save{ true };

		app.add_option(
			"--max-threads",
			max_thread_count,
			"Maximum number of threads to use"
		);

		app.add_option(
			"--allow-user-input",
			allow_user_input,
			"Whether to allow user input for callisto prompts (default is yes)"
		);

		app.add_option(
			"--check-pending-save",
			check_for_pending_save,
			"Whether to check for a pending automatic resource export before saving/building"
		);

		auto init{ [&] {
			if (max_thread_count.has_value()) {
				globals::setMaxThreadCount(max_thread_count.value());
			}

			globals::ALLOW_USER_INPUT = allow_user_input;
		} };

		auto build_sub{ app.add_subcommand("rebuild", "Builds your ROM from scratch")->fallthrough() };
		auto update_sub{ app.add_subcommand("update", "Brings your ROM up to date with project files")->fallthrough() };
		auto save_sub{ app.add_subcommand("save", "Exports project files from ROM")->fallthrough() };
		auto edit_sub{ app.add_subcommand("edit", "Opens project ROM in Lunar Magic")->fallthrough() };
		auto package_sub{ app.add_subcommand("package", "Packages project ROM into a BPS patch")->fallthrough() };
		auto profiles_sub{ app.add_subcommand("profiles", "Lists available configuration profiles")->fallthrough() };

		bool abort_on_unsaved{ false };
		build_sub->add_flag(
			"--no-export",
			abort_on_unsaved,
			"Will cause the build to abort if there are unsaved resources in the ROM, default is to export them and then build"
		);

		update_sub->add_flag(
			"--no-export",
			abort_on_unsaved,
			"Will cause the build to abort if there are unsaved resources in the ROM, default is to export them and then build"
		);

		std::optional<std::string> profile_name{};
		build_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to build with"
		);

		update_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to update with"
		);

		build_sub->callback([&] {
			init();
			const auto config{ config_manager.getConfiguration(profile_name) };

#ifdef _WIN32
			if (check_for_pending_save && config->lunar_magic_path.isSet()) {
				lunar_magic_wrapper.attemptReattach(config->lunar_magic_path.getOrThrow());
				if (lunar_magic_wrapper.pendingEloperSave().has_value()) {
					throw std::runtime_error("There is a pending automatic resource export, refusing to rebuild to avoid conflicting with it");
				}
			}
#endif

			if (config->output_rom.isSet() && fs::exists(config->output_rom.getOrThrow())) {
				const auto needs_extraction{ Marker::getNeededExtractions(config->output_rom.getOrThrow(),
					config->project_root.getOrThrow(),
					Saver::getExtractableTypes(*config),
					config->use_text_map16_format.getOrDefault(false)) 
				};

				if (!needs_extraction.empty()) {
					if (abort_on_unsaved) {
						spdlog::error("There are unsaved resources in ROM '{}', aborting rebuild", config->output_rom.getOrThrow().string());
						exit(2);
					}
					Saver::exportResources(config->output_rom.getOrThrow(), *config, true);
				}
			}

			Rebuilder rebuilder{};
			rebuilder.build(*config);
#ifdef _WIN32
			if (config->lunar_magic_path.isSet() && config->enable_automatic_reloads.getOrDefault(true)) {
				lunar_magic_wrapper.reloadRom(config->output_rom.getOrThrow());
			}
#endif

			exit(0);
		});

		update_sub->callback([&] {
			init();
			const auto config{ config_manager.getConfiguration(profile_name) };

#ifdef _WIN32
			if (check_for_pending_save && config->lunar_magic_path.isSet()) {
				lunar_magic_wrapper.attemptReattach(config->lunar_magic_path.getOrThrow());
				if (lunar_magic_wrapper.pendingEloperSave().has_value()) {
					throw std::runtime_error("There is a pending automatic resource export, refusing to update to avoid conflicting with it");
				}
			}
#endif
			if (config->output_rom.isSet() && fs::exists(config->output_rom.getOrThrow())) {
				const auto needs_extraction{ Marker::getNeededExtractions(config->output_rom.getOrThrow(),
					config->project_root.getOrThrow(),
					Saver::getExtractableTypes(*config),
					config->use_text_map16_format.getOrDefault(false))
				};

				if (!needs_extraction.empty()) {
					if (abort_on_unsaved) {
						spdlog::error("There are unsaved resources in ROM '{}', aborting update", config->output_rom.getOrThrow().string());
						exit(2);
					}
					Saver::exportResources(config->output_rom.getOrThrow(), *config, true);
				}
			}

			try {
				QuickBuilder quick_builder{ config->project_root.getOrThrow() };
				const auto result{ quick_builder.build(*config) };
#ifdef _WIN32
				if (result == QuickBuilder::Result::SUCCESS && config->lunar_magic_path.isSet()
					&& config->enable_automatic_reloads.getOrDefault(true)) {
					lunar_magic_wrapper.reloadRom(config->output_rom.getOrThrow());
				}
#endif
			}
			catch (const MustRebuildException& e) {
				spdlog::info("Update cannot continue due to the following reason, rebuilding ROM:\n\r{}\n", e.what());
				Rebuilder rebuilder{};
				rebuilder.build(*config);
#ifdef _WIN32
				if (config->lunar_magic_path.isSet() && config->enable_automatic_reloads.getOrDefault(true)) {
					lunar_magic_wrapper.reloadRom(config->output_rom.getOrThrow());
				}
#endif
			}
		});

		save_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to save with"
		);

		save_sub->callback([&] {
			init();
			const auto config{ config_manager.getConfiguration(profile_name) };

#ifdef _WIN32
			if (check_for_pending_save && config->lunar_magic_path.isSet()) {
				lunar_magic_wrapper.attemptReattach(config->lunar_magic_path.getOrThrow());
				if (lunar_magic_wrapper.pendingEloperSave().has_value()) {
					throw std::runtime_error("There is a pending automatic resource export, refusing to save to avoid conflicting with it");
				}
			}
#endif
			Saver::exportResources(config->output_rom.getOrThrow(), *config, true);
			exit(0);
		});

		edit_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to edit with"
		);

		edit_sub->callback([&] {
			init();
			const auto config{ config_manager.getConfiguration(profile_name) };

#ifdef _WIN32
			lunar_magic_wrapper.attemptReattach(config->lunar_magic_path.getOrThrow());
#endif
			lunar_magic_wrapper.bringToFrontOrOpen(
				callisto_path, config->lunar_magic_path.getOrThrow(), config->output_rom.getOrThrow());
			exit(0);
		});

		package_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to package with"
		);

		package_sub->callback([&] {
			init();
			const auto config{ config_manager.getConfiguration(profile_name) };

			const auto exit_code{ bp::system(
				config->flips_path.getOrThrow().string(), "--create", "--bps-delta", config->clean_rom.getOrThrow().string(),
				config->output_rom.getOrThrow().string(), config->bps_package.getOrThrow().string(), bp::std_out > bp::null
			) };

			if (exit_code != 0) {
				throw std::runtime_error(fmt::format("Failed to create package of '{}' at '{}'",
					config->output_rom.getOrThrow().string(),
					config->bps_package.getOrThrow().string()
				));
			}

			spdlog::info("Successfully created package of '{}' at '{}'", 
				config->output_rom.getOrThrow().string(), config->bps_package.getOrThrow().string());

			exit(0);
		});

		profiles_sub->callback([&] {
			fmt::print("{}", fmt::join(config_manager.getProfileNames(), "\n"));
			exit(0);
		});

		try {
			CLI11_PARSE(app, argc, argv);
		}
		catch (const std::exception &e) {
			spdlog::error(e.what());
			exit(2);
		}

		exit(0);
	}
}
