#pragma once

#include <filesystem>
#include <optional>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include <spdlog/spdlog.h>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include "../emulators/emulators.h"
#include "../configuration/configuration_manager.h"

#include "../path_util.h"

using namespace ftxui;

namespace fs = std::filesystem;

namespace stardust {
	class TUI {
	protected:
		ConfigurationManager config_manager;
		std::shared_ptr<Configuration> config;
		const fs::path& stardust_directory;

		ScreenInteractive screen;

		Component getConfigOnlyButton(const std::string& button_text, Closure button_func);
		Component getRomOnlyButton(const std::string& button_text, Closure button_func);

		int selected_main_menu_entry{ 0 };
		void setMainMenu();
		Component main_menu_component;
		Component main_menu;

		void determineInitialProfile();
		std::vector<std::string> profile_names{};
		int selected_profile_menu_entry{ 0 };
		int previously_selected_profile_menu_entry{ 0 };
		void setProfileMenu();
		Component profile_menu_component;
		Component profile_menu;

		Emulators emulators;
		std::vector<std::string> emulator_names{};
		int selected_emulator_menu_entry{ 0 };
		void setEmulatorMenu();
		Component emulator_menu_component;
		Component emulator_menu;

		std::string modal_title{};
		std::string modal_text{};
		bool show_modal{ false };
		Component getModal();
		void showModal(const std::string& new_title, const std::string& new_text);

		std::optional<std::string> getLastConfigName(const fs::path& stardust_directory) const;
		void setProfile(const std::optional<std::string>& profile_name, const fs::path& stardust_directory);
		void trySetSelectedProfile();

		bool invokeCatchError(std::function<void()> func);

		static Elements split(std::string str) {
			Elements output;
			std::stringstream ss(str);
			std::string word;
			while (std::getline(ss, word, '\n')) {
				output.push_back(text(word));
			}
			return output;
		}

		static Element nonWrappingParagraph(std::string str) {
			Elements lines{};
			for (auto& line : split(std::move(str))) {
				lines.push_back(line);
			}
			return vbox(std::move(lines));
		}

	public:
		void run();

		TUI(const fs::path& stardust_directory);
	};
}
