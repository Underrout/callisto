#include <spdlog/spdlog.h>

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

#include "insertables/pixi.h"

#include "stardust_exception.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	try {
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
	}
	catch (const StardustException& e) {
		spdlog::error(e.what());
	}
	catch (const std::runtime_error& e) {
		spdlog::error(e.what());
	}
}
