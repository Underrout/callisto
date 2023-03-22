#pragma once

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "interval.h"

namespace stardust {
	class Rebuilder : public Builder {
	protected:
		static json getJsonDependencies(const DependencyVector& dependencies);
		static void checkPatchConflicts(const Insertables& insertables);

	public:
		void build(const Configuration& config) override;
	};
}
