#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace callisto {
	class Credits : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_credits";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferCredits";
		}

		inline std::string getResourceName() const {
			return "Credits";
		}

	public:
		Credits(const Configuration& config) 
			: FlipsInsertable(config, config.credits)
		{
			checkPatchExists();
		}
	};
}
