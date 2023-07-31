#pragma once

#include "../callisto_exception.h"

namespace callisto {
	class MustRebuildException : public CallistoException {
	public:
		using CallistoException::CallistoException;
	};
}
