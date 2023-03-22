#pragma once

#include <fmt/format.h>

#include "../stardust_exception.h"

namespace stardust {
	class Interval {
	public:
		const int lower;
		const int upper;

		Interval(int lower, int upper) : lower(lower), upper(upper) {
			if (lower > upper) {
				throw StardustException(fmt::format(
					"Invalid interval with lower={} and upper={}",
					lower, upper
				));
			}
		}

		int length() const {
			return upper - lower;
		}

		bool overlaps(const Interval& other) const {
			return !((other.lower < lower && other.upper <= lower) ||
				(other.lower >= upper && other.upper > upper));
		}

		Interval overlap(const Interval& other) const {
			if (!overlaps(other)) {
				return Interval(0, 0);
			}
			else {
				return Interval(std::max({ lower, other.lower }), std::min(upper, other.upper));
			}
		}
	};
}
