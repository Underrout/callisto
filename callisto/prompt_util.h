#pragma once

#include <iostream>

#include <spdlog/spdlog.h>

namespace callisto {
	class PromptUtil {
	public:
		template<typename L>
		static void yesNoPrompt(const std::string& prompt, L&& on_yes) {
			spdlog::warn("{} Y/N", prompt);
			char response;
			std::cin >> response;
			std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
			if (response == 'y' || response == 'Y') {
				on_yes();
			}
		}
	};
}