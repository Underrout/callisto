#ifdef _WIN32

#include "lunar_magic_wrapper.h"

namespace callisto {
	std::pair<HWND, uint16_t> LunarMagicWrapper::extractHandleAndVerificationCode(const std::string& lm_string) {
		const auto colon{ lm_string.find(':') };

		const auto hwnd_part{ lm_string.substr(0, colon) };
		const auto hwnd{ reinterpret_cast<HWND>(std::stoi(hwnd_part, nullptr, 16)) };

		const auto verification_part{ lm_string.substr(colon + 1) };
		const auto verification_code{ static_cast<uint16_t>(std::stoi(verification_part, nullptr, 16))};

		return { hwnd, verification_code };
	}

	void LunarMagicWrapper::setUpSharedMemory() {
		shared_memory_name = determineSharedMemoryName();

		shared_memory_object = bi::shared_memory_object(
			bi::create_only,
			shared_memory_name.data(),
			bi::read_write
		);

		shared_memory_object.truncate(sizeof(SharedMemory));

		mapped_region = bi::mapped_region(shared_memory_object, bi::read_write);

		void* addr = mapped_region.get_address();

		shared_memory = new (addr) SharedMemory;
	}

	fs::path LunarMagicWrapper::getUsertoolbarPath(const fs::path& lunar_magic_path) {
		return lunar_magic_path.parent_path() / LM_USERTOOLBAR_FILE;
	}

	bp::child LunarMagicWrapper::launchLunarMagic(const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		auto process{ bp::child(fmt::format(
			"\"{}\" \"{}\"",
			lunar_magic_path.string(), rom_to_open.string()
		)) };
		process.detach();
		return process;
	}

	bp::child LunarMagicWrapper::launchInjectedLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		const auto usertoolbar_path{ getUsertoolbarPath(lunar_magic_path) };
		appendInjectionStringToUsertoolbar(callisto_path, usertoolbar_path);

		auto lunar_magic{ launchLunarMagic(lunar_magic_path, rom_to_open) };

		return lunar_magic;
	}

	std::string LunarMagicWrapper::determineSharedMemoryName() {
		return fmt::format(SHARED_MEMORY_NAME, std::chrono::high_resolution_clock::now().time_since_epoch().count());
	}

	void LunarMagicWrapper::appendInjectionStringToUsertoolbar(const fs::path& callisto_path, const fs::path& usertoolbar_path){
		const auto injection_string{ fmt::format(
			LM_USERBAR_TEXT, CALLISTO_MAGIC_MARKER, callisto_path.string(), shared_memory_name, usertoolbar_path.string()) };

		spdlog::debug("Adding injection string to Lunar Magic's usertoolbar.txt at {}:\n\r{}", usertoolbar_path.string(), injection_string);

		std::ofstream stream{ usertoolbar_path, std::ios_base::out | std::ios_base::app };
		if (fs::exists(usertoolbar_path)) {
			stream << '\n';
		}
		stream << injection_string;
	}

	void LunarMagicWrapper::restoreUsertoolbar(const fs::path& usertoolbar_path) {
		spdlog::debug("Attempting to clean usertoolbar.txt at {}", usertoolbar_path.string());
		std::ostringstream out{};
		std::ifstream usertoolbar_file{ usertoolbar_path };
		std::string line;
		while (std::getline(usertoolbar_file, line)) {
			if (line == CALLISTO_MAGIC_MARKER) {
				spdlog::debug("Magic marker line found");
				break;
			}

			out << line << std::endl;
		}
		usertoolbar_file.close();

		std::string out_string{ out.str() };

		if (out_string.empty()) {
			spdlog::debug("Cleaned usertoolbar file would be empty, deleting it instead");
			fs::remove(usertoolbar_path);
		}
		else {
			spdlog::debug("Writing back user's previous usertoolbar file");
			std::ofstream outfile{ usertoolbar_path };
			outfile << out_string;
		}
	}

	bool LunarMagicWrapper::tryPopulateFromSharedMemory() {
		spdlog::debug("Attempting to populate Lunar Magic info from shared memory");
		
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->set) {
			spdlog::debug("No shared memory set yet");
			return false;
		}

		verification_code = shared_memory->verification_code;
		lm_message_window = (HWND)shared_memory->hwnd;
		current_rom = shared_memory->current_rom;

		spdlog::debug(
			"Shared memory was set, received:\n\r"
			"verification_code = {:X}\n\r"
			"lm_hwnd = {:X}\n\r"
			"current_rom = {}",
			verification_code.value(),
			(uint32_t)lm_message_window.value(),
			current_rom.string()
		);

		return true;
	}

	void LunarMagicWrapper::tearDownSharedMemory() {
		bi::shared_memory_object::remove(shared_memory_name.data());
	}

	bool LunarMagicWrapper::isRunning() {
		return lunar_magic_process.has_value() && lunar_magic_process.value().running();
	}

	LunarMagicWrapper::LunarMagicWrapper() {
		setUpSharedMemory();
	}

	void LunarMagicWrapper::openLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);
		shared_memory->set = false;
		shared_memory->usertoolbar_needs_cleaning = true;

		lunar_magic_process = launchInjectedLunarMagic(callisto_path, lunar_magic_path, rom_to_open);
	}

	LunarMagicWrapper::Result LunarMagicWrapper::reloadRom(const fs::path& rom_to_reload) {
		spdlog::debug("Attempting to reload ROM {}", rom_to_reload.string());

		if (!isRunning()) {
			spdlog::debug("Not reloading as no Lunar Magic instance running currently");
			return Result::NO_INSTANCE;
		}

		if (!tryPopulateFromSharedMemory()) {
			return Result::FAILURE;
		}

		if (rom_to_reload != current_rom) {
			spdlog::debug("Current ROM {} does not match ROM to reload {}, not reloading", current_rom.string(), rom_to_reload.string());
			return Result::NOT_PERFORMED;
		}

		const auto result{ SendMessage(lm_message_window.value(), LM_MESSAGE_ID, 0, (verification_code.value() << 16) | LM_REQUEST_TYPE) };

		spdlog::info("Successfully reloaded ROM in Lunar Magic for you ^0^");

		// LM seems to say nothing about the return value of the message, so I'm just ignoring it I guess
	}

	LunarMagicWrapper::Result LunarMagicWrapper::bringToFront() {
		spdlog::debug("Attempting to bring Lunar Magic window to front");

		if (!isRunning()) {
			return Result::NO_INSTANCE;
		}

		// TODO implement this

		return Result::SUCCESS;
	}

	void LunarMagicWrapper::communicate(const fs::path& current_rom, 
		const std::string& lm_verification_string, const std::string& shared_memory_name, const fs::path& usertoolbar_path) {
		// this will throw if the shared memory does not exist, which I think is fine cause this is meant to be called by the Lunar Magic side

		spdlog::debug("Attempting to communicate Lunar Magic information back to main callisto instance through {}", shared_memory_name);
		auto shared_memory_object{ bi::shared_memory_object(
			bi::open_only,
			shared_memory_name.data(),
			bi::read_write
		) };

		bi::mapped_region mapped_region{ shared_memory_object, bi::read_write };

		void* addr{ mapped_region.get_address() };
		auto shared_memory{ static_cast<SharedMemory*>(addr) };

		spdlog::debug("Successfully set up writeable shared memory");

		const auto [handle, verification] = extractHandleAndVerificationCode(lm_verification_string);

		spdlog::debug(
			"Attempting to communicate Lunar Magic information:\n\r"
			"Verification code: {:X}\n\r"
			"LM Message window: {:X}\n\r"
			"Current ROM: {}",
			verification,
			(uint32_t)handle,
			current_rom.string()
		);

		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		shared_memory->set = true;
		std::strcpy(shared_memory->current_rom, current_rom.string().data());
		shared_memory->verification_code = verification;
		shared_memory->hwnd = (uint32_t)handle;

		if (shared_memory->usertoolbar_needs_cleaning) {
			spdlog::debug("Cleaning usertoolbar.txt");
			restoreUsertoolbar(usertoolbar_path);
			shared_memory->usertoolbar_needs_cleaning = false;
		}

		spdlog::debug("Successfully communicated with main callisto instance, exiting now!");
	}
}

#endif
