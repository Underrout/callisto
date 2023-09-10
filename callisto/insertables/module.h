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
		static constexpr auto RATS_TAG_TEXT{ "STAR" };
		static constexpr auto RATS_TAG_SIZE{ 8 };

		struct WrittenBlock {
			WrittenBlock(size_t start_pc, size_t start_snes, size_t size)
				: start_pc(start_pc), end_pc(start_pc + size), size(size), start_snes(start_snes), end_snes(start_snes + size) {};
			
			// this shit sucks, fuck the pc vs. snes shit
			size_t start_snes;
			size_t end_snes;
			size_t start_pc;
			size_t end_pc;
			size_t size;
		};
		using FreespaceArea = std::vector<WrittenBlock>;

		std::string patch_string{};

		const int id;

		std::shared_ptr<std::unordered_set<int>> current_module_addresses;
		std::unordered_set<int> our_module_addresses{};

		const fs::path input_path;
		const std::vector<fs::path> output_paths;

		const fs::path real_module_folder_location;
		const fs::path cleanup_folder_location;

		const fs::path project_relative_path;

		std::vector<fs::path> additional_include_paths;

		const fs::path callisto_asm_file;
		const std::optional<fs::path> module_header_file;

		void emitOutputFiles() const;
		void emitOutputFile(const fs::path& output_path) const;
		void emitPlainAddressFile() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void fixAsarMemoryLeak() const;

		void recordOurAddresses();
		void verifyWrittenBlockCoverage(const std::vector<char>& rom) const;
		void verifyNonHijacking() const;

		static std::optional<uint16_t> determineFreespaceBlockSize(size_t pc_address, const std::vector<char>& rom);
		static std::vector<WrittenBlock> convertToWrittenBlockVector(const writtenblockdata* const written_blocks, int block_count);
		static std::vector<FreespaceArea> convertToFreespaceAreas(const std::vector<WrittenBlock>& written_blocks, const std::vector<char>& rom);

	public:
		static std::string modulePathToName(const fs::path& path);

		const std::vector<fs::path>& getOutputPaths() const {
			return output_paths;
		};

		Module(const Configuration& config,
			const fs::path& input_path,
			const fs::path& callisto_asm_file,
			std::shared_ptr<std::unordered_set<int>> current_module_addresses,
			int id,
			const std::vector<fs::path>& additional_include_paths = {});

		void init() override;
		void insert() override;
	};
}
