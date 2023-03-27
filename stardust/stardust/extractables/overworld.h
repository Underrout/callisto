#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_extractable.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"

namespace stardust {
	namespace extractables {
		class Overworld : public FlipsExtractable {
		private:
			inline std::string getTemporaryResourceRomPostfix() const {
				return "_overworld";
			}

			inline std::string getLunarMagicFlag() const {
				return "-TransferOverworld";
			}

			inline std::string getResourceName() const {
				return "Overworld";
			}

		public:
			Overworld(const Configuration& config)
				: FlipsExtractable(config, config.overworld.getOrThrow()) {}
		};
	}
}
