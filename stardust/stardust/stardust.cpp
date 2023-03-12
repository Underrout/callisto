#include <spdlog/spdlog.h>
#include <toml.hpp>

#include <conio.h>

#include "stardust.h"

#include "insertables/exgraphics.h"
#include "insertables/graphics.h"
#include "insertables/shared_palettes.h"
#include "insertables/overworld.h"
#include "insertables/title_screen.h"
#include "insertables/global_exanimation.h"
#include "insertables/credits.h"
#include "insertables/title_moves.h"
#include "insertables/level.h"
#include "insertables/binary_map16.h"
#include "insertables/text_map16.h"
#include "insertables/external_tool.h"
#include "insertables/patch.h"
#include "insertables/globule.h"

#include "insertables/pixi.h"

#include "stardust_exception.h"

#include "configuration/configuration.h"
#include "configuration/configuration_level.h"
#include "configuration/config_exception.h"
#include "configuration/configuration_manager.h"
#include "dependency/dependency.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	spdlog::set_level(spdlog::level::debug);

	getch();

	try {
		ConfigurationManager config_manager{ fs::current_path() };

		const auto config{ config_manager.getConfiguration({}) };

		ExGraphics exgfx{"./LunarMagic.exe", "./temp.smc", "./hack.smc"};
		Graphics gfx{ "./LunarMagic.exe", "./temp.smc", "./hack.smc" };
		SharedPalettes shared_palettes{ "./LunarMagic.exe", "./temp.smc", "./shared.pal" };
		Overworld overworld{ "./flips.exe", "./clean.smc", "./LunarMagic.exe", "./temp.smc", "./ow.bps" };
		TitleScreen title{ "./flips.exe", "./clean.smc", "./LunarMagic.exe", "./temp.smc", "./ow.bps" };
		Credits cred{ "./flips.exe", "./clean.smc", "./LunarMagic.exe", "./temp.smc", "./ow.bps" };
		GlobalExAnimation ex{ "./flips.exe", "./clean.smc", "./LunarMagic.exe", "./temp.smc", "./ow.bps" };
		TitleMoves title_moves{ "./LunarMagic.exe", "./temp.smc", "./title.zst" };
		Level level{ "./LunarMagic.exe", "./temp.smc", "./level.mwl" };
		BinaryMap16 binary_map16{ "./LunarMagic.exe", "./temp.smc", "./all.map16" };
		TextMap16 text_map16{ "./LunarMagic.exe", "./temp.smc", "./map16_folder", "./cli.exe" };


		Pixi pixi{ "./", "./temp.smc", "-l ./list.txt -d", config.pixi_static_dependencies.getOrThrow(), config.pixi_dependency_report_file.getOrThrow()};

		ExternalTool uberasm{ "UberASM", fs::canonical("./uberasm/UberASMTool.exe"), "list.txt", "./", "./temp.smc", false,
			config.generic_tool_configurations.at("UberASM").static_dependencies.getOrThrow(),
			config.generic_tool_configurations.at("UberASM").dependency_report_file.getOrThrow()
		};

		ExternalTool addmusick{ "AddMusicK", fs::canonical("./addmusick/AddMusicK.exe"), "", "./addmusick", "../temp.smc", false, 
			config.generic_tool_configurations.at("AddMusicK").static_dependencies.getOrThrow(),
			config.generic_tool_configurations.at("AddMusicK").dependency_report_file.getOrThrow() };
		Patch patch{ "./", "./temp.smc", fs::canonical("./patch.asm") };
		Globule globule{ "./", "./temp.smc", fs::canonical("./globule.asm"), 
			fs::canonical("./imprints"), fs::canonical("./call.asm"), {"Data", "cod"}, {}};

		exgfx.insertWithDependencies();
		gfx.insertWithDependencies();
		shared_palettes.insertWithDependencies();
		overworld.insertWithDependencies();
		title.insertWithDependencies();
		ex.insertWithDependencies();
		cred.insertWithDependencies();
		// title_moves.insert();
		level.insertWithDependencies();
		binary_map16.insertWithDependencies();
		text_map16.insertWithDependencies();
		pixi.insertWithDependencies();
		globule.insertWithDependencies();
		uberasm.insertWithDependencies();
		addmusick.insertWithDependencies();
		patch.insertWithDependencies();

		return 0;
	}
	catch (const TomlException2& e) {
		spdlog::error(toml::format_error(e.what(), e.toml_value, e.comment, e.toml_value2, e.comment2, e.hints));
	}
	catch (const TomlException& e) {
		spdlog::error(toml::format_error(e.what(), e.toml_value, e.comment, e.hints));
	}
	catch (const toml::syntax_error& e) {
		spdlog::error(e.what());
	}
	catch (const toml::type_error& e) {
		spdlog::error(e.what());
	}
	catch (const toml::internal_error& e) {
		spdlog::error(e.what());
	}
	catch (const StardustException& e) {
		spdlog::error(e.what());
		return 1;
	}
	catch (const std::runtime_error& e) {
		spdlog::error(e.what());
		return 1;
	}
}
