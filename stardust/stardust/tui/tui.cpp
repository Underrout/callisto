#include "tui.h"

namespace stardust {
	TUI::TUI(const fs::path& stardust_directory) : config_manager(stardust_directory), stardust_directory(stardust_directory),
		screen(ScreenInteractive::Fullscreen())
	{
		screen.SetCursor(ftxui::Screen::Cursor{ .shape = ftxui::Screen::Cursor::Shape::Hidden });

		spdlog::set_level(spdlog::level::info);
		spdlog::set_pattern("[%l] %v");
	}

	Component TUI::getConfigOnlyButton(const std::string& button_text, Closure button_func) {
		auto option{ ButtonOption::Ascii() };
		option.transform = [&](const EntryState& e) {
			auto label{ e.label };
			if (e.active || e.focused) {
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
			if (e.focused) {
				element |= bold;
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
			if (e.active || e.focused) {
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
			if (e.focused) {
				element |= bold;
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

			getRomOnlyButton("Save", [&] { showModal("Saving", "Now"); }),
			getRomOnlyButton("Edit", [&] { showModal("Editing", "yup"); }),

			Renderer([] { return separator(); }),

			Button("Reload configuration", [&] { trySetConfiguration(); }, ButtonOption::Ascii()),
			Button("Reload profiles", [&] {
				profile_names = config_manager.getProfileNames();
				config = nullptr;
				determineInitialProfile();
				emulator_menu_component->DetachAllChildren();
				if (config != nullptr) {
					emulator_names = emulators.getEmulatorNames();
					for (const auto& emulator_name : emulator_names) {
						emulator_menu_component->Add(getRomOnlyButton(emulator_name, [] {}));
					}
				}
			}, ButtonOption::Ascii()),

			Renderer([] { return separator(); }),

			Button("Exit", [] { exit(0); }, ButtonOption::Ascii())
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
		for (const auto& emulator_name : emulator_names) {
			emulator_menu_component->Add(getRomOnlyButton(emulator_name, [] {}));
		}
		const auto windowed{ Renderer(emulator_menu_component, [=] { return window(text("Emulators"), emulator_menu_component->Render()); }) };
		emulator_menu = Maybe(windowed, [&] { return !emulator_names.empty(); });
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
				invokeCatchError([&] { setConfiguration(*found, stardust_directory); });
				used_cached = true;
			}
		}

		if (!used_cached) {
			if (!profile_names.empty()) {
				selected_profile_menu_entry = 0;
				invokeCatchError([&] { setConfiguration(profile_names.at(0), stardust_directory); });
			}
			else {
				invokeCatchError([&] { setConfiguration({}, stardust_directory); });
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

		const auto wrapped{ CatchEvent(full_menu, [&](Event event) {
			if (event == Event::Escape) {
				exit(0);
				return true;
			}
			return false;
		}) };

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
		catch (const std::runtime_error&) {
			config = nullptr;
			throw;
		}
	}

	void TUI::trySetConfiguration() {
		const bool succeeded{ invokeCatchError([&] {
			if (profile_names.empty()) {
				setConfiguration({}, stardust_directory);
			}
			else {
				setConfiguration(profile_names.at(selected_profile_menu_entry), stardust_directory);
			}
		}) };
		if (succeeded) {
			emulators = Emulators(*config);
			emulator_names = emulators.getEmulatorNames();
		}
		else {
			config = nullptr;
			emulator_names.clear();
		}
		emulator_menu_component->DetachAllChildren();
		for (const auto& emulator_name : emulator_names) {
			emulator_menu_component->Add(getRomOnlyButton(emulator_name, [] {}));
		}
		previously_selected_profile_menu_entry = selected_profile_menu_entry;
	}

	bool TUI::invokeCatchError(std::function<void()> func) {
		bool no_error{ false };
		try {
			func();
			no_error = true;
		}
		catch (const TomlException2& e) {
			modal_text = toml::format_error(e.what(), e.toml_value, e.comment, e.toml_value2, e.comment2, e.hints);
		}
		catch (const TomlException& e) {
			modal_text = toml::format_error(e.what(), e.toml_value, e.comment, e.hints);
		}
		catch (const toml::syntax_error& e) {
			modal_text = e.what();
		}
		catch (const toml::type_error& e) {
			modal_text = e.what();
		}
		catch (const toml::internal_error& e) {
			modal_text = e.what();
		}
		catch (const StardustException& e) {
			modal_text = e.what();
		}
		catch (const std::runtime_error& e) {
			modal_text = e.what();
		}
		catch (const json::exception& e) {
			modal_text = e.what();
		}

		if (no_error) {
			return true;
		}
		else {
			show_modal = true;
			modal_title = "Error";
			return false;
		}
	}

	Component TUI::getModal() {
		auto component = Container::Vertical({
			Button("OK", [&] { show_modal = false; }, ButtonOption::Ascii()),
			});

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
