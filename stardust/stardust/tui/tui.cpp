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

	Component TUI::getRomOnlyButton(const std::string& button_text, Closure button_func, bool is_save_button) {
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
				if (!is_save_button || !config->clean_rom.isSet() || !fs::exists(config->clean_rom.getOrThrow())) {
					label += " (ROM not found)";
				}
				else {
					label += " (from clean ROM)";
				}
			}
			auto element{ text(label) };
			if (config == nullptr || !project_rom_set) {
				element |= color(Color::Red);
			}
			else if (!project_rom_exists && !is_save_button) {
				element |= color(Color::GrayDark);
			}
			return element;
		};
		return Button(button_text, button_func, option);
	}

	void TUI::setMainMenu() {
		main_menu_component = Container::Vertical({
			getConfigOnlyButton("Rebuild ROM (R)", [=] { rebuildButton(); }),
			getConfigOnlyButton("Quickbuild ROM (Q)", [=] { quickbuildButton(); }),
			getRomOnlyButton("Package ROM (P)", [=] { packageButton(); }),

			Renderer([] { return separator(); }),

			getRomOnlyButton("Save ROM (S)", [=] { saveButton(); }, true),
			getRomOnlyButton("Edit ROM (E)", [=] { editButton(); }),

			Renderer([] { return separator(); }),

			Button("Reload configuration (C)", [&] { trySetConfiguration(); }, ButtonOption::Ascii()),
			Button("Reload profiles", [&] {
				profile_names = config_manager.getProfileNames();
				config = nullptr;
				determineInitialProfile();
				updateEmulatorMenu();
			}, ButtonOption::Ascii()),

			Renderer([] { return separator(); }),

			Button("View console output (V)", screen.WithRestoredIO([&] { 
				if (!anything_on_command_line) {
					std::cout << "Press ESC to return to stardust" << std::endl << std::endl << std::endl;
					anything_on_command_line = true;
				}
				waitForEscape(); 
			}), ButtonOption::Ascii()),

			Renderer([] { return separator(); }),

			Button("Exit (ESC)", [] { exit(0); }, ButtonOption::Ascii())
			}, &selected_main_menu_entry);

		const auto window_title{ fmt::format("stardust v{}.{}.{}a", STARDUST_VERSION_MAJOR, STARDUST_VERSION_MINOR, STARDUST_VERSION_PATCH) };
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

	void TUI::markerSafeguard(const std::string& title, std::function<void()> func) {
		if (config != nullptr && config->project_rom.isSet() && fs::exists(config->project_rom.getOrThrow())) {
			const auto needs_extraction{ Marker::getNeededExtractions(config->project_rom.getOrThrow(),
				Saver::getExtractableTypes(*config),
				config->use_text_map16_format.getOrDefault(false)) };
			if (!needs_extraction.empty()) {
				showChoiceModal("Prompt", fmt::format(
					"WARNING: Potentially unexported resources present in ROM\n    {}\n\n"
					"Building anyway could lead to loss of data. Run Save before the build?",
					config->project_rom.getOrThrow().string()
				), [=] {
					runWithLogging(title + " (with Save)", [=] {
						Saver::exportResources(config->project_rom.getOrThrow(), *config, true);
						func();
					});
					},
					[=] {
						showModal("Info", "Aborting build to prevent potential data loss");
				}
				);
				return;
			}
		}
		runWithLogging(title, func);
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
			main_menu | size(HEIGHT, EQUAL, 13) | size(WIDTH, EQUAL, 40),
			Container::Vertical({
				profile_menu, emulator_menu
			})
		}) };

		full_menu = wrapMenuInEventCatcher(full_menu);

		full_menu |= Modal(getModal(), &show_modal);
		full_menu |= Modal(getChoiceModal(), &show_choice_modal);

		screen.Loop(full_menu);
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

	void TUI::waitForEscape() {
#ifdef _WIN32
		while (_getch() != 27) {}
#elif defined(__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined(__APPLE__)
		while (getch() != 27) {}
#endif
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

	void TUI::runWithLogging(const std::string& function_name, std::function<void()> func) {
		spdlog::set_level(spdlog::level::info);
		screen.WithRestoredIO([=] {
			auto now{ std::chrono::system_clock::now() };
		spdlog::info(
			"--- {} at {} ---\n",
			function_name, now
		);
		logError(func);
		std::cout << "Press ESC to return to stardust" << std::endl;
		waitForEscape();
		anything_on_command_line = true;
		std::cout << std::endl << std::endl;
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

	void TUI::packageButton() {
		if (config == nullptr) {
			showModal("Error", "Current configuration is not valid\nCannot package ROM");
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files\nCannot package ROM", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			showModal("Error", fmt::format(
				"No ROM found at\n    {}\n\nCannot package ROM",
				config->project_rom.getOrThrow().string())
			);
			return;
		}

		if (!config->flips_path.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files\nCannot package ROM", config->flips_path.name));
		}

		if (!fs::exists(config->flips_path.getOrThrow())) {
			showModal("Error", fmt::format(
				"FLIPS not found at\n    {}\n\nCannot package ROM",
				config->flips_path.getOrThrow().string())
			);
			return;
		}

		if (!config->clean_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files\nCannot package ROM", config->clean_rom.name));
		}

		if (!fs::exists(config->clean_rom.getOrThrow())) {
			showModal("Error", fmt::format(
				"Clean ROM not found at\n    {}\n\nCannot package ROM",
				config->clean_rom.getOrThrow().string())
			);
			return;
		}

		const auto package_folder{ config->bps_package.getOrThrow().parent_path() };
		if (!fs::exists(package_folder)) {
			try {
				fs::create_directories(package_folder);
			}
			catch (const fs::filesystem_error& e) {
				showModal("Error", fmt::format("Failed to create folder at\n    {}\nwith exception:{}",
					package_folder.string(), e.what()
				));
			}
		}

		int exit_code;
		try {
			exit_code = bp::system(
				config->flips_path.getOrThrow().string(), "--create", "--bps-delta", config->clean_rom.getOrThrow().string(),
				config->project_rom.getOrThrow().string(), config->bps_package.getOrThrow().string(), bp::std_out > bp::null
			);
		}
		catch (const std::exception& e) {
			showModal("Error", fmt::format(
				"Failed to package ROM with exception:\n{}", e.what()
			));
		}

		if (exit_code == 0) {
			showModal("Success", fmt::format("Successfully created package of ROM at\n    {}", config->bps_package.getOrThrow().string()));
		}
		else {
			showModal("Error", fmt::format("FLIPS failed to create package of ROM at\n    {}", config->bps_package.getOrThrow().string()));
		}
	}

	void TUI::rebuildButton() {
		trySetConfiguration();

		if (config == nullptr) {
			showModal("Error", "Current configuration is not valid\nCannot build ROM");
			return;
		}

		markerSafeguard("Rebuild", [=] { Rebuilder rebuilder{}; rebuilder.build(*config); });
	}

	void TUI::quickbuildButton() {
		trySetConfiguration();

		if (config == nullptr) {
			showModal("Error", "Current configuration is not valid\nCannot quick build ROM");
			return;
		}

		markerSafeguard("Quickbuild", [=] {
			try {
				QuickBuilder quick_builder{ config->project_root.getOrThrow() };
				quick_builder.build(*config);
			}
			catch (const MustRebuildException& e) {
				spdlog::info("Quickbuild cannot continue due to the following reason, rebuilding ROM:\n{}", e.what());
				Rebuilder rebuilder{};
				rebuilder.build(*config);
			}
		});
	}

	void TUI::saveButton() {
		trySetConfiguration();

		if (config == nullptr) {
			showModal("Error", "Current configuration is not valid\nCannot save ROM");
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files\nCannot save ROM", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			if (!config->clean_rom.isSet() || !fs::exists(config->clean_rom.getOrThrow())) {
				showModal("Error", fmt::format(
					"No ROM found at\n    {}\n\nCannot save ROM",
					config->project_rom.getOrThrow().string())
				);
			}
			else {
				showChoiceModal("Prompt", fmt::format(
					"No ROM found at\n    {}\n\nSave from clean ROM instead?"
					"\n\n(WARNING: This may overwrite previously exported files with vanilla ones,\n"
					"only use this if you are starting a fresh project!)",
					config->project_rom.getOrThrow().string()),
					[=] { runWithLogging("Clean ROM Save", [=] {
						Saver::exportResources(config->clean_rom.getOrThrow(), *config, true, false); });
					},
					[] {}
					);
			}
			return;
		}

		runWithLogging("ROM Save", [=] { Saver::exportResources(config->project_rom.getOrThrow(), *config, true); });
	}

	void TUI::editButton() {
		trySetConfiguration();
		
		if (config == nullptr) {
			showModal("Error", "Current configuration is not valid\nCannot edit ROM");
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files\nCannot edit ROM", config->project_rom.name));
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
			showModal("Error", fmt::format("{} not set in configuration files\nCannot edit ROM", config->lunar_magic_path.name));
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
			else if (event == Event::Character('c')) {
				trySetConfiguration();
				return true;
			}
			else if (event == Event::Character('v')) {
				screen.WithRestoredIO([=] { waitForEscape(); })();
				return true;
			}
			else if (event == Event::Character('p')) {
				packageButton();
				return true;
			}
			else if (event == Event::Character('r')) {
				rebuildButton();
				return true;
			}
			else if (event == Event::Character('q')) {
				quickbuildButton();
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
				return true;
			}
			++i;
		}

		return false;
			});
	}

	void TUI::emulatorButton(const std::string& emulator_to_launch) {
		if (config == nullptr) {
			showModal("Error", fmt::format("Current configuration is not valid\nCannot launch {}", emulator_to_launch));
			return;
		}

		if (!config->project_rom.isSet()) {
			showModal("Error", fmt::format("{} not set in configuration files", config->project_rom.name));
			return;
		}

		if (!fs::exists(config->project_rom.getOrThrow())) {
			showModal("Error", fmt::format("No ROM found at\n    {}\n\nCannot open in {}",
				config->project_rom.getOrThrow().string(), emulator_to_launch)
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
		return CatchEvent(component, [=](const Event& e) {
			if (e == Event::Escape) {
				show_modal = false;
				return true;
			}
			return false;
		});
	}

	Component TUI::getChoiceModal() {
		auto component = Container::Horizontal({
			Button("Yes", [=] { show_choice_modal = false; choice_yes_func(); }, ButtonOption::Ascii()),
			Button("No", [=] { show_choice_modal = false; choice_no_func(); }, ButtonOption::Ascii())
			});

		component |= Renderer([&](Element inner) {
			return window(text(choice_modal_title), vbox({
						nonWrappingParagraph(choice_modal_text),
						separator(),
						inner | center,
				}))
				| size(WIDTH, GREATER_THAN, 30);
			});
		return CatchEvent(component, [=](const Event& e) {
			if (e == Event::Character('y')) {
				show_choice_modal = false;
				choice_yes_func();
				return true;
			}
			else if (e == Event::Character('n') || e == Event::Escape) {
				show_choice_modal = false;
				choice_no_func();
				return true;
			}
		return false;
			});
	}

	void TUI::showChoiceModal(const std::string& new_title, const std::string& new_text,
		std::function<void()> yes_function, std::function<void()> no_function) {
		show_choice_modal = true;
		choice_modal_title = new_title;
		choice_modal_text = new_text;
		choice_yes_func = yes_function;
		choice_no_func = no_function;
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
