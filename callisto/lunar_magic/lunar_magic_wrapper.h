#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <utility>

#include <boost/process.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <tlhelp32.h>
#endif

#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace bp = boost::process;
namespace fs = std::filesystem;

#include "../process_info.h"

#include "../colors.h"

namespace callisto {
	class LunarMagicWrapper {
	protected:
		static constexpr auto LM_MESSAGE_ID{ 0xBECB };
		static constexpr auto LM_REQUEST_TYPE{ 2 };
		static constexpr auto LM_USERTOOLBAR_FILE{ "usertoolbar.txt" };
		static constexpr auto ELOPER_NAME{ "eloper.exe" };

		static constexpr auto CALLISTO_MAGIC_MARKER{ "; Callisto generated call below, if you can see this, you should remove it!" };

		static constexpr auto LM_USERBAR_TEXT{
			"{}\n"
			"***START***\n"
			"\"{}\" \"%9\" \"%1\" \"{}\" \"{}\"\n"
			"LM_DEFAULT\n"
			"LM_NO_BUTTON,LM_AUTORUN_ON_NEW_ROM,LM_NOTIFY_ON_NEW_ROM,"
			"LM_NOTIFY_ON_SAVE_LEVEL,LM_NOTIFY_ON_SAVE_MAP16,LM_NOTIFY_ON_SAVE_OV"
			",LM_NO_CONSOLE_WINDOW"
			"\n"
			"***END***\n"
		};

		using LunarMagicProcessVector = std::vector<std::pair<bp::child, ProcessInfo>>;

		LunarMagicProcessVector lunar_magic_processes{};


		static fs::path getUsertoolbarPath(const fs::path& lunar_magic_path);
		static bp::child launchLunarMagic(const fs::path& lunar_magic_path, const fs::path& rom_to_open);
		void launchInjectedLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open);

		void appendInjectionStringToUsertoolbar(const std::string& shared_memory_name, const fs::path& callisto_path, const fs::path& usertoolbar_path);

		void cleanClosedInstances();

		LunarMagicProcessVector::iterator getLunarMagicWithRomOpen(const fs::path& desired_rom);

		void openLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open);

	public:
		enum class Result {
			SUCCESS,
			FAILURE,
			NOT_PERFORMED,
			NO_INSTANCE
		};

		Result reloadRom(const fs::path& rom_to_reload);
		void bringToFrontOrOpen(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open);

		void setNewProjectRom(const fs::path& project_rom_path);

		void attemptReattach(const fs::path& lunar_magic_path);

		std::optional<bp::group::native_handle_t> pendingEloperSave();
	};
}
