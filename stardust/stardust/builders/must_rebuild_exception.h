#pragma once

#include "../stardust_exception.h"

namespace stardust {
	class MustRebuildException : public StardustException {
	public:
		using StardustException::StardustException;
	};
}
