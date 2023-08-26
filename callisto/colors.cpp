#include "colors.h"

namespace callisto {
	namespace colors {

		fmt::text_style ACTION_START = DEFAULT_ACTION_START;
		fmt::text_style RESOURCE = DEFAULT_RESOURCE;
		fmt::text_style SUCCESS = DEFAULT_SUCCESS;
		fmt::text_style PARTIAL_SUCCESS = DEFAULT_PARTIAL_SUCCESS;
		fmt::text_style NOTIFICATION = DEFAULT_NOTIFICATION;
		fmt::text_style EXCEPTION = DEFAULT_EXCEPTION;
		fmt::text_style WARNING = DEFAULT_WARNING;
		fmt::text_style CALLISTO = DEFAULT_CALLISTO;

		fmt::text_style& configurableColorStringToStyle(const std::string& style_name) {
			if (style_name == "action_start") {
				return ACTION_START;
			}
			else if (style_name == "resource") {
				return RESOURCE;
			}
			else if (style_name == "success") {
				return SUCCESS;
			}
			else if (style_name == "partial_success") {
				return PARTIAL_SUCCESS;
			}
			else if (style_name == "notification") {
				return NOTIFICATION;
			}
			else if (style_name == "exception") {
				return EXCEPTION;
			}
			else if (style_name == "warning") {
				return WARNING;
			}
			else if (style_name == "callisto") {
				return CALLISTO;
			}
		}

		void resetStyles() {
			ACTION_START = DEFAULT_ACTION_START;
			RESOURCE = DEFAULT_RESOURCE;
			SUCCESS = DEFAULT_SUCCESS;
			PARTIAL_SUCCESS = DEFAULT_PARTIAL_SUCCESS;
			NOTIFICATION = DEFAULT_NOTIFICATION;
			EXCEPTION = DEFAULT_EXCEPTION;
			WARNING = DEFAULT_WARNING;
			CALLISTO = DEFAULT_CALLISTO;
		}

		std::optional<fmt::emphasis> stringToEmphasis(const std::string& str) {
			const auto found{ std::find(emphasis_names.begin(), emphasis_names.end(), str) };
			if (found == emphasis_names.end()) {
				return {};
			}
			const auto idx{ std::distance(emphasis_names.begin(), found) };

			return static_cast<fmt::emphasis>(1 << idx);
		}

		fmt::color getColor(uint32_t rgb) {
			if (rgb > 0xFFFFFF) {
				throw CallistoException(fmt::format(
					"0x{:06X} is not a valid RGB color and thus cannot be used, sorry ;_;",
					rgb
				));
			}

			return static_cast<fmt::color>(rgb);
		}

		void addFgColor(fmt::text_style& style, uint32_t rgb) {
			style |= fmt::fg(getColor(rgb));
		}

		void addBgColor(fmt::text_style& style, uint32_t rgb) {
			style |= fmt::bg(getColor(rgb));
		}

		void addEmphasis(fmt::text_style& style, const std::string& emphasis_string) {
			const auto emphasis{ stringToEmphasis(emphasis_string) };
			if (!emphasis.has_value()) {
				throw CallistoException(fmt::format(
					"'{}' is not a valid emphasis type and thus cannot be used, sorry ;_;",
					emphasis_string
				));
			}

			style |= emphasis.value();
		}

		void resetStyle(fmt::text_style& style) {
			style = fmt::fg(fmt::color::black);
		}

		std::array<std::string, 8> configurable_colors = {
			"action_start",
			"resource",
			"success",
			"partial_success",
			"notification",
			"exception",
			"warning",
			"callisto"
		};

		std::array<std::string, 8> emphasis_names = {
		  "bold",
		  "faint",
		  "italic",
		  "underline",
		  "blink",
		  "reverse",
		  "conceal",
		  "strikethrough"
		};

		std::array<std::string, 141> named_html_colors = {
			"alice_blue",
			"antique_white",
			"aqua",
			"aquamarine",
			"azure",
			"beige",
			"bisque",
			"black",
			"blanched_almond",
			"blue",
			"blue_violet",
			"brown",
			"burly_wood",
			"cadet_blue",
			"chartreuse",
			"chocolate",
			"coral",
			"cornflower_blue",
			"cornsilk",
			"crimson",
			"cyan",
			"dark_blue",
			"dark_cyan",
			"dark_golden_rod",
			"dark_gray",
			"dark_green",
			"dark_khaki",
			"dark_magenta",
			"dark_olive_green",
			"dark_orange",
			"dark_orchid",
			"dark_red",
			"dark_salmon",
			"dark_sea_green",
			"dark_slate_blue",
			"dark_slate_gray",
			"dark_turquoise",
			"dark_violet",
			"deep_pink",
			"deep_sky_blue",
			"dim_gray",
			"dodger_blue",
			"fire_brick",
			"floral_white",
			"forest_green",
			"fuchsia",
			"gainsboro",
			"ghost_white",
			"gold",
			"golden_rod",
			"gray",
			"green",
			"green_yellow",
			"honey_dew",
			"hot_pink",
			"indian_red",
			"indigo",
			"ivory",
			"khaki",
			"lavender",
			"lavender_blush",
			"lawn_green",
			"lemon_chiffon",
			"light_blue",
			"light_coral",
			"light_cyan",
			"light_golden_rod_yellow",
			"light_gray",
			"light_green",
			"light_pink",
			"light_salmon",
			"light_sea_green",
			"light_sky_blue",
			"light_slate_gray",
			"light_steel_blue",
			"light_yellow",
			"lime",
			"lime_green",
			"linen",
			"magenta",
			"maroon",
			"medium_aquamarine",
			"medium_blue",
			"medium_orchid",
			"medium_purple",
			"medium_sea_green",
			"medium_slate_blue",
			"medium_spring_green",
			"medium_turquoise",
			"medium_violet_red",
			"midnight_blue",
			"mint_cream",
			"misty_rose",
			"moccasin",
			"navajo_white",
			"navy",
			"old_lace",
			"olive",
			"olive_drab",
			"orange",
			"orange_red",
			"orchid",
			"pale_golden_rod",
			"pale_green",
			"pale_turquoise",
			"pale_violet_red",
			"papaya_whip",
			"peach_puff",
			"peru",
			"pink",
			"plum",
			"powder_blue",
			"purple",
			"rebecca_purple",
			"red",
			"rosy_brown",
			"royal_blue",
			"saddle_brown",
			"salmon",
			"sandy_brown",
			"sea_green",
			"sea_shell",
			"sienna",
			"silver",
			"sky_blue",
			"slate_blue",
			"slate_gray",
			"snow",
			"spring_green",
			"steel_blue",
			"tan",
			"teal",
			"thistle",
			"tomato",
			"turquoise",
			"violet",
			"wheat",
			"white",
			"white_smoke",
			"yellow",
			"yellow_green"
		};
	}
}
