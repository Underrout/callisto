#include <spdlog/spdlog.h>

#include "stardust.h"

#include "insertables/exgraphics.h"
#include "insertables/graphics.h"
#include "insertables/shared_palettes.h"
#include "insertables/overworld.h"

#include "stardust_exception.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	try {
		ExGraphics exgfx{ "./LunarMagic.exe", "./temp.smc", "./hack.smc" };
		Graphics gfx{ "./LunarMagic.exe", "./temp.smc", "./hack.smc" };
		SharedPalettes shared_palettes{ "./LunarMagic.exe", "./temp.smc", "./shared.pal" };
		Overworld overworld{ "./LunarMagic.exe", "flips.exe", "./clean.smc", "./temp.smc", "./ow.bps" };

		exgfx.insert();
		gfx.insert();
		shared_palettes.insert();
		overworld.insert();
	}
	catch (const StardustException& e) {
		spdlog::error(e.what());
	}
}
