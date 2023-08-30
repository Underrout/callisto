#pragma once

#include <string>
#include <filesystem>

#include <fmt/format.h>

#include <boost/process/system.hpp>

#include "../insertable.h"
#include "../not_found_exception.h"
#include "rom_insertable.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace bp = boost::process;
namespace fs = std::filesystem;

namespace callisto {
	class LunarMagicInsertable : public RomInsertable {
	protected:
		const fs::path lunar_magic_path;

		LunarMagicInsertable(const Configuration& config);

		template<typename... Args>
		int callLunarMagic(Args... args) {
			return bp::system(lunar_magic_path.string(), args...);
		}

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void checkLunarMagicExists();
	};
}
