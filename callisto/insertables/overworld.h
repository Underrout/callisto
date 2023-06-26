#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

#include "../configuration/configuration.h"

namespace callisto {
	class Overworld : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
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
			: FlipsInsertable(config, config.overworld)
		{
			checkPatchExists();
		}
	};
}
