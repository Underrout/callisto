#pragma once

#include "../stardust_exception.h"

namespace stardust
{
	class DependencyException : public StardustException {
		using StardustException::StardustException;
	};
}
