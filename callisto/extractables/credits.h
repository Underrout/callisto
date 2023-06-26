#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_extractable.h"
#include "../not_found_exception.h"

namespace callisto {
	namespace extractables {
		class Credits : public FlipsExtractable {
		private:
			inline std::string getTemporaryResourceRomPostfix() const {
				return "_credits";
			}

			inline std::string getLunarMagicFlag() const {
				return "-TransferCredits";
			}

			inline std::string getResourceName() const {
				return "Credits";
			}

		public:
			Credits(const Configuration& config, const fs::path& extracting_rom)
				: FlipsExtractable(config, config.credits.getOrThrow(), extracting_rom) {}
		};
	}
}
