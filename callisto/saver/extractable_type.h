#pragma once

namespace callisto {
	enum class ExtractableType {
		BINARY_MAP16,
		TEXT_MAP16,
		CREDITS,
		GLOBAL_EX_ANIMATION,
		OVERWORLD,
		TITLE_SCREEN,
		LEVELS,
		SHARED_PALETTES,
		_COUNT,  // end marker
		
		GRAPHICS,
		EX_GRAPHICS
	};
}
