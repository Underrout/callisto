#pragma once

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "interval.h"
#include "../insertables/initial_patch.h"

namespace stardust {
	class QuickBuilder : public Builder {
	public:
		void build(const Configuration& config) override;

		QuickBuilder(const json& j);
	};
}
