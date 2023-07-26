#pragma once

#include <filesystem>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <boost/process/system.hpp>

#include "rom_insertable.h"
#include "../configuration/configuration.h"
#include "../insertion_exception.h"
#include "../dependency/policy.h"
#include "../dependency/resource_dependency.h"

namespace bp = boost::process;
namespace fs = std::filesystem;

namespace callisto {
	class InitialPatch : public RomInsertable {
	protected:
		const fs::path flips_path;
		const fs::path clean_rom_path;
		const fs::path initial_patch_path;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		InitialPatch(const Configuration& config);

		void insert() override;
		void insert(const fs::path& target_rom);
	};
}
