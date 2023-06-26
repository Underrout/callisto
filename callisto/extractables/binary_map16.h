#pragma once

#include <filesystem>

#include <fmt/format.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"

namespace fs = std::filesystem;

namespace callisto {
	namespace extractables {
		class BinaryMap16 : public LunarMagicExtractable {
		protected:
			const fs::path map16_file_path;

		public:
			void extract() override;

			BinaryMap16(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
