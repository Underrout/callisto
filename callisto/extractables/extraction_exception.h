#pragma once

#include "../callisto_exception.h"

namespace callisto {
	class ExtractionException : public CallistoException {
	public:
		using CallistoException::CallistoException;
	};
}
