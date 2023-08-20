#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <utility>

#include <boost/process.hpp>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace bp = boost::process;
namespace fs = std::filesystem;

#include "../process_info.h"

#include "../callisto_exception.h"

#include <Windows.h>
#include <shellapi.h>
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

constexpr auto CALLISTO_MAGIC_MARKER{ "; Callisto generated call below, if you can see this, you should remove it!" };

constexpr auto LM_MESSAGE_ID{ 0xBECA };
constexpr auto LM_CONFIRMATION_CODE{ 0x6942 };  // is anyone seriously going to use this?

enum class LunarMagicNotificationType {
	NEW_ROM = 0,
	SAVE_LEVEL = 3,
	SAVE_MAP16 = 4,
	SAVE_OW = 5
};

ATOM registerClass(HINSTANCE hInstance);
BOOL initInstance(HINSTANCE, int);
LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

bp::pid_t getLunarMagicPid(HWND message_window_handle);

void handleNewRom(HWND message_window_hwnd);
void handleSave();

std::pair<HWND, uint16_t> extractHandleAndVerificationCode(const std::string& lm_string);

void restoreUsertoolbar(const fs::path& usertoolbar_path);
