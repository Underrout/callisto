#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace stardust {
	class GlobalExAnimation : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_global_exanimation";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferLevelGlobalExAnim";
		}

		inline std::string getResourceName() const {
			return "Global ExAnimation";
		}

	public:
		GlobalExAnimation(const Configuration& config)
			: FlipsInsertable(config, config.global_exanimation)
		{
			checkPatchExists();
		}
	};
}
