#include "rebuilder.h"

namespace stardust {
	void Rebuilder::build(const Configuration& config) {
		for (auto& [descriptor, insertable] : buildOrderToInsertables(config)) {
			insertable->insertWithDependencies();
		}
	}
}
