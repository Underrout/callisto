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
	class ExGraphics : public Insertable {
	private:
		static const char* EXGRAPHICS_FOLDER_NAME;

		const fs::path lunar_magic_path;
		const fs::path project_exgraphics_folder_path;
		const fs::path temporary_exgraphics_folder_path;
		const fs::path temporary_rom_path;

		void createTemporaryExGraphicsFolder() const;
		void deleteTemporaryExGraphicsFolder() const;

	public:
		void insert() override;

		ExGraphics(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& output_rom_path);
	};
}
