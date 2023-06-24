#pragma once

namespace callisto
{
	class CallistoException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
}
