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

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	class Patch : public RomInsertable {
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;

		const fs::path patch_path;
		std::vector<fs::path> additional_include_paths;
		std::vector<std::pair<size_t, size_t>> hijacks{};

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void fixAsarMemoryLeak() const;

	public:
		const fs::path project_relative_path;

		const std::vector<std::pair<size_t, size_t>>& getHijacks() const;

		Patch(const Configuration& config,
			const fs::path& patch_path, const std::vector<fs::path>& additional_include_paths = {});

		void insert() override;
	};
}
