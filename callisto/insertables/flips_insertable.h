#pragma once

#include <string>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <boost/process/system.hpp>

#include "lunar_magic_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"
#include "rom_insertable.h"

#include "../graphics_util.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace bp = boost::process;
namespace fs = std::filesystem;

namespace callisto {
	class FlipsInsertable : public LunarMagicInsertable {
	protected:
		const fs::path flips_path;
		const fs::path clean_rom_path;
		const fs::path bps_patch_path;
		fs::path temporary_patched_rom_path;

		const Configuration& config;

		bool needs_exgfx{ false };
		bool needs_gfx{ false };

		fs::path getTemporaryPatchedRomPath() const;
		fs::path createTemporaryPatchedRom() const;
		void deleteTemporaryPatchedRom(const fs::path& patched_rom_path) const;

		virtual inline std::string getTemporaryPatchedRomPostfix() const = 0;
		virtual inline std::string getLunarMagicFlag() const = 0;
		virtual inline std::string getResourceName() const = 0;

		int bpsToRom(const fs::path& bps_path, const fs::path& output_rom_path) const;

		void checkPatchExists() const;

		FlipsInsertable(const Configuration& config, const PathConfigVariable& bps_patch_path);

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void init() override;
		void insert() override;
	};
}
