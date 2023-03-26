#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include <sstream>

#include "data_error.h"
#include "tile_format.h"
#include "human_readable_map16.h"

class TileError : public DataError {
	private:
		TileFormat expected_tile_format;
		unsigned int expected_tile_number;

	public:
		TileError(std::string message, fs::path file, unsigned int line_number, std::string line, 
			unsigned int char_index, TileFormat tile_format, unsigned int tile_number) : DataError(message, file, line_number, line, char_index) {
			expected_tile_format = tile_format;
			expected_tile_number = tile_number;
		}

		TileFormat get_format() {
			return expected_tile_format;
		}

		unsigned int get_tile_number() {
			return expected_tile_number;
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
			s << "Expected format example (";

			switch (expected_tile_format) {
				case TileFormat::FULL:
					s << "acts like settings & 8x8 tiles):" << std::endl << "    \"" << HumanReadableMap16::STANDARD_FORMAT_EXAMPLE;
					break;
				case TileFormat::ACTS_LIKE_ONLY:
					s << "acts like settings only):" << std::endl << "    \"" << HumanReadableMap16::NO_TILES_EXAMPLE;
					break;
				case TileFormat::TILES_ONLY:
					s << "8x8 tiles only):" << std::endl << "    \"" << HumanReadableMap16::NO_ACTS_FORMAT_EXAMPLE;
					break;
			}

			s << '\"';

			return s.str();
		}
};
