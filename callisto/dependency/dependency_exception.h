#pragma once

#include "../callisto_exception.h"

namespace callisto
{
	class DependencyException : public CallistoException {
		using CallistoException::CallistoException;
	};
}
