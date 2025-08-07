#pragma once

#define CVAR(name, desc) CvarData("customtitle_" name, desc) // automatically apply the prefix to cvar names

struct CvarData
{
	const char* name;
	const char* description;

	constexpr CvarData(const char* name, const char* description) : name(name), description(description) {}
};

namespace Cvars
{
// bools
constexpr CvarData useCustomTitle           = CVAR("use_custom_title", "use custom title");
constexpr CvarData showOtherPlayerTitles    = CVAR("show_other_player_titles", "show other players' custom titles");
constexpr CvarData showTitleToOthers        = CVAR("show_title_to_others", "show current title to other players with the same mod");
constexpr CvarData filterOtherPlayerTitles  = CVAR("filter_other_player_titles", "use word filter on other players' custom titles");
constexpr CvarData applyOthersTitleNotif    = CVAR("apply_others_title_notification", "show noti when applying another player's title");
constexpr CvarData useHueColorPicker        = CVAR("use_hue_color_picker", "use HSV hue color wheel triangle illuminati thing");
constexpr CvarData showEquippedTitleDetails = CVAR("show_equipped_title_details", "show extra details about the equipped title");

// strings

// colors
} // namespace Cvars

namespace Commands
{
constexpr CvarData spawnCustomTitle = CVAR("spawn_custom_title", "spawn selected title preset");
constexpr CvarData spawnItem        = CVAR("spawn_item", "spawn an item based on the product ID");
constexpr CvarData test             = CVAR("test", "test");
constexpr CvarData test2            = CVAR("test2", "test 2");
constexpr CvarData test3            = CVAR("test3", "test 3");
} // namespace Commands