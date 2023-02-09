#pragma once

namespace stardust
{
	class StardustException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
}
