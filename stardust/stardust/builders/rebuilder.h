#pragma once

#include <chrono>
#include <sstream>

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "../insertables/initial_patch.h"

namespace stardust {
	class Rebuilder : public Builder {
	protected:
		using Writes = std::vector<std::pair<std::string, unsigned char>>;
		using WriteMap = std::map<int, Writes>;
		using ConflictVector = std::vector<std::pair<std::string, std::vector<unsigned char>>>;

		enum class Conflicts {
			NONE,
			HIJACKS,
			ALL
		};

		static json getJsonDependencies(const DependencyVector& dependencies);
		static void reportConflicts(const WriteMap& write_map);
		static void outputConflict(const ConflictVector& conflict_vector, int pc_start_offset, int conflict_size);
		static bool writesAreIdentical(const Writes& writes);
		static int pcToSnes(int address);
		static std::vector<std::string> getWriters(const Writes& writes);
		static std::vector<char> getRom(const fs::path& rom_path);
		static void updateWrites(const std::vector<char>& old_rom, const std::vector<char>& new_rom, 
			Conflicts conflict_policy, WriteMap& write_map, const std::string& descriptor_string);
		static Conflicts determineConflictCheckSetting(const Configuration& config);

	public:
		void build(const Configuration& config) override;
	};
}
