#pragma once

#include <filesystem>

#include <boost/program_options/parsers.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <pixi_api.h>

#include "rom_insertable.h"
#include "../insertion_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class Pixi : public RomInsertable {
	private:
		const fs::path pixi_folder_path;
		const std::string pixi_options;

	public:
		Pixi(const fs::path& pixi_folder_path, const fs::path& temporary_rom_path, const std::string& pixi_options);

		void insert() override;
	};
}
