#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	class BinaryMap16 : public LunarMagicInsertable {
	protected:
		const fs::path map16_file_path;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		BinaryMap16(const Configuration& config);
	};
}
