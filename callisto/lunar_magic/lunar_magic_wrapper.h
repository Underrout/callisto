#pragma once

#ifdef _WIN32

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

#include <boost/process.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <Windows.h>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace bi = boost::interprocess;
namespace bp = boost::process;
namespace fs = std::filesystem;

namespace callisto {
	class LunarMagicWrapper {
	protected:
		struct SharedMemory {
			SharedMemory() : set(false), usertoolbar_needs_cleaning(true), hwnd(0), verification_code(0) {}

			bi::interprocess_mutex mutex;
			bool set;
			bool usertoolbar_needs_cleaning;
			uint32_t hwnd;
			uint16_t verification_code;
			char current_rom[MAX_PATH];
		};

		static constexpr auto LM_MESSAGE_ID{ 0xBECB };
		static constexpr auto LM_REQUEST_TYPE{ 2 };
		static constexpr auto LM_USERTOOLBAR_FILE{ "usertoolbar.txt" };

		static constexpr auto SHARED_MEMORY_NAME{ "callisto_discourse_{}" };

		static constexpr auto CALLISTO_MAGIC_MARKER{ "; Callisto generated call below, if you can see this, you should remove it!" };

		static constexpr auto LM_USERBAR_TEXT{
			"{}\n"
			"***START***\n"
			"\"{}\" \"magic\" \"%9\" \"%1\" \"{}\" \"{}\"\n"
			"LM_DEFAULT\n"
			"LM_NO_BUTTON,LM_AUTORUN_ON_NEW_ROM,LM_NO_CONSOLE_WINDOW\n"
			"***END***\n"
		};

		std::optional<bp::child> lunar_magic_process{};

		std::optional<uint16_t> verification_code{};
		std::optional<HWND> lm_message_window{};
		fs::path current_rom{};

		std::string shared_memory_name;

		bi::shared_memory_object shared_memory_object;
		bi::mapped_region mapped_region;
		SharedMemory* shared_memory;

		static fs::path getUsertoolbarPath(const fs::path& lunar_magic_path);
		static bp::child launchLunarMagic(const fs::path& lunar_magic_path, const fs::path& rom_to_open);
		bp::child launchInjectedLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open);
		static std::pair<HWND, uint16_t> extractHandleAndVerificationCode(const std::string& lm_string);
		static std::string determineSharedMemoryName();

		void appendInjectionStringToUsertoolbar(const fs::path& callisto_path, const fs::path& usertoolbar_path);
		static void restoreUsertoolbar(const fs::path& usertoolbar_path);

		bool tryPopulateFromSharedMemory();

		void setUpSharedMemory();
		void tearDownSharedMemory();
		bool isRunning();

	public:
		enum class Result {
			SUCCESS,
			FAILURE,
			NOT_PERFORMED,
			NO_INSTANCE
		};

		LunarMagicWrapper();

		void openLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open);

		Result reloadRom(const fs::path& rom_to_reload);
		Result bringToFront();

		static void communicate(const fs::path& current_rom, 
			const std::string& lm_verification_string, const std::string& shared_memory_name, const fs::path& usertoolbar_path);

		~LunarMagicWrapper() {
			tearDownSharedMemory();
		};
	};
}

#endif
