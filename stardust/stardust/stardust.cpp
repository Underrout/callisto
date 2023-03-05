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

#include "insertables/pixi.h"

#include "stardust_exception.h"

#include "configuration/configuration.h"
#include "configuration/configuration_level.h"
#include "configuration/config_exception.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	spdlog::set_level(spdlog::level::debug);

	getch();

	try {
		Configuration config{};

		{
			auto tom{ toml::parse("./config.toml") };
			auto tom2{ toml::parse("./config2.toml") };

			const auto vars1{ Configuration::parseUserVariables({ tom }) };

			for (const auto& [key, val] : vars1) {
				std::cout << key << " = " << val << std::endl;
			}


			for (const auto& [key, val] : Configuration::parseUserVariables({ tom2 }, vars1)) {
				std::cout << key << " = " << val << std::endl;
			}

			std::map<std::string, std::string> user_variables{
				{"lol", "stuff"},
				{"hella", "HELLA"},
				{"trans", "rights"},
				{"patches", "./PATCH3S"}
			};

			config.project_root.trySet(tom, ConfigurationLevel::PROJECT, ".", user_variables);
			config.rom_size.trySet(tom, ConfigurationLevel::PROJECT);
			config.config_name.trySet(tom, ConfigurationLevel::PROJECT, user_variables);
			config.patches.trySet(tom, ConfigurationLevel::PROJECT, ".", user_variables);
			config.patches.trySet(tom, ConfigurationLevel::PROFILE, ".", user_variables);
			config.build_order.trySet(tom, ConfigurationLevel::PROJECT, user_variables);
		}

		std::cout << config.project_root.getOrThrow().string() << std::endl;
		std::cout << config.config_name.getOrThrow() << std::endl;
		std::cout << config.rom_size.getOrThrow() << std::endl;

		for (const auto& patch : config.patches.getOrThrow()) {
			std::cout << patch << std::endl;
		}

		for (const auto& order_entry : config.build_order.getOrThrow()) {
			std::cout << order_entry << std::endl;
		}

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
		Pixi pixi{ "./", "./temp.smc", "-l ./list.txt -d" };
		ExternalTool uberasm{ "UberASM", fs::canonical("./uberasm/UberASMTool.exe"), "list.txt ../temp.smc" };
		ExternalTool addmusick{ "AddMusicK", fs::canonical("./addmusick/AddMusicK.exe"), "../temp.smc" };
		Patch patch{ "./", "./temp.smc", fs::canonical("./patch.asm") };

		exgfx.insert();
		gfx.insert();
		shared_palettes.insert();
		overworld.insert();
		title.insert();
		ex.insert();
		cred.insert();
		// title_moves.insert();
		level.insert();
		binary_map16.insert();
		text_map16.insert();
		pixi.insert();
		uberasm.insert();
		addmusick.insert();
		patch.insert();

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
