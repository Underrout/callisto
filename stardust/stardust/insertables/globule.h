#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unordered_set>
#include <optional>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <asar-dll-bindings/c/asardll.h>
#include <asar/warnings.h>
#include <boost/filesystem.hpp>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"



namespace fs = std::filesystem;

namespace stardust {
	class Globule : public RomInsertable {
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;

		const fs::path globule_path;
		const fs::path project_relative_path;
		std::vector<const char*> additional_include_paths;
		const fs::path imprint_directory;
		const fs::path globule_call_file;
		const std::unordered_set<std::string> other_globule_names;
		const std::optional<fs::path> globule_header_file;

		void emitImprintFile() const;

		std::unordered_set<Dependency> determineDependencies() override;

	public:

		Globule(const fs::path& project_root_path, const fs::path& temporary_rom_path,
			const fs::path& globule_path, const fs::path& imprint_directory,
			const fs::path& globule_call_file, 
			const std::unordered_set<std::string>& other_globule_names,
			const std::optional<fs::path> globule_header_file,
			const std::vector<fs::path>& additional_include_paths = {});

		void insert() override;
	};
}
