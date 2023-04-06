#include "tui.h"

namespace stardust {
	TUI::TUI(const fs::path& stardust_directory) : config_manager(stardust_directory), stardust_directory(stardust_directory),
		screen(ScreenInteractive::Fullscreen())
	{
		screen.SetCursor(ftxui::Screen::Cursor{ .shape = ftxui::Screen::Cursor::Shape::Hidden });

		spdlog::set_level(spdlog::level::off);
		spdlog::set_pattern("[%^%l%$] %v");
	}

	Component TUI::getConfigOnlyButton(const std::string& button_text, Closure button_func) {
		auto option{ ButtonOption::Ascii() };
		option.transform = [&](const EntryState& e) {
			auto label{ e.label };
			if (e.focused) {
				label = "[" + label + "]";
			}
			else {
				label = " " + label + " ";
			}
			if (config == nullptr) {
				label += " (invalid config)";
			}
			auto element{ text(label) };
			if (config == nullptr) {
				element |= color(Color::Red);
			}
			return element;
		};
		return Button(button_text, button_func, option);
	}

	Component TUI::getRomOnlyButton(const std::string& button_text, Closure button_func) {
		auto option{ ButtonOption::Ascii() };
		option.transform = [=](const EntryState& e) {
			bool project_rom_set{ config != nullptr && config->project_rom.isSet() };
			bool project_rom_exists{ false };
			if (project_rom_set) {
				project_rom_exists = fs::exists(config->project_rom.getOrThrow());
			}

			auto label{ e.label };
			if (e.focused) {
				label = "[" + label + "]";
			}
			else {
				label = " " + label + " ";
			}
			if (config == nullptr) {
				label += " (invalid config)";
			}
			else if (!project_rom_set) {
				label += " (ROM path not set)";
			}
			else if (!project_rom_exists) {
				label += " (ROM not found)";
			}
			auto element{ text(label) };
			if (config == nullptr || !project_rom_set) {
				element |= color(Color::Red);
			}
			else if (!project_rom_exists) {
				element |= color(Color::GrayDark);
			}
			return element;
		};
		return Button(button_text, button_func, option);
	}

	void TUI::setMainMenu() {
		main_menu_component = Container::Vertical({
			getConfigOnlyButton("Rebuild", [&] { showModal("Title", "haha, rebuild"); }),
			getConfigOnlyButton("Quick Build", [&] { showModal("Error", "Can't do that yet"); }),
			getConfigOnlyButton("Package", [&] { showModal("Package", "The package"); }),

			Renderer([] { return separator(); }),

			getRomOnlyButton("Save ROM (S)", [=] { saveButton(); }),
			getRomOnlyButton("Edit ROM (E)", [=] { editButton(); }),

			Renderer([] { return separator(); }),

			Button("Reload configuration", [&] { trySetConfiguration(); }, ButtonOption::Ascii()),
			Button("Reload profiles", [&] {
				profile_names = config_manager.getProfileNames();
				config = nullptr;
				determineInitialProfile();
				updateEmulatorMenu();
			}, ButtonOption::Ascii()),

			Renderer([] { return separator(); }),

			Button("Exit (ESC)", [] { exit(0); }, ButtonOption::Ascii())
		}, &selected_main_menu_entry);

		const auto window_title{ fmt::format("stardust {}.{}.{}", STARDUST_VERSION_MAJOR, STARDUST_VERSION_MINOR, STARDUST_VERSION_PATCH) };
		main_menu = Renderer(main_menu_component, [=] { return window(text(window_title), main_menu_component->Render()); });
	}

	void TUI::setProfileMenu() {
		RadioboxOption option{};
		option.on_change = [&] {
			if (selected_profile_menu_entry != previously_selected_profile_menu_entry) {
				trySetConfiguration();
			}
		};
		profile_menu_component = Radiobox(&profile_names, &selected_profile_menu_entry, option);
		const auto windowed{ Renderer(profile_menu_component, [=] { return window(text("Profile"), profile_menu_component->Render()); }) };
		profile_menu = Maybe(windowed, [&] { return !profile_names.empty(); });
	}

	void TUI::setEmulatorMenu() {
		emulator_menu_component = Container::Vertical({}, &selected_emulator_menu_entry);
		updateEmulatorMenu();
		const auto windowed{ Renderer(emulator_menu_component, [=] { return window(text("Emulators"), emulator_menu_component->Render()); }) };
		emulator_menu = Maybe(windowed, [&] { return !emulator_names.empty(); });
	}

	void TUI::updateEmulatorMenu() {
		emulator_menu_component->DetachAllChildren();
		if (config != nullptr) {
			emulator_names = emulators.getEmulatorNames();
			int i{ 1 };
			for (const auto& emulator_name : emulator_names) {
				emulator_menu_component->Add(getRomOnlyButton(emulator_name + fmt::format(" ({})", i++), [=] {
					emulatorButton(emulator_name);
				}));
			}
		}
	}

	void TUI::showModal(const std::string& new_title, const std::string& new_text) {
		show_modal = true;
		modal_title = new_title;
		modal_text = new_text;
	}

	void TUI::determineInitialProfile() {
		const auto last_config_name{ getLastConfigName(stardust_directory) };
		bool used_cached{ false };
		if (last_config_name.has_value()) {
			const auto found{ std::find(profile_names.begin(), profile_names.end(), last_config_name.value()) };
			if (found != profile_names.end()) {
				selected_profile_menu_entry = std::distance(profile_names.begin(), found);
				modalError([&] { setConfiguration(*found, stardust_directory); });
				used_cached = true;
			}
		}

		if (!used_cached) {
			if (!profile_names.empty()) {
				selected_profile_menu_entry = 0;
				modalError([&] { setConfiguration(profile_names.at(0), stardust_directory); });
			}
			else {
				modalError([&] { setConfiguration({}, stardust_directory); });
			}
		}
		previously_selected_profile_menu_entry = selected_profile_menu_entry;
	}

	void TUI::run() {
		profile_names = config_manager.getProfileNames();
		determineInitialProfile();

		if (config != nullptr) {
			emulators = Emulators(*config);
			emulator_names = emulators.getEmulatorNames();
		}

		setMainMenu();
		setEmulatorMenu();
		setProfileMenu();

		Component full_menu{ Container::Horizontal({
			main_menu | size(HEIGHT, EQUAL, 11) | size(WIDTH, EQUAL, 40),
			Container::Vertical({
				profile_menu, emulator_menu
			})
		}) };

		full_menu |= Modal(getModal(), &show_modal);

		const auto wrapped{ wrapMenuInEventCatcher(full_menu) };

		screen.Loop(wrapped);
	}

	void TUI::setConfiguration(const std::optional<std::string>& profile_name, const fs::path& stardust_directory) {
		if (profile_name.has_value()) {
			fs::create_directories(stardust_directory / ".cache");

			std::ofstream last_profile_file{ stardust_directory / ".cache" / "last_profile.txt" };
			last_profile_file << profile_name.value();
			last_profile_file.close();
		}
		try {
			auto new_config{ config_manager.getConfiguration(profile_name) };
			config = new_config;
		}
		catch (const std::exception&) {
			config = nullptr;
			throw;
		}
	}

	void TUI::trySetConfiguration() {
		const bool error_occured{ modalError([&] {
			if (profile_names.empty()) {
				setConfiguration({}, stardust_directory);
			}
			else {
				setConfiguration(profile_names.at(selected_profile_menu_entry), stardust_directory);
			}
		}) };
		if (!error_occured) {
			emulators = Emulators(*config);
			emulator_names = emulators.getEmulatorNames();
		}
		else {
			config = nullptr;
			emulator_names.clear();
		}
		updateEmulatorMenu();
		previously_selected_profile_menu_entry = selected_profile_menu_entry;
	}

	std::optional<std::string> TUI::errorToText(std::function<void()> func) {
		try {
			func();
			return {};
		}
		catch (const TomlException2& e) {
			return toml::format_error(e.what(), e.toml_value, e.comment, e.toml_value2, e.comment2, e.hints);
		}
		catch (const TomlException& e) {
			return toml::format_error(e.what(), e.toml_value, e.comment, e.hints);
		}
		catch (const std::exception& e) {
			return e.what();
		}
	}

	void TUI::runWithLogging(std::function<void()> func) {
		spdlog::set_level(spdlog::level::info);
		screen.WithRestoredIO([=] {
			logError(func);
			std::cout << "Press ESC to return to stardust" << std::endl;
#ifdef _WIN32
			while (_getch() != 27) {}
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
			while (getch() != 27) {}
#endif
			clearConsole();
		})();
		spdlog::set_level(spdlog::level::off);
	}

	bool TUI::modalError(std::function<void()> func) {
		const auto maybe_error{ errorToText(func) };
		if (maybe_error.has_value()) {
			show_modal = true;
			modal_title = "Error";
			modal_text = maybe_error.value();
		}
		return maybe_error.has_value();
	}

	void TUI::logError(std::function<void()> func) {
		const auto maybe_error{ errorToText(func) };
		if (maybe_error.has_value()) {
			spdlog::error(maybe_error.value());
		}
	}

	void TUI::clearConsole() {
#ifdef _WIN32
		system("cls");
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
		system("clear");
#endif
	}

	void TUI::saveButton() {
		if (config == nullptr) {
			showModal("Error", "Cannot save ROM, current configuration is not valid");
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			showModal("Error", fmt::format(
				"No ROM found at\n    {}\n\nCannot save ROM",
				config->project_rom.getOrThrow().string())
			);
			return;
		}

		runWithLogging( [=] { Saver::exportResources(config->project_rom.getOrThrow(), *config, true); });
	}

	void TUI::editButton() {
		if (config == nullptr) {
			showModal("Error", "Cannot edit ROM, current configuration is not valid");
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			showModal("Error", fmt::format(
				"No ROM found at\n    {}\n\nCannot edit ROM",
				config->project_rom.getOrThrow().string())
			);
			return;
		}

		if (!config->lunar_magic_path.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files", config->lunar_magic_path.name));
			return;
		}

		if (!fs::exists(config->lunar_magic_path.getOrThrow())) {
			showModal("Error", fmt::format(
				"Lunar Magic not found at\n    {}\n\nCannot edit ROM",
				config->lunar_magic_path.getOrThrow().string())
			);
			return;
		}

		try {
			bp::spawn(fmt::format(
				"\"{}\" \"{}\"",
				config->lunar_magic_path.getOrThrow().string(), config->project_rom.getOrThrow().string()
			));
		}
		catch (const std::exception& e) {
			showModal("Error", fmt::format("Failed to launch Lunar Magic with exception:\n{}", e.what()));
		}
	}

	Component TUI::wrapMenuInEventCatcher(Component full_menu) {
		return CatchEvent(full_menu, [&](Event event) {
			if (event == Event::Escape) {
				exit(0);
				return true;
			}
			else if (event == Event::Character('s')) {
				saveButton();
				return true;
			}
			else if (event == Event::Character('e')) {
				editButton();
				return true;
			}

			char i{ 0 };
			for (const auto& emulator : emulator_names) {
				if (i == 10) {
					break;
				}

				char button_index_char{ '0' + i + 1 };
				if (event == Event::Character(button_index_char)) {
					emulatorButton(emulator_names.at(i));
				}
				++i;
			}

			return false;
		});
	}

	void TUI::emulatorButton(const std::string& emulator_to_launch) {
		if (config == nullptr) {
			showModal("Error", fmt::format("Cannot launch {}, current configuration is not valid", emulator_to_launch));
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			showModal("Error", fmt::format(
				"No ROM found at\n    {}\nCannot save",
				config->project_rom.getOrThrow().string())
			);
			return;
		}

		try {
			emulators.launch(emulator_to_launch, config->project_rom.getOrThrow());
		}
		catch (const std::exception& e) {
			showModal("Error", fmt::format("Failed to launch {} with exception:\n{}", emulator_to_launch, e.what()));
		}
	}

	Component TUI::getModal() {
		auto component = Container::Vertical({
			Button("OK", [&] { show_modal = false; }, ButtonOption::Ascii()) });

		component |= Renderer([&](Element inner) {
			return window(text(modal_title), vbox({
						nonWrappingParagraph(modal_text),
						separator(),
						inner | center,
				}))
				| size(WIDTH, GREATER_THAN, 30);
			});
		return component;
	}

	std::optional<std::string> TUI::getLastConfigName(const fs::path& stardust_directory) const {
		const auto path{ stardust_directory / ".cache" / "last_profile.txt" };
		if (fs::exists(path)) {
			std::ifstream last_profile_file{ path };
			std::string name;
			std::getline(last_profile_file, name);
			last_profile_file.close();
			return name;
		}
		else {
			return {};
		}
	}
}
