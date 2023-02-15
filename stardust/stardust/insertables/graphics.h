#pragma once

#include <filesystem>

#include <boost/process/system.hpp>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "../insertable.h"
#include "../insertion_exception.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class Graphics : public Insertable {
	private:
		static const char* GRAPHICS_FOLDER_NAME;

		const fs::path lunar_magic_path;
		const fs::path project_graphics_folder_path;
		const fs::path temporary_graphics_folder_path;
		const fs::path temporary_rom_path;

		void createTemporaryGraphicsFolder() const;
		void deleteTemporaryGraphicsFolder() const;

	public:
		void insert() override;

		Graphics(const fs::path &lunar_magic_path, const fs::path &temporary_rom_path, const fs::path &output_rom_path);
	};
}
