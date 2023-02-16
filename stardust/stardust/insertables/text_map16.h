#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class TextMap16 : public LunarMagicInsertable {
	private:
		const fs::path map16_folder_path;
		const fs::path conversion_tool_path;

		fs::path getTemporaryMap16FilePath() const;
		fs::path generateTemporaryMap16File() const;
		void deleteTemporaryMap16File() const;

	public:
		void insert() override;

		TextMap16(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, 
			const fs::path& map16_folder_path, const fs::path& conversion_tool_path);
	};
}
