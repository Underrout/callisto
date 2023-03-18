#pragma once

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"

namespace stardust {
	class Rebuilder : public Builder {
	public:
		void build(const Configuration& config) override;
	};
}
