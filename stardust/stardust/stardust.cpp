#include <spdlog/spdlog.h>

#include "stardust.h"

#include "insertables/exgraphics.h"
#include "insertables/graphics.h"
#include "insertables/shared_palettes.h"

#include "stardust_exception.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	try {
		ExGraphics exgfx{ "./LunarMagic.exe", "./temp.smc", "./hack.smc" };
		Graphics gfx{ "./LunarMagic.exe", "./temp.smc", "./hack.smc" };
		SharedPalettes shared_palettes{ "./LunarMagic.exe", "./temp.smc", "./shared.pal" };

		exgfx.insert();
		gfx.insert();
		shared_palettes.insert();
	}
	catch (const StardustException& e) {
		spdlog::error(e.what());
	}
}
