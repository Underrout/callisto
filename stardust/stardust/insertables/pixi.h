#pragma once

#include <filesystem>

#include <boost/program_options/parsers.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <pixi_api.h>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../dependency/dependency_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class Pixi : public RomInsertable {
	public:
		struct ExtraByteInfo {
			fs::path config_file_path;
			int byte_count;
			int extra_byte_count;

			ExtraByteInfo(const fs::path& config_file_path, int byte_count, int extra_byte_count)
				: config_file_path(config_file_path), byte_count(byte_count), extra_byte_count(extra_byte_count) {}
		};

	protected:
		const fs::path pixi_folder_path;
		const std::string pixi_options;
		const std::vector<fs::path> static_dependencies;
		const std::optional<fs::path> dependency_report_file_path;
		std::vector<ExtraByteInfo> extra_bytes;
		
		std::unordered_set<Dependency> determineDependencies() override;

	public:
		Pixi(const fs::path& pixi_folder_path, const fs::path& temporary_rom_path, const std::string& pixi_options,
			const std::vector<fs::path>& static_dependencies = {}, 
			const std::optional<fs::path>& dependency_report_file_path = {});

		void insert() override;

		const std::vector<ExtraByteInfo>& getExtraByteInfo() const;
	};
}
