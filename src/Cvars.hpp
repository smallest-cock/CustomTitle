#pragma once

#define CVAR(name, desc)                                                                                                                   \
	CvarData { "customtitle_" name, desc }

struct CvarData {
	const char *name;
	const char *description;
};

namespace Cvars {
	// bools
	constexpr auto enabled                  = CVAR("enabled", "enable/disable custom title");
	constexpr auto showOtherPlayerTitles    = CVAR("show_other_player_titles", "show other players' custom titles");
	constexpr auto showTitleToOthers        = CVAR("show_title_to_others", "show current title to other players with the same mod");
	constexpr auto filterOtherPlayerTitles  = CVAR("filter_other_player_titles", "use word filter on other players' custom titles");
	constexpr auto applyOthersTitleNotif    = CVAR("apply_others_title_notification", "show noti when applying another player's title");
	constexpr auto useHueColorPicker        = CVAR("use_hue_color_picker", "use HSV hue color wheel triangle illuminati thing");
	constexpr auto showEquippedTitleDetails = CVAR("show_equipped_title_details", "show extra details about the equipped title");
	constexpr auto applyUserPresetFromChat  = CVAR(
        "apply_user_preset_from_chat", "Applies user's own preset after sending a title broadcast chat");

	// numbers
	constexpr auto rgbSpeed             = CVAR("rgb_speed", "RGB color cycle speed");
	constexpr auto broadcastChatTimeout = CVAR("broadcast_chat_timeout", "The cooldown duration between title broadcast chats");
}

namespace Commands {
	constexpr auto toggleEnabled    = CVAR("toggle_enabled", "toggle enabling/disabling custom title");
	constexpr auto broadcast        = CVAR("broadcast", "Broadcast your custom title to others with the plugin (via the chat system)");
	constexpr auto spawnCustomTitle = CVAR("spawn_custom_title", "spawn selected title preset");
	constexpr auto spawnItem        = CVAR("spawn_item", "spawn an item based on the product ID");
	constexpr auto test             = CVAR("test", "test");
	constexpr auto test2            = CVAR("test2", "test 2");
	constexpr auto test3            = CVAR("test3", "test 3");
}
