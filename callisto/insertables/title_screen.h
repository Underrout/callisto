#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace callisto {
	class TitleScreen : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_title_screen";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferTitleScreen";
		}

		inline std::string getResourceName() const {
			return "Title Screen";
		}

	public:
		TitleScreen(const Configuration& config)
			: FlipsInsertable(config, config.titlescreen) 
		{
			checkPatchExists();
		}
	};
}
