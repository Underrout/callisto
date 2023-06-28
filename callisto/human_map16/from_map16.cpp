#include "human_readable_map16.h"
#include "arrays.h"

HumanReadableMap16::_4Bytes HumanReadableMap16::from_map16::join_bytes(ByteIterator begin, ByteIterator end) {
	_4Bytes t = 0;
	unsigned int i = 0;
	for (auto& curr = begin; curr != end; ++curr) {
		Byte c = *curr;
		t += c << (i++ * 8);
	}
	return t;
}

std::vector<HumanReadableMap16::Byte> HumanReadableMap16::from_map16::read_binary_file(const fs::path file) {
	std::ifstream f(file.string(), std::ios::in | std::ios::binary);

	f.unsetf(std::ios::skipws);

	f.seekg(0, std::ios::end);
	size_t size = f.tellg();
	f.seekg(0, std::ios::beg);
	std::vector<uint8_t> vec; 
	vec.resize(size);
	f.read(reinterpret_cast<std::ifstream::char_type*>(&vec.front()), size);

	f.close();

	return vec;
}

std::shared_ptr<HumanReadableMap16::Header> HumanReadableMap16::from_map16::get_header_from_map16_buffer(std::vector<Byte> map16_buffer) {
	auto header = std::make_shared<Header>();
	const auto& beg = map16_buffer.begin();

	header->lm16 = "LM16";
	header->file_format_version_number = join_bytes(beg + FILE_FORMAT_VERSION_NUMBER_OFFSET, beg + GAME_ID_OFFSET);
	header->game_id = join_bytes(beg + GAME_ID_OFFSET, beg + PROGRAM_VERSION_OFFSET);
	header->program_version = join_bytes(beg + PROGRAM_VERSION_OFFSET, beg + PROGRAM_ID_OFFSET);
	header->program_id = join_bytes(beg + PROGRAM_ID_OFFSET, beg + EXTRA_FLAGS_OFFSET);
	header->extra_flags = join_bytes(beg + EXTRA_FLAGS_OFFSET, beg + OFFSET_SIZE_TABLE_OFFSET_OFFSET);
	header->offset_size_table_offset = join_bytes(beg + OFFSET_SIZE_TABLE_OFFSET_OFFSET, beg + OFFSET_SIZE_TABLE_SIZE_OFFSET);
	header->offset_size_table_size = join_bytes(beg + OFFSET_SIZE_TABLE_SIZE_OFFSET, beg + SIZE_X_OFFSET);
	header->size_x = join_bytes(beg + SIZE_X_OFFSET, beg + SIZE_Y_OFFSET);
	header->size_y = join_bytes(beg + SIZE_Y_OFFSET, beg + BASE_X_OFFSET);
	header->base_x = join_bytes(beg + BASE_X_OFFSET, beg + BASE_Y_OFFSET);
	header->base_y = join_bytes(beg + BASE_Y_OFFSET, beg + VARIOUS_FLAGS_AND_INFO_OFFSET);
	header->various_flags_and_info = join_bytes(beg + VARIOUS_FLAGS_AND_INFO_OFFSET, beg + VARIOUS_FLAGS_AND_INFO_OFFSET + VARIOUS_FLAGS_AND_INFO_SIZE);

	for (unsigned int i = 0; i != header->offset_size_table_offset - COMMENT_FIELD_OFFSET; i++) {
		header->comment += map16_buffer.at(COMMENT_FIELD_OFFSET + i);
	}

	return header;
}

std::vector<HumanReadableMap16::OffsetSizePair> HumanReadableMap16::from_map16::get_offset_size_table(std::vector<Byte> map16_buffer, std::shared_ptr<Header> header) {
	_4Bytes off_table_offset = header->offset_size_table_offset;
	const auto& beg = map16_buffer.begin();

	std::vector<OffsetSizePair> pointers_and_sizes;

	for (int i = 0; i != header->offset_size_table_size / 4; i += 2) {
		pointers_and_sizes.push_back(
			OffsetSizePair(
				join_bytes(beg + off_table_offset + (4 * i), beg + off_table_offset + (4 * (i + 1))),
				join_bytes(beg + off_table_offset + (4 * (i + 1)), beg + off_table_offset + (4 * (i + 2)))
			)
		);
	}

	return pointers_and_sizes;
}


bool HumanReadableMap16::from_map16::try_LM_empty_convert(FILE* fp, unsigned int tile_number, _2Bytes acts_like, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4) {
	if (acts_like != LM_EMPTY_TILE_ACTS_LIKE || tile1 != LM_EMPTY_TILE_WORD ||
		tile2 != LM_EMPTY_TILE_WORD || tile3 != LM_EMPTY_TILE_WORD || tile4 != LM_EMPTY_TILE_WORD) {
		return false;
	}

	fprintf(fp, LM_EMPTY_TILE_FORMAT, tile_number);

	return true;
}

bool HumanReadableMap16::from_map16::try_LM_empty_convert(FILE* fp, unsigned int tile_number, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4) {
	return try_LM_empty_convert(fp, tile_number, LM_EMPTY_TILE_ACTS_LIKE, tile1, tile2, tile3, tile4);
}

#define TILE_FORMAT(tile) \
tile & 0b1111111111, \
(tile >> 10) & 0b000111, \
(tile & 0b0100000000000000) ? HumanReadableMap16::X_FLIP_ON : HumanReadableMap16::X_FLIP_OFF, \
(tile & 0b1000000000000000) ? HumanReadableMap16::Y_FLIP_ON : HumanReadableMap16::Y_FLIP_OFF, \
(tile & 0b0010000000000000) ? HumanReadableMap16::PRIORITY_ON : HumanReadableMap16::PRIORITY_OFF

void HumanReadableMap16::from_map16::convert_to_file(FILE* fp, unsigned int tile_number, _2Bytes acts_like, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4) {
	if (try_LM_empty_convert(fp, tile_number, acts_like, tile1, tile2, tile3, tile4)) {
		return;
	}

	fprintf(fp, STANDARD_FORMAT,
		tile_number, acts_like,
		TILE_FORMAT(tile1),
		TILE_FORMAT(tile2),
		TILE_FORMAT(tile3),
		TILE_FORMAT(tile4)
	);
}

void HumanReadableMap16::from_map16::convert_to_file(FILE* fp, unsigned int tile_number, _2Bytes acts_like) {
	fprintf(fp, NO_TILES_FORMAT,
		tile_number, acts_like
	);
}

void HumanReadableMap16::from_map16::convert_to_file(FILE* fp, unsigned int tile_number, _2Bytes tile1, _2Bytes tile2, _2Bytes tile3, _2Bytes tile4) {
	if (try_LM_empty_convert(fp, tile_number, tile1, tile2, tile3, tile4)) {
		return;
	}

	fprintf(fp, NO_ACTS_FORMAT,
		tile_number,
		TILE_FORMAT(tile1),
		TILE_FORMAT(tile2),
		TILE_FORMAT(tile3),
		TILE_FORMAT(tile4)
	);
}

void HumanReadableMap16::from_map16::convert_FG_page(std::vector<Byte> map16_buffer, unsigned int page_number,
	size_t tiles_start_offset, size_t acts_like_start_offset) {
	char filename[256];
	sprintf(filename, "global_pages\\FG_pages\\page_%02X.txt", page_number);
	FILE* fp = fopen(filename, "w");
	unsigned int curr_tile_number = page_number * PAGE_SIZE;
	auto curr_tile_it = map16_buffer.begin() + tiles_start_offset + PAGE_SIZE * _16x16_BYTE_SIZE * page_number;
	auto curr_acts_like_it = map16_buffer.begin() + acts_like_start_offset + PAGE_SIZE * ACTS_LIKE_SIZE * page_number;

	for (unsigned int i = 0; i != PAGE_SIZE; i++) {
		_2Bytes acts_like = join_bytes(curr_acts_like_it, curr_acts_like_it + ACTS_LIKE_SIZE);
		_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
		_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
		_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
		_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

		convert_to_file(fp, curr_tile_number, acts_like, tile1, tile2, tile3, tile4);

		if (i != PAGE_SIZE - 1) {
			fprintf(fp, "\n");
		}

		++curr_tile_number;
		curr_tile_it += _16x16_BYTE_SIZE;
		curr_acts_like_it += ACTS_LIKE_SIZE;
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_global_page_2_for_tileset_specific_page_2s(std::vector<Byte> map16_buffer, size_t acts_like_offset) {
	FILE* fp = fopen("global_pages\\FG_pages\\page_02.txt", "w");

	auto curr_acts_like_it = map16_buffer.begin() + acts_like_offset + PAGE_SIZE * ACTS_LIKE_SIZE * 2;

	unsigned int curr_tile_number = 0x200;

	for (unsigned int i = 0; i != PAGE_SIZE; i++) {
		_2Bytes acts_like = join_bytes(curr_acts_like_it, curr_acts_like_it + ACTS_LIKE_SIZE);

		convert_to_file(fp, curr_tile_number, acts_like);

		if (i != PAGE_SIZE - 1) {
			fprintf(fp, "\n");
		}

		++curr_tile_number;
		curr_acts_like_it += ACTS_LIKE_SIZE;
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_BG_page(std::vector<Byte> map16_buffer, unsigned int page_number, size_t tiles_start_offset) {
	char filename[256];
	sprintf(filename, "global_pages\\BG_pages\\page_%02X.txt", page_number);
	FILE* fp = fopen(filename, "w");
	unsigned int curr_tile_number = page_number * PAGE_SIZE;
	auto curr_tile_it = map16_buffer.begin() + tiles_start_offset + PAGE_SIZE * _16x16_BYTE_SIZE * page_number;

	for (unsigned int i = 0; i != PAGE_SIZE; i++) {
		_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
		_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
		_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
		_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

		convert_to_file(fp, curr_tile_number, tile1, tile2, tile3, tile4);

		++curr_tile_number;
		curr_tile_it += _16x16_BYTE_SIZE;

		if (i != PAGE_SIZE - 1) {
			fprintf(fp, "\n");
		}
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_tileset_group_specific_pages(std::vector<Byte> map16_buffer, unsigned int tileset_group_number,
	size_t tiles_start_offset, size_t diagonal_pipes_offset) {
	char filename[256];
	sprintf(filename, "tileset_group_specific_tiles\\tileset_group_%X.txt", tileset_group_number);
	FILE* fp = fopen(filename, "w");

	auto curr_tile_it = map16_buffer.begin() + tiles_start_offset + PAGE_SIZE * _16x16_BYTE_SIZE * 2 * tileset_group_number + _16x16_BYTE_SIZE * TILESET_GROUP_SPECIFIC_TILES.at(0);

	for (unsigned int i = 0; i != TILESET_GROUP_SPECIFIC_TILES.size(); i++) {
		const _2Bytes tile_number = TILESET_GROUP_SPECIFIC_TILES.at(i);

		_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
		_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
		_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
		_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

		convert_to_file(fp, tile_number, tile1, tile2, tile3, tile4);

		if (i != TILESET_GROUP_SPECIFIC_TILES.size() - 1) {
			fprintf(fp, "\n");
		}

		try {
			curr_tile_it += _16x16_BYTE_SIZE * (TILESET_GROUP_SPECIFIC_TILES.at(i + 1) - TILESET_GROUP_SPECIFIC_TILES.at(i));
		}
		catch (const std::out_of_range&) {
			// do nothing, it's the last element and we got an out of range error trying to access the next one
		}
	}

	if (tileset_group_number == 0) {
		auto diag_pipe_it = map16_buffer.begin() + diagonal_pipes_offset;

		for (const _2Bytes tile_number : DIAGONAL_PIPE_TILES) {
			_2Bytes tile1 = join_bytes(diag_pipe_it, diag_pipe_it + 2);
			_2Bytes tile2 = join_bytes(diag_pipe_it + 2, diag_pipe_it + 4);
			_2Bytes tile3 = join_bytes(diag_pipe_it + 4, diag_pipe_it + 6);
			_2Bytes tile4 = join_bytes(diag_pipe_it + 6, diag_pipe_it + 8);

			convert_to_file(fp, tile_number, tile1, tile2, tile3, tile4);

			if (tile_number != DIAGONAL_PIPE_TILES.back()) {
				fprintf(fp, "\n");
			}

			diag_pipe_it += _16x16_BYTE_SIZE;
		}
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_tileset_specific_page_2(std::vector<Byte> map16_buffer, unsigned int tileset_number, size_t tiles_start_offset) {
	char filename[256];
	sprintf(filename, "tileset_specific_tiles\\tileset_%X.txt", tileset_number);
	FILE* fp = fopen(filename, "w");

	unsigned int base_tile_number = 0x200;

	auto curr_tile_it = map16_buffer.begin() + tiles_start_offset + PAGE_SIZE * _16x16_BYTE_SIZE * tileset_number;

	for (unsigned int i = 0; i != PAGE_SIZE; i++) {
		const _2Bytes tile_number = base_tile_number + i;

		_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
		_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
		_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
		_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

		convert_to_file(fp, tile_number, tile1, tile2, tile3, tile4);

		if (i != PAGE_SIZE - 1) {
			fprintf(fp, "\n");
		}

		curr_tile_it += _16x16_BYTE_SIZE;
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_normal_pipe_tiles(std::vector<Byte> map16_buffer, unsigned int pipe_number, size_t normal_pipe_offset) {
	char filename[256];
	sprintf(filename, "pipe_tiles\\pipe_%X.txt", pipe_number);
	FILE* fp = fopen(filename, "w");

	auto curr_tile_it = map16_buffer.begin() + normal_pipe_offset + _16x16_BYTE_SIZE * 8 * pipe_number;

	for (const auto tile_number : NORMAL_PIPE_TILES) {
		_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
		_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
		_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
		_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

		convert_to_file(fp, tile_number, tile1, tile2, tile3, tile4);

		if (tile_number != NORMAL_PIPE_TILES.back()) {
			fprintf(fp, "\n");
		}

		curr_tile_it += _16x16_BYTE_SIZE;
	}

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert_first_two_non_tileset_specific(std::vector<Byte> map16_buffer,
	size_t tileset_group_specific_offset, size_t acts_like_offset) {

	FILE* fp1 = fopen("global_pages\\FG_pages\\page_00.txt", "w");
	FILE* fp2 = fopen("global_pages\\FG_pages\\page_01.txt", "w");

	std::unordered_set<_2Bytes> tileset_group_specific = std::unordered_set<_2Bytes>(TILESET_GROUP_SPECIFIC_TILES.begin(), TILESET_GROUP_SPECIFIC_TILES.end());

	size_t offset_of_tileset_group_1_tiles = tileset_group_specific_offset + PAGE_SIZE * _16x16_BYTE_SIZE * 2;

	auto curr_tile_it = map16_buffer.begin() + offset_of_tileset_group_1_tiles;
	auto curr_acts_it = map16_buffer.begin() + acts_like_offset;

	for (unsigned int i = 0; i != PAGE_SIZE * 2; i++) {
		const _2Bytes tile_number = i;
		const _2Bytes acts_like = join_bytes(curr_acts_it, curr_acts_it + 2);

		FILE* fp = (i < 0x100) ? fp1 : fp2;

		if (tileset_group_specific.count(tile_number) == 0) {
			// is not tileset-group-specific

			_2Bytes tile1 = join_bytes(curr_tile_it, curr_tile_it + 2);
			_2Bytes tile2 = join_bytes(curr_tile_it + 2, curr_tile_it + 4);
			_2Bytes tile3 = join_bytes(curr_tile_it + 4, curr_tile_it + 6);
			_2Bytes tile4 = join_bytes(curr_tile_it + 6, curr_tile_it + 8);

			convert_to_file(fp, tile_number, acts_like, tile1, tile2, tile3, tile4);
		}
		else {
			convert_to_file(fp, tile_number, acts_like);
		}

		if (i != (PAGE_SIZE * 2) - 1) {
			fprintf(fp, "\n");
		}

		curr_tile_it += _16x16_BYTE_SIZE;
		curr_acts_it += ACTS_LIKE_SIZE;
	}

	fclose(fp1);
	fclose(fp2);
}

void HumanReadableMap16::from_map16::write_header_file(std::shared_ptr<Header> header, const fs::path header_path) {
	FILE* fp = fopen(header_path.string().c_str(), "w");

	fprintf(
		fp,
		"file_format_version_number: %X\n" \
		"game_id: %X\n" \
		"program_version: %X\n" \
		"program_id: %X\n" \
		"size_x: %X\n" \
		"size_y: %X\n" \
		"base_x: %X\n" \
		"base_y: %X\n" \
		"is_full_game_export: %X\n" \
		"has_tileset_specific_page_2: %X\n" \
		"comment_field: \"%s\"\n",
		header->file_format_version_number,
		header->game_id,
		header->program_version,
		header->program_id,
		header->size_x,
		header->size_y,
		header->base_x,
		header->base_y,
		is_full_game_export(header),
		has_tileset_specific_page_2s(header),
		header->comment.c_str()
	);

	fclose(fp);
}

void HumanReadableMap16::from_map16::convert(const fs::path input_file, const fs::path output_path) {
	if (!fs::exists(input_file)) {
		throw FilesystemError("Input map16 file does not exist", input_file);
	}

	if (!input_file.has_extension() || input_file.extension() != ".map16") {
		throw FilesystemError("Input file either has no extension or an extension different from .map16, are you sure this is a map16 file?", input_file);
	}

	if (fs::exists(output_path) && fs::is_directory(output_path) && !fs::is_empty(output_path)) {
		// output directory already exists, if it is not empty, we should check if there is a header file, if there is, it's probably 
		// save to overwrite, otherwise the user probably specified this directory in error and we should raise an exception!
		bool header_found = false;
		for (const auto& entry : fs::directory_iterator(output_path)) {
			if (entry.path().filename() == "header.txt") {
				header_found = true;
				break;
			}
		}
		if (!header_found) {
			throw FilesystemError("Pre-existing output directory is not empty and does not contain a header.txt file, are you sure this is the right directory to convert to?", output_path);
		}
	}

	std::vector<Byte> bytes = read_binary_file(input_file);
	auto header = get_header_from_map16_buffer(bytes);

	if (!is_full_game_export(header)) {
		throw HumanMap16Exception("Conversion to non-full-game-export map16 is not (yet?) supported");
	}

	try {

		fs::remove_all(output_path);
		fs::create_directory(output_path);
		fs::current_path(output_path);
		write_header_file(header, "header.txt");


		fs::create_directory("global_pages");
		fs::create_directory("global_pages\\FG_pages");
		fs::create_directory("global_pages\\BG_pages");
		fs::create_directory("tileset_group_specific_tiles");

		if (has_tileset_specific_page_2s(header)) {
			fs::create_directory("tileset_specific_tiles");
		}
		fs::create_directory("pipe_tiles");

	} catch (const fs::filesystem_error e) {
		throw FilesystemError("Encountered underlying file system error: " + std::string(e.what()), output_path);
	}

	const auto offset_size_table = get_offset_size_table(bytes, header);

	const auto& full_map16_pair = offset_size_table[0];
	const auto& full_acts_like_pair = offset_size_table[1];
	const auto& fg_map16_pair = offset_size_table[2];
	const auto& bg_map16_pair = offset_size_table[3];
	const auto& tileset_specific_page_2s_pair = offset_size_table[4];
	const auto& tileset_specific_first_two_pair = offset_size_table[5];
	const auto& normal_pipe_tiles = offset_size_table[6];
	const auto& diagonal_grassland_pipes = offset_size_table[7];

	convert_first_two_non_tileset_specific(bytes, tileset_specific_first_two_pair.first, full_acts_like_pair.first);

	unsigned int first_truly_global_page;
	if (has_tileset_specific_page_2s(header)) {
		first_truly_global_page = 3;
		convert_global_page_2_for_tileset_specific_page_2s(bytes, full_acts_like_pair.first);
	}
	else {
		first_truly_global_page = 2;
	}

	std::exception_ptr thread_exception{};
	auto page_numbers = boost::irange<int>(first_truly_global_page, 0x80);
	std::for_each(std::execution::par, page_numbers.begin(), page_numbers.end(), [&](auto&& page_number) {
		try {
			convert_FG_page(bytes, page_number, full_map16_pair.first, full_acts_like_pair.first);
		}
		catch (...) {
			thread_exception = std::current_exception();
		}
	});
	
	if (thread_exception != nullptr) {
		std::rethrow_exception(thread_exception);
	}

	page_numbers = boost::irange<int>(0x80, 0x100);
	std::for_each(std::execution::par, page_numbers.begin(), page_numbers.end(), [&](auto&& page_number) {
		try {
			convert_BG_page(bytes, page_number, full_map16_pair.first);
		}
		catch (...) {
			thread_exception = std::current_exception();
		}
	});

	if (thread_exception != nullptr) {
		std::rethrow_exception(thread_exception);
	}

	for (unsigned int tileset_group = 0; tileset_group != 5; tileset_group++) {
		convert_tileset_group_specific_pages(bytes, tileset_group, tileset_specific_first_two_pair.first, diagonal_grassland_pipes.first);
	}

	if (has_tileset_specific_page_2s(header)) {
		for (unsigned int tileset = 0; tileset != 0xF; tileset++) {
			convert_tileset_specific_page_2(bytes, tileset, tileset_specific_page_2s_pair.first);
		}
	}

	for (unsigned int pipe = 0; pipe != 0x4; pipe++) {
		convert_normal_pipe_tiles(bytes, pipe, normal_pipe_tiles.first);
	}
}
