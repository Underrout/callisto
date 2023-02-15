#pragma once

#include "stardust_exception.h"

namespace stardust
{
	class NotFoundException : public StardustException {
		using StardustException::StardustException;
	};

	class RomNotFoundException : public NotFoundException {
		using NotFoundException::NotFoundException;
	};

	class ResourceNotFoundException : public NotFoundException {
		using NotFoundException::NotFoundException;
	};

	class ToolNotFoundException : public NotFoundException {
		using NotFoundException::NotFoundException;
	};
}
