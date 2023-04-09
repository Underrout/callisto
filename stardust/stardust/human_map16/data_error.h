#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "human_map16_exception.h"

class DataError : public HumanMap16Exception {
private:
	fs::path in_file;
	unsigned int _line_number;
	std::string incorrect_line;
	unsigned int _char_index;

public:
	DataError(std::string message, fs::path file, unsigned int line_number, std::string line, unsigned int char_index) : HumanMap16Exception(message) {
		in_file = file;
		_line_number = line_number;
		incorrect_line = line;
		_char_index = char_index + 1;
	}

	const fs::path get_file_path() {
		return in_file;
	}

	unsigned int get_line_number() {
		return _line_number;
	}

	const std::string get_line() {
		return incorrect_line;
	}

	unsigned int get_char_index() {
		return _char_index;
	}
};
