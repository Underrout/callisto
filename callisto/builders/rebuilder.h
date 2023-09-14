#pragma once

#include <chrono>
#include <sstream>

#include <boost/range/adaptor/reversed.hpp>
#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "../insertables/initial_patch.h"

namespace callisto {
	class Rebuilder : public Builder {
	protected:
		using Writes = std::vector<std::pair<std::string, unsigned char>>;
		using WriteMap = std::map<int, Writes>;
		using ConflictVector = std::vector<std::pair<std::string, std::vector<unsigned char>>>;
		using PatchHijacksVector = std::vector<std::optional<std::vector<std::pair<size_t, size_t>>>>;

		enum class Conflicts {
			NONE,
			HIJACKS,
			ALL
		};

		static json getJsonDependencies(const DependencyVector& dependencies, const PatchHijacksVector& hijacks);
		static void reportConflicts(std::shared_ptr<WriteMap> write_map, const std::optional<fs::path>& log_file_path,
			Conflicts conflict_policy, std::exception_ptr conflict_exception, const std::unordered_set<Descriptor>& ignored_descriptors,
			const fs::path& project_root);
		static std::string getConflictString(const ConflictVector& conflict_vector, 
			int pc_start_offset, int conflict_size, bool for_console = true);
		static bool writesAreIdentical(const Writes& writes, const std::unordered_set<std::string>& ignored_names);
		static int pcToSnes(int address);
		static std::vector<std::string> getWriters(const Writes& writes);
		static std::vector<char> getRom(const fs::path& rom_path);
		static void updateWrites(std::shared_ptr<std::vector<char>> old_rom, std::shared_ptr<std::vector<char>> new_rom,
			Conflicts conflict_policy, std::shared_ptr<WriteMap> write_map, const std::string& descriptor_string);
		static Conflicts determineConflictCheckSetting(const Configuration& config);

	public:
		void build(const Configuration& config);
	};
}
