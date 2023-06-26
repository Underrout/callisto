#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_extractable.h"
#include "../not_found_exception.h"

namespace callisto {
	namespace extractables {
		class GlobalExAnimation : public FlipsExtractable {
		private:
			inline std::string getTemporaryResourceRomPostfix() const {
				return "_global_exanimation";
			}

			inline std::string getLunarMagicFlag() const {
				return "-TransferLevelGlobalExAnim";
			}

			inline std::string getResourceName() const {
				return "Global ExAnimation";
			}

		public:
			GlobalExAnimation(const Configuration& config, const fs::path& extracting_rom)
				: FlipsExtractable(config, config.global_exanimation.getOrThrow(), extracting_rom) {}
		};
	}
}
