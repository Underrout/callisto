#include "pixi.h"

namespace stardust {
	void Pixi::insert() {
		spdlog::info("Running PIXI");

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

	Pixi::Pixi(const fs::path& pixi_folder_path, const fs::path& temporary_rom_path, const std::string& pixi_options)
		: RomInsertable(temporary_rom_path), pixi_folder_path(pixi_folder_path), pixi_options(pixi_options) 
	{
		if (!fs::exists(pixi_folder_path)) {
			throw NotFoundException(fmt::format(
				"PIXI folder {} does not exist",
				pixi_folder_path.string()
			));
		}
	}
}
