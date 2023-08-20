#include "eloper.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"eloper";
WCHAR szWindowClass[MAX_LOADSTRING] = L"eloper";

bp::child lunar_magic_process;
callisto::ProcessInfo process_info;
HWND message_only_window;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);

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

	const auto& usertoolbar_path{ argv[3] };
	restoreUsertoolbar(usertoolbar_path);

	const auto& lm_verification_string{ argv[1] };

	const auto [hwnd, verification_code] = extractHandleAndVerificationCode(lm_verification_string);
	const auto pid{ getLunarMagicPid(hwnd) };
	lunar_magic_process = bp::child(pid);
	process_info = callisto::ProcessInfo(lunar_magic_process.id());

	process_info.setLunarMagicMessageWindowHandle(hwnd);
	process_info.setLunarMagicVerificationCode(verification_code);

	const auto& current_rom{ argv[2] };
	process_info.setCurrentLunarMagicRomPath(current_rom);

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

			switch (static_cast<LunarMagicNotificationType>(notification_type)) {
			case LunarMagicNotificationType::NEW_ROM:
				handleNewRom((HWND)wparam);
				break;

			case LunarMagicNotificationType::SAVE_LEVEL:
				[[fallthrough]];
			case LunarMagicNotificationType::SAVE_MAP16:
				[[fallthrough]];
			case LunarMagicNotificationType::SAVE_OW:
				handleSave();
				break;

			default:
				break;
			}
		}
	}

	return DefWindowProc(hwnd, umsg, wparam, lparam);
}


std::pair<HWND, uint16_t> extractHandleAndVerificationCode(const std::string& lm_string) {
	const auto colon{ lm_string.find(':') };

	const auto hwnd_part{ lm_string.substr(0, colon) };
	const auto hwnd{ reinterpret_cast<HWND>(std::stoi(hwnd_part, nullptr, 16)) };

	const auto verification_part{ lm_string.substr(colon + 1) };
	const auto verification_code{ static_cast<uint16_t>(std::stoi(verification_part, nullptr, 16)) };

	return { hwnd, verification_code };
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

void handleSave() {
	// TODO make callisto export in separate process here, maybe make it interruptable too
	// so that we can terminate it early if the user saves something again
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
