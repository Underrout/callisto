#include "lunar_magic_wrapper.h"

namespace callisto {
#ifdef _WIN32
	std::pair<HWND, uint16_t> LunarMagicWrapper::extractHandleAndVerificationCode(const std::string& lm_string) {
		const auto colon{ lm_string.find(':') };

		const auto hwnd_part{ lm_string.substr(0, colon) };
		const auto hwnd{ reinterpret_cast<HWND>(std::stoi(hwnd_part, nullptr, 16)) };

		const auto verification_part{ lm_string.substr(colon + 1) };
		const auto verification_code{ static_cast<uint16_t>(std::stoi(verification_part, nullptr, 16))};

		return { hwnd, verification_code };
	}
#endif

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

	void LunarMagicWrapper::launchInjectedLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		const auto usertoolbar_path{ getUsertoolbarPath(lunar_magic_path) };

		ProcessInfo process_info{};

		appendInjectionStringToUsertoolbar(process_info.getSharedMemoryName(), callisto_path, usertoolbar_path);

		auto lunar_magic{ launchLunarMagic(lunar_magic_path, rom_to_open) };

		lunar_magic_processes.emplace_back(std::move(lunar_magic), std::move(process_info));
	}

	void LunarMagicWrapper::appendInjectionStringToUsertoolbar(const std::string& shared_memory_name, const fs::path& callisto_path, const fs::path& usertoolbar_path){
		const auto injection_string{ fmt::format(
			LM_USERBAR_TEXT, CALLISTO_MAGIC_MARKER, callisto_path.string(), shared_memory_name, usertoolbar_path.string()) };

		spdlog::debug("Adding injection string to Lunar Magic's usertoolbar.txt at {}:\n\r{}", usertoolbar_path.string(), injection_string);

		const auto file_exists{ fs::exists(usertoolbar_path) };
		std::ofstream stream{ usertoolbar_path, std::ios_base::out | std::ios_base::app };
		if (file_exists) {
			stream << '\n';
		}
		stream << injection_string;
	}

	void LunarMagicWrapper::restoreUsertoolbar(const fs::path& usertoolbar_path) {
		spdlog::debug("Attempting to clean usertoolbar.txt at {}", usertoolbar_path.string());
		std::ostringstream out{};
		std::ifstream usertoolbar_file{ usertoolbar_path };
		std::string line;
		bool first{ true };
		while (std::getline(usertoolbar_file, line)) {
			if (line == CALLISTO_MAGIC_MARKER) {
				spdlog::debug("Magic marker line found");
				break;
			}

			if (first) {
				first = false;
			}
			else {
				out << std::endl;
			}

			out << line;
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

	void LunarMagicWrapper::cleanClosedInstances() {
		auto it{ lunar_magic_processes.begin() };
		while (it != lunar_magic_processes.end()) {
			if (!it->first.running()) {
				it = lunar_magic_processes.erase(it);
			}
			else {
				++it;
			}
		}
	}

	LunarMagicWrapper::LunarMagicProcessVector::iterator LunarMagicWrapper::getLunarMagicWithRomOpen(const fs::path& desired_rom) {
		cleanClosedInstances();

		auto it{ lunar_magic_processes.begin() };
		while (it != lunar_magic_processes.end()) {
			const auto open_rom{ it->second.getCurrentLunarMagicRomPath() };
			if (open_rom.has_value() && open_rom.value() == desired_rom) {
				return it;
			}
			++it;
		}

		return lunar_magic_processes.end();
	}

	void LunarMagicWrapper::openLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		launchInjectedLunarMagic(callisto_path, lunar_magic_path, rom_to_open);
	}

	LunarMagicWrapper::Result LunarMagicWrapper::reloadRom(const fs::path& rom_to_reload) {
		spdlog::debug("Attempting to reload ROM {}", rom_to_reload.string());

#ifdef _WIN32
		cleanClosedInstances();

		if (lunar_magic_processes.empty()) {
			spdlog::debug("Not reloading as no Lunar Magic instances running currently");
			return Result::NO_INSTANCE;
		}

		bool reloaded_at_least_one{ false };
		for (auto& [lm_process, process_info] : lunar_magic_processes) {
			const auto current_rom{ process_info.getCurrentLunarMagicRomPath() };
			if (!current_rom.has_value()) {
				spdlog::debug("Unable to get current ROM of Lunar Magic process, skipping it");
				continue;
			}

			if (rom_to_reload != current_rom.value()) {
				spdlog::debug("Current ROM {} does not match ROM to reload {}, not reloading", current_rom.value().string(), rom_to_reload.string());
				continue;
			}

			const auto lm_message_window{ process_info.getLunarMagicMessageWindowHandle() };
			if (!lm_message_window.has_value()) {
				spdlog::debug("Unable to get Lunar Magic message window handle for this process, skipping it");
				continue;
			}

			const auto verification_code{ process_info.getLunarMagicVerificationCode() };
			if (!verification_code.has_value()) {
				spdlog::debug("Unable to get Lunar Magic verification code for this process, skipping it");
				continue;
			}

			const auto result{ SendMessage(lm_message_window.value(), LM_MESSAGE_ID, 0, (verification_code.value() << 16) | LM_REQUEST_TYPE) };
			reloaded_at_least_one = true;
		}

		if (reloaded_at_least_one) {
			spdlog::info(fmt::format(colors::build::REMARK, "Successfully reloaded ROM in Lunar Magic for you ^-^"));
			return Result::SUCCESS;
		}
		else {
			return Result::NOT_PERFORMED;
		}

		// LM seems to say nothing about the return value of the message, so I'm just ignoring it I guess
#else
		spdlog::info("Automatic ROM reloading is not supported on operating systems other than Windows, "
			"please reload your ROM in Lunar Magic manually if you have it open!");
#endif
		return Result::SUCCESS;
	}

	void LunarMagicWrapper::bringToFrontOrOpen(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		spdlog::debug("Attempting to bring Lunar Magic window to front");

#ifdef _WIN32
		auto existing_instance{ getLunarMagicWithRomOpen(rom_to_open) };

		if (existing_instance == lunar_magic_processes.end()) {
			openLunarMagic(callisto_path, lunar_magic_path, rom_to_open);
			return;
		}

		const auto potential_lm_window_handle{ existing_instance->second.getLunarMagicMessageWindowHandle() };
		if (!potential_lm_window_handle.has_value()) {
			spdlog::debug("Failed to get window handle of Lunar Magic message window, cannot bring window to front");
			return;
		}

		const auto lm_main_window{ GetParent(potential_lm_window_handle.value()) };

		if (IsIconic(lm_main_window)) {
			ShowWindow(lm_main_window, SW_RESTORE);
		}
		SetForegroundWindow(lm_main_window);

		spdlog::debug("Successfully brought main Lunar Magic window to front!");
#else
		auto existing_instance{ getLunarMagicWithRomOpen(rom_to_open) };

		if (existing_instance == lunar_magic_processes.end()) {
			openLunarMagic(callisto_path, lunar_magic_path, rom_to_open);
		}
		else {
			spdlog::info("Bringing Lunar Magic window to top is only supported on Windows, please manually locate your Lunar Magic window!");
		}
#endif
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
		auto shared_memory{ static_cast<ProcessInfo::SharedMemory*>(addr) };

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
