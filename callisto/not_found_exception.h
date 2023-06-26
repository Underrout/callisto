#pragma once

#include "callisto_exception.h"

namespace callisto
{
	class NotFoundException : public CallistoException {
		using CallistoException::CallistoException;
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
