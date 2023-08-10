#pragma once

#include <optional>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <Windows.h>
#endif

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace bi = boost::interprocess;

#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace callisto {
	class ProcessInfo {
	public:
		struct SharedMemory {
			SharedMemory() : set(false), usertoolbar_needs_cleaning(true) {}

			bi::interprocess_mutex mutex;
			bool set;
			bool usertoolbar_needs_cleaning;
			uint32_t hwnd;
			uint16_t verification_code;
			char current_rom[MAX_PATH];
		};
	protected:
		std::string shared_memory_name;
		SharedMemory* shared_memory;
		bi::shared_memory_object shared_memory_object;
		bi::mapped_region mapped_region;

		static constexpr auto SHARED_MEMORY_NAME{ "callisto_discourse_{}" };

		static std::string determineSharedMemoryName();
		void setUpSharedMemory();

	public:
		ProcessInfo();
		ProcessInfo(ProcessInfo&& other) noexcept;

		ProcessInfo& operator=(ProcessInfo&& other) noexcept;

#ifdef _WIN32
		std::optional<HWND> getLunarMagicMessageWindowHandle();
#endif
		std::optional<uint16_t> getLunarMagicVerificationCode();
		std::optional<fs::path> getCurrentLunarMagicRomPath();

		const std::string& getSharedMemoryName() const;

		~ProcessInfo();
	};
}
