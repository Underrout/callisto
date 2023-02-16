#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class BinaryMap16 : public LunarMagicInsertable {
	private:
		const fs::path map16_file_path;

	public:
		void insert() override;

		BinaryMap16(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& map16_file_path);
	};
}
