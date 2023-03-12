#include "pixi.h"

namespace stardust {
	void Pixi::insert() {
		spdlog::info("Running PIXI");

		// TODO this is apparently not portable...
		std::vector<std::string> args{ boost::program_options::split_winmain(pixi_options) };

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

	std::unordered_set<Dependency> Pixi::determineDependencies() {
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

		auto dependencies{ static_dependencies };
		const auto reported{ Insertable::extractDependenciesFromReport(dependency_report_file_path.value()) };

		dependencies.insert(reported.begin(), reported.end());
		
		// little hacky, basically just checks if for any .asm file there exists a .cfg or .json file with the same name and location, 
		// if so, they're added as dependencies, should work well in practice but may include superflous files in some edge cases
		for (const auto& dependency : reported) {
			if (dependency.dependent_path.extension() == ".asm") {
				const auto cfg{ fs::path(dependency.dependent_path).replace_extension(".cfg") };
				const auto json{ fs::path(dependency.dependent_path).replace_extension(".json") };

				if (fs::exists(cfg)) {
					dependencies.insert(Dependency(cfg));
				}
				if (fs::exists(json)) {
					dependencies.insert(Dependency(json));
				}
			}
		}

		return dependencies;
	}

	Pixi::Pixi(const fs::path& pixi_folder_path, const fs::path& temporary_rom_path, const std::string& pixi_options,
		const std::unordered_set<Dependency>& static_dependencies, const std::optional<fs::path>& dependency_report_file_path)
		: RomInsertable(temporary_rom_path), pixi_folder_path(pixi_folder_path), pixi_options(pixi_options),
		static_dependencies(static_dependencies), dependency_report_file_path(dependency_report_file_path)
	{
		if (!fs::exists(pixi_folder_path)) {
			throw NotFoundException(fmt::format(
				"PIXI folder {} does not exist",
				pixi_folder_path.string()
			));
		}
	}
}
