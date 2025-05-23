#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"

#include <ModUtils/includes.hpp>
#include "Components/Includes.hpp"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
constexpr auto short_plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);


class CustomTitle: public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase
	,public PluginWindowBase
{
	void onLoad() override;
	void onUnload() override;


	std::string h_label;
	fs::path m_pluginFolder;
	void pluginInit();

	std::shared_ptr<bool> m_notifyWhenApplyingOthersTitle;

	// cvar helpers
	CVarWrapper RegisterCvar_Bool(const CvarData& cvar, bool startingValue);
	CVarWrapper RegisterCvar_String(const CvarData& cvar, const std::string& startingValue);
	CVarWrapper RegisterCvar_Number(const CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper RegisterCvar_Color(const CvarData& cvar, const std::string& startingValue);
	void RegisterCommand(const CvarData& cvar, std::function<void(std::vector<std::string>)> callback);
	CVarWrapper GetCvar(const CvarData& cvar);
	void RunCommand(const CvarData& command, float delaySeconds = 0);
	void RunCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);

public:
	bool handleIncomingChatMessage(const FChatMessage& message, AHUDBase_TA* caller);

	// commands
	void cmd_spawnCustomTitle(std::vector<std::string> args);
	void cmd_spawnItem(std::vector<std::string> args);
	void cmd_test(std::vector<std::string> args);
	void cmd_test2(std::vector<std::string> args);
	void cmd_test3(std::vector<std::string> args);
	
	// event callbacks
	void event_HUDBase_TA_OnChatMessage(ActorWrapper caller, void* params, std::string eventName);

public:
	// gui
	void RenderSettings() override;
	void RenderWindow() override;
	void TitlePresets_Tab();
	void TourneyIcons_Tab();
	void Settings_Tab();

	// header/footer stuff
	static constexpr float header_height = 80.0f;
	static constexpr float footer_height = 40.0f;
};
