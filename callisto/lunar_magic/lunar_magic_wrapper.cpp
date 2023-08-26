#include "lunar_magic_wrapper.h"

namespace callisto {
	void LunarMagicWrapper::attemptReattach(const fs::path& lunar_magic_path) {
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };

		if (Process32First(snapshot, &entry) == TRUE) {
			while (Process32Next(snapshot, &entry) == TRUE) {
				HANDLE module_snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, entry.th32ProcessID) };

				MODULEENTRY32 module_entry;
				module_entry.dwSize = sizeof(MODULEENTRY32);
				if (Module32First(module_snapshot, &module_entry)) {
					if (stricmp(module_entry.szExePath, lunar_magic_path.string().data()) == 0) {
						if (ProcessInfo::sharedMemoryExistsFor(entry.th32ProcessID)) {
							lunar_magic_processes.emplace_back(bp::child(entry.th32ProcessID), ProcessInfo(entry.th32ProcessID, true));
						}
					}
				}

				CloseHandle(module_snapshot);
			}
		}

		CloseHandle(snapshot);
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

	void LunarMagicWrapper::launchInjectedLunarMagic(const fs::path& callisto_path, const fs::path& lunar_magic_path, const fs::path& rom_to_open) {
		const auto usertoolbar_path{ getUsertoolbarPath(lunar_magic_path) };

		ProcessInfo process_info{};

		appendInjectionStringToUsertoolbar(process_info.getSharedMemoryName(), callisto_path, usertoolbar_path);

		auto lunar_magic{ launchLunarMagic(lunar_magic_path, rom_to_open) };

		process_info.setPid(lunar_magic.id());

		lunar_magic_processes.emplace_back(std::move(lunar_magic), std::move(process_info));
	}

	void LunarMagicWrapper::appendInjectionStringToUsertoolbar(const std::string& shared_memory_name, const fs::path& callisto_path, const fs::path& usertoolbar_path){
		const auto injection_string{ fmt::format(
			LM_USERBAR_TEXT, CALLISTO_MAGIC_MARKER, (callisto_path.parent_path() / ELOPER_NAME).string(), 
			usertoolbar_path.string(), callisto_path.string())};

		spdlog::debug("Adding injection string to Lunar Magic's usertoolbar.txt at {}:\n\r{}", usertoolbar_path.string(), injection_string);

		const auto file_exists{ fs::exists(usertoolbar_path) };
		std::ofstream stream{ usertoolbar_path, std::ios_base::out | std::ios_base::app };
		if (file_exists) {
			stream << '\n';
		}
		stream << injection_string;
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

	std::optional<bp::group::native_handle_t> LunarMagicWrapper::pendingEloperSave() {
		for (auto& [_, proc_info] : lunar_magic_processes) {
			const auto potential_pid{ proc_info.getSaveProcessPid() };
			if (potential_pid.has_value()) {
				return potential_pid.value();
			}
		}
		return {};
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
			spdlog::info(fmt::format(colors::RESOURCE, "Successfully reloaded ROM in Lunar Magic for you (/o.o)-~*.^"));
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
}
