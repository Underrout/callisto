#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class ExGraphics : public LunarMagicInsertable {
	private:
		static const char* EXGRAPHICS_FOLDER_NAME;

		const fs::path project_exgraphics_folder_path;
		const fs::path temporary_exgraphics_folder_path;

		void createTemporaryExGraphicsFolder() const;
		void deleteTemporaryExGraphicsFolder() const;

	public:
		void insert() override;

		ExGraphics(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& output_rom_path);
	};
}
