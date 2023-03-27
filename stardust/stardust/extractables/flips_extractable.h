#pragma once

#include <fmt/format.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"

namespace stardust {
	class FlipsExtractable : public LunarMagicExtractable {
	protected:
		const fs::path flips_executable;
		const fs::path clean_rom_path;
		const fs::path output_patch_path;

		virtual inline std::string getTemporaryResourceRomPostfix() const = 0;
		virtual inline std::string getLunarMagicFlag() const = 0;
		virtual inline std::string getResourceName() const = 0;

		fs::path getTemporaryResourceRomPath() const;

		void createTemporaryResourceRom(const fs::path& temporary_resource_rom) const;
		void invokeLunarMagic(const fs::path& temporary_resource_rom) const;
		void createOutputPatch(const fs::path& temporary_resource_rom) const;
		void deleteTemporaryResourceRom(const fs::path& temporary_resource_rom) const;

	public:
		FlipsExtractable(const Configuration& config, const fs::path& output_patch_path);

		void extract() override;
	};
}
