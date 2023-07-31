#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "data_error.h"

class HeaderError : public DataError {
	private:
		std::string expected_variable;

	public:
		HeaderError(std::string message, fs::path file, unsigned int line_number, std::string line, unsigned int char_index, std::string variable)
			: DataError(message, file, line_number, line, char_index) {
			expected_variable = variable;
		}

		const std::string get_variable() {
			return expected_variable;
		}

		std::string get_detailed_error_message() {
			std::ostringstream s;
			s << "Error in file \"" << get_file_path().string() << "\" on line " << get_line_number() << " at character " << get_char_index() << ":" << std::endl <<
				"    \"" << get_line() << '\"' << std::endl;
			for (int i = 0; i != get_char_index() + 4; i++) {
				s << " ";
			}
			s << "^" << std::endl;
			for (int i = 0; i != get_char_index() + 4; i++) {
				s << " ";
			}
			s << "|" << std::endl;
			for (int i = 0; i != get_char_index() + 4; i++) {
				s << " ";
			}
			s << get_message() << std::endl;

			return s.str();
		}
};
