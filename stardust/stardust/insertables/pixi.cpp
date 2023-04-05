#include "pixi.h"

#ifdef _WIN32
#define SPLIT boost::program_options::split_winmain
#else
#define SPLIT boost::program_options::split_unix
#endif

namespace stardust {
	void Pixi::insert() {
		spdlog::info("Running PIXI");

		if (dependency_report_file_path.has_value()) {
			// delete potential previous dependency report
			fs::remove(dependency_report_file_path.value());
		}

		std::vector<std::string> args{ SPLIT(pixi_options) };

		args.push_back(temporary_rom_path.string());

		std::vector<const char*> char_args{};
		std::transform(args.begin(), args.end(), std::back_inserter(char_args),
			[](const std::string& s) { return s.c_str(); });

		const auto prev_folder{ fs::current_path() };
		fs::current_path(pixi_folder_path);
		const auto exit_code{ pixi_run(char_args.size(), char_args.data(), false)};
		fs::current_path(prev_folder);

		int pixi_output_size;
		const auto arr{ pixi_output(&pixi_output_size) };

		int i{ 0 };
		while (i != pixi_output_size) {
			spdlog::info(arr[i++]);
		}

		if (exit_code == 0) {
			spdlog::info("Successfully ran PIXI!");
		}
		else {
			int size;
			const std::string error{ pixi_last_error(&size) };

			spdlog::error(error);

			throw InsertionException(fmt::format(
				"Failed to insert PIXI"
			));
		}
	}

	std::unordered_set<ResourceDependency> Pixi::determineDependencies() {
		if (!dependency_report_file_path.has_value()) {
			throw DependencyException("No dependency report file specified for PIXI");
		}

		if (!fs::exists(dependency_report_file_path.value())) {
			throw DependencyException(fmt::format(
				"No dependency report file found at {}, are you sure this is the "
				"correct path and that you have installed quickbuild correctly?",
				dependency_report_file_path.value().string()
			));
		}

		std::unordered_set<ResourceDependency> dependencies{ static_dependencies.begin(), static_dependencies.end() };
		const auto reported{ Insertable::extractDependenciesFromReport(dependency_report_file_path.value()) };

		dependencies.insert(reported.begin(), reported.end());

		return dependencies;
	}

	Pixi::Pixi(const Configuration& config)
		: RomInsertable(config), 
		pixi_folder_path(registerConfigurationDependency(config.pixi_working_dir, Policy::REINSERT).getOrThrow()), 
		pixi_options(registerConfigurationDependency(config.pixi_options, Policy::REINSERT).getOrDefault("")),
		static_dependencies(config.pixi_static_dependencies.getOrDefault({})),
		dependency_report_file_path(config.pixi_dependency_report_file.getOrDefault({}))
	{
		// delete potential previous dependency report
		if (dependency_report_file_path.has_value()) {
			fs::remove(dependency_report_file_path.value());
		}

		if (!fs::exists(pixi_folder_path)) {
			throw NotFoundException(fmt::format(
				"PIXI folder {} does not exist",
				pixi_folder_path.string()
			));
		}
	}
}
