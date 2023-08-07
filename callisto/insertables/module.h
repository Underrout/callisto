#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unordered_set>
#include <optional>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include "asar-dll-bindings/c/asardll.h"
#include <asar/warnings.h>
#include <boost/filesystem.hpp>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	class Module : public RomInsertable {
	protected:
		static constexpr auto PLACEHOLDER_LABEL{ "PLACEHOLDER " };
		static constexpr auto MAX_ROM_SIZE{ 16 * 1024 * 1024 };

		std::string patch_string{};

		std::unordered_set<size_t> our_module_addresses{};

		const fs::path input_path;
		const std::vector<fs::path> output_paths;

		const fs::path real_module_folder_location;
		const fs::path cleanup_folder_location;

		const fs::path project_relative_path;

		std::vector<const char*> additional_include_paths;

		const fs::path callisto_asm_file;
		std::unordered_set<int> other_module_addresses;
		const std::optional<fs::path> module_header_file;

		void emitOutputFiles() const;
		void emitOutputFile(const fs::path& output_path) const;
		void emitPlainAddressFile() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void fixAsarMemoryLeak() const;

		void recordOurAddresses();
		void verifyWrittenBlockCoverage() const;
		void verifyNonHijacking() const;

	public:
		static std::string modulePathToName(const fs::path& path);

		const std::unordered_set<size_t>& getModuleAddresses() const {
			return our_module_addresses;
		}

		const std::vector<fs::path>& getOutputPaths() const {
			return output_paths;
		};

		Module(const Configuration& config,
			const fs::path& input_path,
			const fs::path& callisto_asm_file,
			const std::unordered_set<int>& other_module_addresses,
			const std::vector<fs::path>& additional_include_paths = {});

		void init() override;
		void insert() override;
	};
}
