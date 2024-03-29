#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_extractable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace callisto {
	namespace extractables {
		class TitleScreen : public FlipsExtractable {
		private:
			inline std::string getTemporaryResourceRomPostfix() const {
				return "_title_screen";
			}

			inline std::string getLunarMagicFlag() const {
				return "-TransferTitleScreen";
			}

			inline std::string getResourceName() const {
				return "Title Screen";
			}

		public:
			TitleScreen(const Configuration& config, const fs::path& extracting_rom) 
				: FlipsExtractable(config, config.titlescreen.getOrThrow(), extracting_rom) {}
		};
	}
}
