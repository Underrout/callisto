#pragma once

#include <chrono>

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "interval.h"
#include "../insertables/initial_patch.h"

namespace stardust {
	class Rebuilder : public Builder {
	protected:
		enum class Conflicts {
			NONE,
			HIJACKS,
			ALL
		};

		static json getJsonDependencies(const DependencyVector& dependencies);
		static void checkConflicts(const std::unordered_map<Descriptor, std::vector<Interval>>& written_intervals, const fs::path& project_root);
		static std::vector<char> getRom(const fs::path& rom_path);
		static std::vector<Interval> getDifferences(const std::vector<char> old_rom, const std::vector<char> new_rom, Conflicts conflict_policy);
		static Conflicts determineConflictCheckSetting(const Configuration& config);

	public:
		void build(const Configuration& config) override;
	};
}
