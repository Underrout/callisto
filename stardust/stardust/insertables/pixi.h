#pragma once

#include "../insertable.h"

#include <boost/process/system.hpp>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class Pixi : public RomInsertable {
	private:
		const fs::path pixi_exe_path;
		const std::string pixi_options;

	public:
		Pixi(const fs::path& pixi_exe_path, const fs::path& temporary_rom_path, const std::string& pixi_options);

		void insert() override;
	};
}
