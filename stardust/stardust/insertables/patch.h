#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <asar-dll-bindings/c/asardll.h>
#include <asar/warnings.h>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;

namespace fs = std::filesystem;

namespace stardust {
	class Patch : public RomInsertable {
	private:
		const fs::path patch_path;
		const fs::path project_relative_path;
		std::vector<const char*> additional_include_paths;

	public:
		Patch(const fs::path& project_root_path, const fs::path& temporary_rom, 
			const fs::path& patch_path, const std::vector<fs::path>& additional_include_paths = {});

		void insert() override;
	};
}
