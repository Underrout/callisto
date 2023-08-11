#pragma once

#include "../colors.h"

namespace callisto {
	class Extractable {
	public:
		virtual void extract() = 0;
	};
}
