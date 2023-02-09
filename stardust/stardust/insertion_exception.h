#pragma once

#include "stardust_exception.h"

namespace stardust
{
	class InsertionException : public StardustException
	{
		using StardustException::StardustException;
	};
}
