#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <unordered_set>
#include <memory>
#include <filesystem>
namespace fs = std::filesystem;

#include "tile_format.h"
#include "filesystem_error.h"

namespace HumanReadableMap16 {
	using Byte = unsigned char;
	using _2Bytes = unsigned short int;
	using _4Bytes = unsigned int;
	using ByteIterator = std::vector<Byte>::iterator;
	using OffsetSizePair = std::pair<_4Bytes, _4Bytes>;

	constexpr const char X_FLIP_ON = 'x';
	constexpr const char X_FLIP_OFF = '-';
	constexpr const char Y_FLIP_ON = 'y';
	constexpr const char Y_FLIP_OFF = '-';
	constexpr const char PRIORITY_ON = 'p';
	constexpr const char PRIORITY_OFF = '-';

	constexpr const char* STANDARD_FORMAT = "%04X: %04X { %03X %d %c%c%c  %03X %d %c%c%c  %03X %d %c%c%c  %03X %d %c%c%c }\n";
	constexpr const char* STANDARD_FORMAT_EXAMPLE = "003C: 0130 { 0A0 7 x-p  2A2 3 xyp  3A1 5 -yp  1A3 0 --p }";

	constexpr const char* NO_ACTS_FORMAT =  "%04X:      { %03X %d %c%c%c  %03X %d %c%c%c  %03X %d %c%c%c  %03X %d %c%c%c }\n";
	constexpr const char* NO_ACTS_FORMAT_EXAMPLE = "003C:      { 0A0 7 x-p  2A2 3 xyp  3A1 5 -yp  1A3 0 --p }";

	constexpr const char* NO_TILES_FORMAT = "%04X: %04X\n";
	constexpr const char* NO_TILES_EXAMPLE = "003C: 0130";

	constexpr const char LM_EMPTY_TILE_SHORTHAND = '~';
	constexpr const char* LM_EMPTY_TILE_FORMAT = "%04X: ~\n";
	constexpr const char* LM_EMTPY_TILE_FORMAT_NO_NEWLINE = "%04X: ~";
	constexpr _2Bytes LM_EMPTY_TILE_ACTS_LIKE = 0x130;
	constexpr _2Bytes LM_EMPTY_TILE_WORD = 0x1004;

	constexpr size_t PAGE_SIZE = 0x100;
	constexpr size_t _16x16_BYTE_SIZE = 8;
	constexpr size_t ACTS_LIKE_SIZE = 2;

	constexpr size_t BASE_OFFSET = 0;

	constexpr size_t LM16_STR_OFFSET = BASE_OFFSET;
	constexpr size_t LM16_STR_SIZE = 4;

	constexpr size_t FILE_FORMAT_VERSION_NUMBER_OFFSET = LM16_STR_OFFSET + LM16_STR_SIZE;
	constexpr size_t FILE_FORMAT_VERSION_NUMBER_SIZE = 2;

	constexpr size_t GAME_ID_OFFSET = FILE_FORMAT_VERSION_NUMBER_OFFSET + FILE_FORMAT_VERSION_NUMBER_SIZE;
	constexpr size_t GAME_ID_SIZE = 2;

	constexpr size_t PROGRAM_VERSION_OFFSET = GAME_ID_OFFSET + GAME_ID_SIZE;
	constexpr size_t PROGRAM_VERSION_SIZE = 2;

	constexpr size_t PROGRAM_ID_OFFSET = PROGRAM_VERSION_OFFSET + PROGRAM_VERSION_SIZE;
	constexpr size_t PROGRAM_ID_SIZE = 2;

	constexpr size_t EXTRA_FLAGS_OFFSET = PROGRAM_ID_OFFSET + PROGRAM_ID_SIZE;
	constexpr size_t EXTRA_FLAGS_SIZE = 4;

	constexpr size_t OFFSET_SIZE_TABLE_OFFSET_OFFSET = EXTRA_FLAGS_OFFSET + EXTRA_FLAGS_SIZE;
	constexpr size_t OFFSET_SIZE_TABLE_OFFSET_SIZE = 4;

	constexpr size_t OFFSET_SIZE_TABLE_SIZE_OFFSET = OFFSET_SIZE_TABLE_OFFSET_OFFSET + OFFSET_SIZE_TABLE_OFFSET_SIZE;
	constexpr size_t OFFSET_SIZE_TABLE_SIZE_SIZE = 4;

	constexpr size_t SIZE_X_OFFSET = OFFSET_SIZE_TABLE_SIZE_OFFSET + OFFSET_SIZE_TABLE_SIZE_SIZE;
	constexpr size_t SIZE_X_SIZE = 4;

	constexpr size_t SIZE_Y_OFFSET = SIZE_X_OFFSET + SIZE_X_SIZE;
	constexpr size_t SIZE_Y_SIZE = 4;

	constexpr size_t BASE_X_OFFSET = SIZE_Y_OFFSET + SIZE_Y_SIZE;
	constexpr size_t BASE_X_SIZE = 4;

	constexpr size_t BASE_Y_OFFSET = BASE_X_OFFSET + BASE_X_SIZE;
	constexpr size_t BASE_Y_SIZE = 4;

	constexpr size_t VARIOUS_FLAGS_AND_INFO_OFFSET = BASE_Y_OFFSET + BASE_Y_SIZE;
	constexpr size_t VARIOUS_FLAGS_AND_INFO_SIZE = 4;

	constexpr size_t UNUSED_AREA_SIZE = 0x14;

	constexpr size_t COMMENT_FIELD_OFFSET = VARIOUS_FLAGS_AND_INFO_OFFSET + VARIOUS_FLAGS_AND_INFO_SIZE + UNUSED_AREA_SIZE;

	constexpr size_t OFFSET_SIZE_TABLE_SIZE = 0x40;

	struct Header {
		const char* lm16 = "LM16";
		_4Bytes file_format_version_number;
		_4Bytes game_id;
		_4Bytes program_version;
		_4Bytes program_id;
		_4Bytes extra_flags;

		_4Bytes offset_size_table_offset;
		_4Bytes offset_size_table_size;

		_4Bytes size_x;
		_4Bytes size_y;

		_4Bytes base_x;
		_4Bytes base_y;

		_4Bytes various_flags_and_info;

		// 0x14 unused bytes here

		// optional data here (comment field)

		std::string comment;

		// 0x30 bytes copyright string (will insert fine without)
	};

	bool has_tileset_specific_page_2s(std::shared_ptr<Header> header);
	bool is_full_game_export(std::shared_ptr<Header> header);

	class from_map16 {
		private:
			static void write_header_file(std::shared_ptr<Header> header, const fs::path header_path);

			static std::vector<Byte> read_binary_file(const fs::path file);
			static _4Bytes join_bytes(ByteIterator begin, ByteIterator end);
			static std::shared_ptr<Header> get_header_from_map16_buffer(std::vector<Byte> map16_buffer);
			static std::vector<OffsetSizePair> get_offset_size_table(std::vector<Byte> map16_buffer, std::shared_ptr<Header> header);

			static bool try_LM_empty_convert(FILE* fp, unsigned int tile_number, _2Bytes acts_like, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4);
			static bool try_LM_empty_convert(FILE* fp, unsigned int tile_number, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4);

			static void convert_to_file(FILE* fp, unsigned int tile_numer, _2Bytes acts_like, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4);
			static void convert_to_file(FILE* fp, unsigned int tile_numer, _2Bytes acts_like);
			static void convert_to_file(FILE* fp, unsigned int tile_numer, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4);

			// for pages 0x2/0x3-0x7F, converts tiles and acts like settings, tiles_start_offset and acts_like_start_offset should both just be the offsets from the header
			static void convert_FG_page(std::vector<Byte> map16_buffer, unsigned int page_number,
				size_t tiles_start_offset, size_t acts_like_start_offset);

			// for pages 0x80-0xFF, converts tiles only since BG pages do not have acts like settings
			static void convert_BG_page(std::vector<Byte> map16_buffer, unsigned int page_number, size_t tiles_start_offset);
			
			// for pages 0x0-0x1 of tileset groups 0x0-0x4, only converts tile numbers of tileset-group-specific tiles, includes diagonal pipe tiles in tileset group 0x0
			static void convert_tileset_group_specific_pages(std::vector<Byte> map16_buffer, unsigned int tileset_group_number,
				size_t tiles_start_offset, size_t diagonal_pipes_offset);

			// for page 0x2 of tilesets 0x0-0xE, if page 2 is set to be tileset-specific
			static void convert_tileset_specific_page_2(std::vector<Byte> map16_buffer, unsigned int tileset_number, size_t tiles_start_offset);

			static void convert_global_page_2_for_tileset_specific_page_2s(std::vector<Byte> map16_buffer, size_t acts_like_offset);

			// converts one set of 8 pipe tiles for pipe tile numbers 0x0-0x3, no acts like settings
			static void convert_normal_pipe_tiles(std::vector<Byte> map16_buffer, unsigned int pipe_number, size_t normal_pipe_offset);

			static void convert_first_two_non_tileset_specific(std::vector<Byte> map16_buffer, size_t tileset_group_specific_pair, size_t acts_like_pair);

		public:
			static void convert(const fs::path input_file, const fs::path output_path);
	};

	class to_map16 {
		private:
			static std::shared_ptr<Header> parse_header_file(const fs::path header_path);
			static void verify_header_file(const fs::path header_path);

			// static void get_line_or_throw(std::fstream* stream, std::string& string, const fs::path file_path, unsigned int curr_line_number);

			static std::vector<fs::path> get_sorted_paths(const fs::path directory);

			static void split_and_insert_2(_2Bytes bytes, std::vector<Byte>& byte_vec);
			static void split_and_insert_4(_4Bytes bytes, std::vector<Byte>& byte_vec);

			static _2Bytes to_bytes(_2Bytes _8x8_tile_number, unsigned int palette, char x_flip, char y_flip, char priority);

			static bool try_LM_empty_convert_full(std::vector<Byte>& tiles_vec, std::vector<Byte>& acts_like_vec, const std::string line, unsigned int expected_tile_number);
			static bool try_LM_empty_convert_tiles_only(std::vector<Byte>& tiles_vec, const std::string line, unsigned int expected_tile_number);

			static void convert_full(std::vector<Byte>& tiles_vec, std::vector<Byte>& acts_like_vec, const std::string line, unsigned int expected_tile_number, unsigned int line_number, const fs::path file);
			static void verify_full(const std::string line, unsigned int line_number, const fs::path file, unsigned int expected_tile_number);

			static void convert_acts_like_only(std::vector<Byte>& acts_like_vec, const std::string line, unsigned int expected_tile_number, unsigned int line_number, const fs::path file);
			static void verify_acts_like_only(const std::string line, unsigned int line_number, const fs::path file, unsigned int expected_tile_number);

			static void convert_tiles_only(std::vector<Byte>& tiles_vec, const std::string line, unsigned int expected_tile_number, unsigned int line_number, const fs::path file);
			static void verify_tiles_only(const std::string line, unsigned int line_number, const fs::path file, unsigned int expected_tile_number);

			static void verify_tile_number(const std::string line, unsigned int line_number, const fs::path file, unsigned int& curr_char_idx, 
				TileFormat tile_format, unsigned int expected_tile_number);

			static void verify_acts_like(const std::string line, unsigned int line_number, const fs::path file, unsigned int& curr_char_idx, TileFormat tile_format,
				unsigned int expected_tile_number);

			static void verify_8x8_tiles(const std::string line, unsigned int line_number, const fs::path file, unsigned int& curr_char_idx, TileFormat tile_format,
				unsigned int expected_tile_number);

			static void verify_8x8_tile(const std::string line, unsigned int line_number, const fs::path file, 
				unsigned int expected_tile_number, unsigned int& curr_char_idx, TileFormat tile_format);

			static unsigned int parse_BG_pages(std::vector<Byte>& bg_tiles_vec, unsigned int base_tile_number);
			static unsigned int parse_FG_pages(std::vector<Byte>& fg_tiles_vec, std::vector<Byte>& acts_like_vec, unsigned int base_tile_number);
			static unsigned int parse_FG_pages_tileset_specific_page_2(std::vector<Byte>& fg_tiles_vec, std::vector<Byte>& acts_like_vec, 
				std::vector<Byte>& tileset_specific_tiles_vec, unsigned int base_tile_number);

			static void parse_tileset_group_specific_pages(std::vector<Byte>& tileset_group_specific_tiles_vec, 
				std::vector<Byte>& diagonal_pipe_tiles_vec, const std::vector<Byte>& fg_tiles_vec);

			static void duplicate_tileset_group_specific_pages(std::vector<Byte>& tileset_group_specific_tiles_vec);

			static void parse_tileset_specific_pages(std::vector<Byte>& tileset_specific_tiles_vec);

			static void parse_normal_pipe_tiles(std::vector<Byte>& pipe_tiles_vec);

			static std::vector<Byte> get_offset_size_vec(size_t header_size, size_t fg_tiles_size,
				size_t bg_tiles_size, size_t acts_like_size, size_t tileset_specific_size, size_t tileset_group_specific_size, size_t normal_pipe_tiles_size,
				size_t diagonal_pipe_tiles_size);

			static std::vector<Byte> get_header_vec(std::shared_ptr<Header> header);

			static std::vector<Byte> combine(std::vector<Byte> header_vec, std::vector<Byte> offset_size_vec, std::vector<Byte> fg_tiles_vec, 
				std::vector<Byte> bg_tiles_vec, std::vector<Byte> acts_like_vec, std::vector<Byte> tileset_specific_vec, 
				std::vector<Byte> tileset_group_specific_vec, std::vector<Byte> normal_pipe_tiles_vec, std::vector<Byte> diagonal_pipe_tiles_vec);

		public: 
			// static void tests();
			static void convert(const fs::path input_path, const fs::path output_file);
	};
}
