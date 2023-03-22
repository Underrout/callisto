#include "rebuilder.h"

namespace stardust {
	void Rebuilder::build(const Configuration& config) {
		DependencyVector dependencies{};

		auto insertables{ buildOrderToInsertables(config) };

		for (auto& [descriptor, insertable] : insertables) {
			const auto resource_dependencies{ insertable->insertWithDependencies() };
			const auto config_dependencies{ insertable->getConfigurationDependencies() };

			dependencies.push_back({ descriptor, { resource_dependencies, config_dependencies } });
		}

		checkPatchConflicts(insertables);
	}

	json Rebuilder::getJsonDependencies(const DependencyVector& dependencies) {
		json j;

		for (const auto& [descriptor, pair] : dependencies) {
			auto block{ json({
				{"descriptor", descriptor.toJson()},
				{"resource_dependencies", std::vector<json>()},
				{"configuration_dependencies", std::vector<json>()}
			}) };

			for (const auto& resource_dependency : pair.first) {
				block["resource_dependencies"].push_back(resource_dependency.toJson());
			}

			for (const auto& config_dependency : pair.second) {
				block["configuration_dependencies"].push_back(config_dependency.toJson());
			}

			j.push_back(block);
		}

		return j;
	}

	void Rebuilder::checkPatchConflicts(const Insertables& insertables) {
		std::map<std::string, std::vector<Interval>> written_intervals{};

		for (const auto& [descriptor, insertable] : insertables) {
			if (descriptor.symbol == Symbol::PATCH) {
				const auto patch{ std::dynamic_pointer_cast<Patch>(insertable) };
				const auto patch_name{ patch->project_relative_path.string() };

				std::vector<Interval> intervals{};
				
				for (const auto& written_block : patch->getWrittenBlocks()) {
					if (written_block.snesoffset == 0x80FFDC && written_block.numbytes == 4) {
						// discard writes to checksum/inverse checksum
						continue;
					}

					const auto our_interval{ Interval(
							written_block.snesoffset,
							written_block.snesoffset + written_block.numbytes
					) };
					intervals.push_back(our_interval);

					for (const auto& [other_patch_name, intervals] : written_intervals) {
						if (patch_name == other_patch_name) {
							// don't check patch against itself if applied multiple times
							continue;
						}

						for (const auto& other_interval : intervals) {
							if (our_interval.overlaps(other_interval)) {
								const auto overlap{ our_interval.overlap(other_interval) };
								const auto start{ fmt::format("{:06X}", overlap.lower) };

								const auto byte_or_bytes{ overlap.length() == 1 ? "byte" : "bytes" };

								spdlog::warn(fmt::format(
									"WARNING: Patches '{}' and '{}' both wrote {} {} to address {}",
									other_patch_name, patch_name, overlap.length(), byte_or_bytes, start
								));
							}
						}
					}
				}

				written_intervals.insert({ patch_name, intervals });
			}
		}
	}
}
