#pragma once

#include "../stardust_exception.h"

namespace stardust {
	class ExtractionException : public StardustException {
	public:
		using StardustException::StardustException;
	};
}
