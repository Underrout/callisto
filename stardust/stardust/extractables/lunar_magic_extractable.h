#pragma once

#include <filesystem>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <boost/process/system.hpp>

#include "extractable.h"
#include "../configuration/configuration.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class LunarMagicExtractable : public Extractable {
	protected:
		const fs::path lunar_magic_executable;
		const fs::path extracting_rom;

		template<typename... Args>
		int callLunarMagic(Args... args) const {
			return bp::system(lunar_magic_executable.string(), args...);
		}

	public:
		LunarMagicExtractable(const Configuration& config, const fs::path& extracting_rom);
	};
}
