#include <spdlog/spdlog.h>
#include <toml.hpp>

#include "ftxui/component/component.hpp"  // for Checkbox, Renderer, Horizontal, Vertical, Input, Menu, Radiobox, ResizableSplitLeft, Tab
#include "ftxui/component/component_base.hpp"  // for ComponentBase, Component
#include "ftxui/component/component_options.hpp"  // for MenuOption, InputOption
#include "ftxui/component/event.hpp"              // for Event, Event::Custom
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component_options.hpp"   // for MenuEntryOption
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for operator|, Element, separator, text, hbox, size, frame, color, vbox, HEIGHT, LESS_THAN, bold, border, inverted
#include "ftxui/screen/color.hpp"

#include "stardust.h"

#include "stardust_exception.h"

#include "configuration/configuration.h"
#include "configuration/configuration_level.h"
#include "configuration/config_exception.h"
#include "configuration/configuration_manager.h"

#include "extractables/credits.h"
#include "extractables/overworld.h"
#include "extractables/global_exanimation.h"
#include "extractables/title_screen.h"
#include "extractables/shared_palettes.h"
#include "extractables/binary_map16.h"
#include "extractables/text_map16.h"
#include "extractables/level.h"
#include "extractables/levels.h"

#include "builders/rebuilder.h"
#include "builders/quick_builder.h"

#include "saver/saver.h"

#include "emulators/emulators.h"

#include "tui/tui.h"

using namespace stardust;

int main(int argc, const char* argv[]) {
	TUI tui{ fs::path(argv[0]).parent_path() };
	tui.run();
}
