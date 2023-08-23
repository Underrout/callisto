#include "eloper.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"eloper";
WCHAR szWindowClass[MAX_LOADSTRING] = L"eloper";

bp::child lunar_magic_process;
callisto::ProcessInfo process_info;
HWND message_only_window;
fs::path callisto_path;

std::atomic<bool> cancel{ false };
std::mutex m;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);

	/*
	auto file_logger = spdlog::basic_logger_mt("file_logger", "log.txt");
	spdlog::set_default_logger(file_logger);
	spdlog::flush_on(spdlog::level::info);
	*/

	registerClass(hInstance);

	if (!initInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	int w_argc{ 0 };
	LPWSTR* wargv{ CommandLineToArgvW(GetCommandLineW(), &w_argc) };

	std::vector<std::string> argv{};
	int argc{ 0 };

	for (int i = 0; i < w_argc; ++i) {
		int w_len = lstrlenW(wargv[i]);
		int len = WideCharToMultiByte(CP_ACP, 0, wargv[i], w_len, NULL, 0, NULL, NULL);

		std::string temp(len, 0);
		WideCharToMultiByte(CP_ACP, 0, wargv[i], w_len, temp.data(), len + 1, NULL, NULL);
		argv.push_back(temp);

		++argc;
	}

	callisto_path = argv[4];
	const auto& usertoolbar_path{ argv[3] };
	restoreUsertoolbar(usertoolbar_path);

	const auto& lm_verification_string{ argv[1] };

	const auto [hwnd, verification_code] = extractHandleAndVerificationCode(lm_verification_string);
	const auto pid{ getLunarMagicPid(hwnd) };
	lunar_magic_process = bp::child(pid);
	process_info = callisto::ProcessInfo(lunar_magic_process.id(), true);

	process_info.setLunarMagicMessageWindowHandle(hwnd);
	process_info.setLunarMagicVerificationCode(verification_code);

	const auto& current_rom{ argv[2] };
	process_info.setCurrentLunarMagicRomPath(current_rom);
	process_info.setProjectRomPath(current_rom);

	MSG msg;

	while (!lunar_magic_process.wait_for(std::chrono::milliseconds(100))) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Construct below serves purely to account for saving stuff on Lunar Magic exit
	// Pretty hacky, but does seem to work
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::this_thread::sleep_for(std::chrono::seconds(10));

	while (process_info.getSaveProcessPid().has_value()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return (int)msg.wParam;
}

ATOM registerClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;


	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = szTitle;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;

	return RegisterClassExW(&wcex);
}

BOOL initInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	if (umsg == LM_MESSAGE_ID) {
		if (lparam >> 16 == LM_CONFIRMATION_CODE) {
			const auto notification_type{ lparam & 0x3F };

			if (notification_type < static_cast<int>(LunarMagicNotificationType::NEW_ROM)) {
				return FALSE;
			}
			if (notification_type > static_cast<int>(LunarMagicNotificationType::SAVE_OW)) {
				return FALSE;
			}

			std::thread thr;
			switch (static_cast<LunarMagicNotificationType>(notification_type)) {
			case LunarMagicNotificationType::NEW_ROM:
				handleNewRom((HWND)wparam);
				break;

			case LunarMagicNotificationType::SAVE_LEVEL:
				[[fallthrough]];
			case LunarMagicNotificationType::SAVE_MAP16:
				[[fallthrough]];
			case LunarMagicNotificationType::SAVE_OW:
				thr = std::thread([&] { handleSave(); });
				thr.detach();
				break;

			default:
				break;
			}
		}
	}

	return DefWindowProc(hwnd, umsg, wparam, lparam);
}


std::unordered_set<std::string> getProfileNames(const fs::path& callisto_path) {
	if (fs::exists(callisto_path)) {
		bp::ipstream ip;
		bp::child profiles_process(callisto_path.string(), "--profiles", bp::std_out > ip, bp::windows::create_no_window);

		std::unordered_set<std::string> profile_names{};
		std::string line;
		while (std::getline(ip, line)) {
			profile_names.insert(line.substr(line.size() - 1));  // drop newline from end of line
		}

		profiles_process.wait();

		return profile_names;
	}
	else {
		throw EloperException(fmt::format("Callisto not found at path '{}', cannot get profile names", callisto_path.string()));
	}
}

std::pair<HWND, uint16_t> extractHandleAndVerificationCode(const std::string& lm_string) {
	const auto colon{ lm_string.find(':') };

	const auto hwnd_part{ lm_string.substr(0, colon) };
	const auto hwnd{ reinterpret_cast<HWND>(std::stoi(hwnd_part, nullptr, 16)) };

	const auto verification_part{ lm_string.substr(colon + 1) };
	const auto verification_code{ static_cast<uint16_t>(std::stoi(verification_part, nullptr, 16)) };

	return { hwnd, verification_code };
}

std::optional<std::string> getLastConfigName(const fs::path& callisto_directory) {
	const auto path{ callisto_directory / ".cache" / "last_profile.txt" };
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

bp::pid_t getLunarMagicPid(HWND message_window_handle) {
	bp::pid_t pid;
	GetWindowThreadProcessId(message_window_handle, &pid);
	return pid;
}

void handleNewRom(HWND message_window_hwnd) {
	std::string rom_path(MAX_PATH, 0);
	GetWindowText(message_window_hwnd, rom_path.data(), MAX_PATH);
	process_info.setCurrentLunarMagicRomPath(rom_path);
}

std::optional<std::string> determineSaveProfile(const fs::path& callisto_path) {
	const auto profile_names{ getProfileNames(callisto_path) };
	if (profile_names.empty()) {
		return {};
	}
	else {
		return *profile_names.begin();
	}

	/* 
	* TODO the commented out portion here almost handles different profile 
	* names and such, but honestly if your profile influences how saving works, 
	* you're probably doing something really weird, so I'm not going to handle
	* this for now and just return the first profile if there are any and nullopt otherwise
	
	const auto last_used{ getLastConfigName(callisto_path.parent_path()) };
	const auto profile_names{ getProfileNames(callisto_path) };

	if (!last_used.has_value()) {
		if (profile_names.empty()) {
			return {};
		}
		else if (profile_names.size() == 1) {
			return *profile_names.begin();
		}
		else {
			// TODO prompt user for which profile to use
		}
	}
	else {
		const auto& last_used_name{ last_used.value() };
		if (profile_names.contains(last_used_name)) {
			return last_used_name;
		}
		else if (profile_names.empty()) {
			return {};
		}
		else {
			// TODO handle profile name no longer being in list, probably prompt user for which profile to use as well
		}
	}
	*/
}

void handleSave() {
	const auto id{ std::hash<std::thread::id>{}(std::this_thread::get_id()) };

	if (process_info.getCurrentLunarMagicRomPath().value() != process_info.getProjectRomPath()) {
		// save of non-project ROM, skip
		return;
	}

	try {
		checkForCallisto(callisto_path);
	}
	catch (const EloperException&) {
		const auto message{ fmt::format("Callisto executable not found at '{}', cannot automatically export resources!", callisto_path.string()) };
		MessageBox(NULL, message.data(), "Callisto not found", MB_OK | MB_ICONWARNING);
		return;
	}

	const auto profile{ determineSaveProfile(callisto_path) };

	const auto callisto_save_process_pid{ process_info.getSaveProcessPid() };

	spdlog::info("Thread {}: Checking for pending save", id);
	if (callisto_save_process_pid.has_value()) {
		cancel = true;
		spdlog::info("Thread {}: Sending cancel", id, callisto_save_process_pid.value());
	}

	std::scoped_lock lock(m);

	bp::ipstream output;

	bp::group g;
	bp::child new_save_process;
	if (profile.has_value()) {
		new_save_process = bp::child(callisto_path.string(), "save", "--profile", profile.value(), bp::std_out > output, bp::windows::create_no_window, g);
	}
	else {
		new_save_process = bp::child(callisto_path.string(), "save", bp::std_out > output, bp::windows::create_no_window, g);
	}

	spdlog::info("Thread {}: Started new save process {:X}", id, new_save_process.id());
	process_info.setSaveProcessPid(g.native_handle());

	spdlog::info("Thread {}: Waiting for save process {:X} to exit", id, new_save_process.id());
	bool received_cancel{ false };
	while (new_save_process.running()) {
		if (received_cancel) {
			spdlog::info("Thread {}: But it's still running...", id);
			continue;
		}

		if (cancel) {
			spdlog::info("Thread {}: Received cancel, terminating save process {:X} and returning", id, new_save_process.id());
			g.terminate();
			cancel = false;
			received_cancel = true;
		}
		else {
			spdlog::info("Thread {}: Not cancelled", id);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (received_cancel) {
		process_info.unsetSaveProcessPid();
		return;
	}

	g.wait();
	spdlog::info("Thread {}: Finished waiting for save process {:X}", id, new_save_process.id());
	process_info.unsetSaveProcessPid();

	const auto& exit_code{ new_save_process.exit_code() };

	spdlog::info("Thread {}: Exit code = {}", id, exit_code);
	if (exit_code == 1) {
		spdlog::info("Thread {}: Got terminated", id);
		// terminate called on our process, ignore it
		return;
	}

	if (exit_code != 0) {
		spdlog::info("Thread {}: Save failed", id);
		const fs::path error_log_path{ callisto_path.parent_path() / "failed_save.txt" };
		std::ofstream error_log{ error_log_path };
		std::string line;
		while (std::getline(output, line)) {
			error_log << line;
		}
		error_log.close();

		const auto message{ fmt::format(
			"Automatic save of resources using callisto failed, "
			"please refer to error log at '{}'",
			error_log_path.string()
		) };

		MessageBox(NULL, message.data(), "Callisto save failed", MB_OK | MB_ICONWARNING);
	}
	else {
		spdlog::info("Thread {}: Save succeeded", id);
	}
}

void checkForCallisto(const fs::path& callisto_path) {
	if (!fs::exists(callisto_path)) {
		throw EloperException(fmt::format("Callisto not found at advertised path '{}'", callisto_path.string()));
	}
}

void restoreUsertoolbar(const fs::path& usertoolbar_path) {
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
