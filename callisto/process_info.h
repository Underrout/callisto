#pragma once

#include <optional>
#include <filesystem>

namespace fs = std::filesystem;

#include <boost/process.hpp>

namespace bp = boost::process;

#ifdef _WIN32
#include <Windows.h>
#endif

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/process.hpp>

namespace bp = boost::process;
namespace bi = boost::interprocess;

#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace callisto {
	class ProcessInfo {
	public:
		struct SharedMemory {
			SharedMemory() : hwnd_set(false), verification_code_set(false), current_rom_set(false), save_process_pid_set(false) {}

			bi::interprocess_mutex mutex;
			bool hwnd_set, verification_code_set, current_rom_set, save_process_pid_set;
			bp::pid_t save_process_pid;
			uint32_t hwnd;
			uint16_t verification_code;
			char current_rom[MAX_PATH];
			char project_rom[MAX_PATH];
		};
	protected:
		std::string shared_memory_name;
		SharedMemory* shared_memory;
		bi::shared_memory_object shared_memory_object;
		bi::mapped_region mapped_region;

		static constexpr auto SHARED_MEMORY_NAME{ "callisto_discourse_{}" };

		static std::string determineSharedMemoryName(const bp::pid_t lunar_magic_pid);
		void setUpSharedMemory(bool open_only);

	public:
		ProcessInfo();
		ProcessInfo(const bp::pid_t lunar_magic_pid, bool open_only = false);
		ProcessInfo(ProcessInfo&& other) noexcept;

		void setPid(const bp::pid_t lunar_magic_pid, bool open_only = false);

		ProcessInfo& operator=(ProcessInfo&& other) noexcept;

#ifdef _WIN32
		std::optional<HWND> getLunarMagicMessageWindowHandle();
		void setLunarMagicMessageWindowHandle(HWND hwnd);
#endif
		std::optional<uint16_t> getLunarMagicVerificationCode();
		void setLunarMagicVerificationCode(uint16_t verification_code);

		std::optional<fs::path> getCurrentLunarMagicRomPath();
		void setCurrentLunarMagicRomPath(const fs::path& rom_path);

		std::optional<bp::pid_t> getSaveProcessPid();
		void setSaveProcessPid(bp::pid_t pid);
		void unsetSaveProcessPid();

		fs::path getProjectRomPath();
		void setProjectRomPath(const fs::path& rom_path);

		const std::string& getSharedMemoryName() const;

		static bool sharedMemoryExistsFor(bp::pid_t pid);

		~ProcessInfo();
	};
}
