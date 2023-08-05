#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unordered_set>
#include <optional>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include "asar-dll-bindings/c/asardll.h"
#include <asar/warnings.h>
#include <boost/filesystem.hpp>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	class Module : public RomInsertable {
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;

		std::string patch_string{};

		const fs::path module_path;
		const fs::path project_relative_path;
		std::vector<const char*> additional_include_paths;
		const fs::path imprint_directory;
		const fs::path callisto_asm_file;
		std::unordered_set<std::string> other_module_names;
		const std::optional<fs::path> module_header_file;

		void emitImprintFile() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void fixAsarMemoryLeak() const;

		void verifyWrittenBlockCoverage() const;
		void verifyNonHijacking() const;

	public:
		static std::string modulePathToName(const fs::path& path);

		Module(const Configuration& config,
			const fs::path& module_path, const fs::path& imprint_directory,
			const fs::path& callisto_asm_file,
			const std::vector<fs::path>& other_module_paths,
			const std::vector<fs::path>& additional_include_paths = {});

		void init() override;
		void insert() override;
	};
}
