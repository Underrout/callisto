#pragma once

#include <filesystem>
#include <optional>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <conio.h>
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <termios.h>
#endif

#include <spdlog/spdlog.h>
#include <boost/process.hpp>
#include <fmt/chrono.h>

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

#include "../builders/rebuilder.h"
#include "../builders/quick_builder.h"
#include "../saver/saver.h"

#include "../path_util.h"

using namespace ftxui;

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class TUI {
	protected:
		ConfigurationManager config_manager;
		std::shared_ptr<Configuration> config;
		const fs::path& stardust_directory;

		ScreenInteractive screen;

		Component getConfigOnlyButton(const std::string& button_text, Closure button_func);
		Component getRomOnlyButton(const std::string& button_text, Closure button_func, bool is_save_button = false);

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
		void updateEmulatorMenu();
		Component emulator_menu_component;
		Component emulator_menu;

		std::string modal_title{};
		std::string modal_text{};
		bool show_modal{ false };
		Component getModal();
		void showModal(const std::string& new_title, const std::string& new_text);

		std::string choice_modal_title{};
		std::string choice_modal_text{};
		bool show_choice_modal{ false };
		std::function<void()> choice_yes_func;
		std::function<void()> choice_no_func;
		Component getChoiceModal();
		void showChoiceModal(const std::string& new_title, const std::string& new_text,
			std::function<void()> yes_function, std::function<void()> no_function);

		std::optional<std::string> getLastConfigName(const fs::path& stardust_directory) const;
		void setConfiguration(const std::optional<std::string>& profile_name, const fs::path& stardust_directory);
		void trySetConfiguration();

		std::optional<std::string> errorToText(std::function<void()> func);

		void runWithLogging(const std::string& function_name, std::function<void()> func);
		bool modalError(std::function<void()> func);
		void logError(std::function<void()> func);

		void waitForEscape();

#if defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
		// Credit for getch: https://stackoverflow.com/a/16361724
		char getch(void)
		{
			char buf = 0;
			struct termios old = { 0 };
			fflush(stdout);
			if (tcgetattr(0, &old) < 0)
				perror("tcsetattr()");
			old.c_lflag &= ~ICANON;
			old.c_lflag &= ~ECHO;
			old.c_cc[VMIN] = 1;
			old.c_cc[VTIME] = 0;
			if (tcsetattr(0, TCSANOW, &old) < 0)
				perror("tcsetattr ICANON");
			if (read(0, &buf, 1) < 0)
				perror("read()");
			old.c_lflag |= ICANON;
			old.c_lflag |= ECHO;
			if (tcsetattr(0, TCSADRAIN, &old) < 0)
				perror("tcsetattr ~ICANON");
			return buf;
		}
#endif

		void rebuildButton();
		void quickbuildButton();
		void packageButton();
		void saveButton();
		void emulatorButton(const std::string& emulator_name);
		void editButton();

		void markerSafeguard(const std::string& title, std::function<void()> func);

		Component wrapMenuInEventCatcher(Component full_menu);

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
