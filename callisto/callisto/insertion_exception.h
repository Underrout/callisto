#pragma once

#include "callisto_exception.h"

namespace callisto
{
	class InsertionException : public CallistoException {
		using CallistoException::CallistoException;
	};
}
