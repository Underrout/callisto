#pragma once

#include <optional>

#include <fmt/format.h>

#include "../stardust_exception.h"

namespace stardust {
	class Interval {
	public:
		const int pc_lower;
		const int pc_upper;

		const int snes_lower;
		const int snes_upper;

		const std::vector<char> bytes;

		// just assuming lorom, idk if that's an issue for sa-1 folks
		static int pcToSnes(int pc_address) {
			int snes_address{ ((pc_address << 1) & 0x7F0000) | (pc_address & 0x7FFF) | 0x8000 };
			return snes_address | 0x800000;
		}

		Interval(int pc_lower, int pc_upper, const std::vector<char>& bytes) 
			: pc_lower(pc_lower), pc_upper(pc_upper), 
			snes_lower(pcToSnes(pc_lower)), snes_upper(pcToSnes(pc_upper)), bytes(bytes) {

			if (pc_lower > pc_upper) {
				throw StardustException(fmt::format(
					"Invalid interval with PC offsets 0x{:06X} to 0x{:06X}",
					pc_lower, pc_upper
				));
			}
		}

		std::vector<char> slice(int pc_start, int pc_end) const {
			return std::vector<char>(bytes.begin() + (pc_start - pc_lower), bytes.begin() + (pc_end - pc_lower));
		}

		int length() const {
			return pc_upper - pc_lower;
		}

		bool overlaps(const Interval& other) const {
			return !((other.pc_lower < pc_lower && other.pc_upper <= pc_lower) ||
				(other.pc_lower >= pc_upper && other.pc_upper > pc_upper));
		}

		std::pair<std::optional<Interval>, std::optional<Interval>> subtract(const Interval& other) const {
			if (!overlaps(other)) {
				return { {*this}, {} };
			}

			if (other.pc_lower <= pc_lower && other.pc_upper >= pc_upper) {
				return { {}, {} };
			}

			if (pc_lower < other.pc_lower && pc_upper > other.pc_upper) {
				return { 
					Interval(pc_lower, other.pc_lower, slice(pc_lower, other.pc_lower)), 
					Interval(other.pc_upper, pc_upper, slice(other.pc_upper, pc_upper)) 
				};
			}

			if (pc_lower >= other.pc_lower) {
				return { Interval(other.pc_upper, pc_upper, slice(other.pc_upper, pc_upper)), {} };
			}
			else {
				return { Interval(pc_upper, other.pc_upper, slice(pc_upper, other.pc_upper)), {} };
			}
		}

		Interval overlap(const Interval& other) const {
			if (!overlaps(other)) {
				return Interval(0, 0, {});
			}
			else {
				const auto lower{ std::max(pc_lower, other.pc_lower) };
				const auto upper{ std::min(pc_upper, other.pc_upper) };
				return Interval(lower, upper, slice(lower, upper));
			}
		}

		bool operator==(const Interval& other) const {
			return pc_lower == other.pc_lower && pc_upper == other.pc_upper && bytes == other.bytes;
		}

		bool operator!=(const Interval& other) const {
			return !(*this == other);
		}
	};
}
