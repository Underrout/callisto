#include "cli_handler.h"

namespace callisto {
	int CLIHandler::run(int argc, char** argv) {
		const auto callisto_directory{ (fs::path(argv[0])).parent_path() };
		ConfigurationManager config_manager{ callisto_directory };

		spdlog::set_level(spdlog::level::info);
		spdlog::set_pattern("[%^%l%$] %v");

		CLI::App app;
		app.require_subcommand(1, 1);

		auto build_sub{ app.add_subcommand("build", "Builds a ROM from project files")};
		auto save_sub{ app.add_subcommand("save", "Exports project files from ROM")};
		auto edit_sub{ app.add_subcommand("edit", "Opens project ROM in Lunar Magic")};
		auto package_sub{ app.add_subcommand("package", "Packages project ROM into a BPS patch")};
		auto profiles_sub{ app.add_subcommand("profiles", "Lists available configuration profiles")};

		bool quick_build{ false };
		build_sub->add_flag(
			"-q,--quick", 
			quick_build,
			"Only perform required insertions rather than rebuilding ROM from scratch"
		);

		bool abort_on_unsaved{ false };
		build_sub->add_flag(
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

		build_sub->callback([&] {
			const auto config{ config_manager.getConfiguration(profile_name, true) };

			if (config->project_rom.isSet() && fs::exists(config->project_rom.getOrThrow())) {
				const auto needs_extraction{ Marker::getNeededExtractions(config->project_rom.getOrThrow(),
					config->project_root.getOrThrow(),
					Saver::getExtractableTypes(*config),
					config->use_text_map16_format.getOrDefault(false)) 
				};

				if (!needs_extraction.empty()) {
					if (abort_on_unsaved) {
						spdlog::error("There are unsaved resources in ROM '{}', aborting build", config->project_rom.getOrThrow().string());
						exit(2);
					}
					Saver::exportResources(config->project_rom.getOrThrow(), *config, true);
				}
			}

			if (quick_build) {
				try {
					QuickBuilder quick_builder{ config->project_root.getOrThrow() };
					quick_builder.build(*config);
				}
				catch (const MustRebuildException& e) {
					spdlog::info("Quickbuild cannot continue due to the following reason, rebuilding ROM:\n\r{}", e.what());
					Rebuilder rebuilder{};
					rebuilder.build(*config);
				}
			}
			else {
				Rebuilder rebuilder{};
				rebuilder.build(*config);
			}
			exit(0);
		});

		save_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to save with"
		);

		save_sub->callback([&] {
			const auto config{ config_manager.getConfiguration(profile_name, true) };

			Saver::exportResources(config->project_rom.getOrThrow(), *config, true);
			exit(0);
		});

		edit_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to edit with"
		);

		edit_sub->callback([&] {
			const auto config{ config_manager.getConfiguration(profile_name, true) };

			bp::spawn(fmt::format(
				"\"{}\" \"{}\"",
				config->lunar_magic_path.getOrThrow().string(), config->project_rom.getOrThrow().string()
			));
			exit(0);
		});

		package_sub->add_option(
			"-p,--profile",
			profile_name,
			"The profile to package with"
		);

		package_sub->callback([&] {
			const auto config{ config_manager.getConfiguration(profile_name, true) };

			const auto exit_code{ bp::system(
				config->flips_path.getOrThrow().string(), "--create", "--bps-delta", config->clean_rom.getOrThrow().string(),
				config->project_rom.getOrThrow().string(), config->bps_package.getOrThrow().string(), bp::std_out > bp::null
			) };

			if (exit_code != 0) {
				throw std::runtime_error(fmt::format("Failed to create package of '{}' at '{}'",
					config->project_rom.getOrThrow().string(),
					config->bps_package.getOrThrow().string()
				));
			}

			spdlog::info("Successfully created package of '{}' at '{}'", 
				config->project_rom.getOrThrow().string(), config->bps_package.getOrThrow().string());

			exit(0);
		});

		profiles_sub->callback([&] {
			fmt::print("{}\n", fmt::join(config_manager.getProfileNames(), ","));
			exit(0);
		});

		try {
			CLI11_PARSE(app, argc, argv);
		}
		catch (const std::exception &e) {
			spdlog::error(e.what());
			exit(1);
		}

		exit(0);
	}
}
