#pragma once

#include <string>
#include <filesystem>

#include <fmt/format.h>

#include "../insertable.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"

namespace fs = std::filesystem; 

namespace callisto {
	class RomInsertable : public Insertable {
	protected:
		const fs::path temporary_rom_path;

		RomInsertable(const Configuration& config) 
			: temporary_rom_path(registerConfigurationDependency(config.temporary_rom).getOrThrow()) {
			if (!fs::exists(temporary_rom_path)) {
				throw RomNotFoundException(fmt::format(
					"Temporary ROM not found at {}",
					temporary_rom_path.string()
				));
			}
		}
	};
}
