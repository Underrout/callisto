#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../graphics_util.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	namespace extractables {
		class Graphics : public LunarMagicExtractable {
		protected:
			const Configuration& config;

		public:
			void extract() override;

			Graphics(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
