#pragma once

#include <string>
#include <unordered_set>

#include "dependency/dependency.h"

namespace stardust {
	class Insertable {
	protected:
		virtual std::unordered_set<Dependency> determineDependencies() = 0;
	public:
		virtual void insert() = 0;

		std::unordered_set<Dependency> insertWithDependencies() {
			insert();
			return determineDependencies();
		}
	};
}
