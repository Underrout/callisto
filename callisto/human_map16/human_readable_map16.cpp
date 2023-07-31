#include "human_readable_map16.h"

bool HumanReadableMap16::has_tileset_specific_page_2s(std::shared_ptr<HumanReadableMap16::Header> header) {
	return header->various_flags_and_info & 1;
}

bool HumanReadableMap16::is_full_game_export(std::shared_ptr<HumanReadableMap16::Header> header) {
	return header->various_flags_and_info & 2;
}
