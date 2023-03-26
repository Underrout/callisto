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

#include "builders/rebuilder.h"
#include "builders/quick_builder.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	spdlog::set_level(spdlog::level::info);

	getch();

	try {
		ConfigurationManager config_manager{ fs::current_path() };

		const auto config{ config_manager.getConfiguration({}) };

		const auto report{ PathUtil::getBuildReportPath(config.project_root.getOrThrow()) };

		if (fs::exists(report)) {
			std::ifstream file{ report };
			QuickBuilder quick_builder{ json::parse(file) };
			quick_builder.build(config);
		}
		else {
			Rebuilder rebuilder{};
			rebuilder.build(config);
		}

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
	catch (const json::exception& e) {
		spdlog::error(e.what());
		return 1;
	}
}
