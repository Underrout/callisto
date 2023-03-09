#pragma once

#include <filesystem>

#include <boost/process/system.hpp>
#include <boost/process.hpp>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "../insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class ExternalTool : public Insertable {
	protected:
		const std::string tool_name;
		const fs::path tool_exe_path;
		const std::string tool_options;
		const fs::path working_directory;
		bool take_user_input;

	public:
		ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options,
			const fs::path& working_directory, bool take_user_input=false);
		ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options);

		void insert() override;
	};
}
