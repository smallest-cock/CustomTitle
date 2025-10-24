#pragma once
#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "version.h"
#include "Cvars.hpp"
#include <minwindef.h>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(
    VERSION_BUILD);

constexpr auto plugin_version_display = "v" VERSION_STR;

class CustomTitle : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase, public PluginWindowBase
{
	void onLoad() override;
	void onUnload() override;

	// init
	void pluginInit();
	void initCvars();
	void initCommands();
	void initHooks();

private:
	// plugin boilerplate helper stuff
	CVarWrapper registerCvar_Bool(const CvarData& cvar, bool startingValue);
	CVarWrapper registerCvar_String(const CvarData& cvar, const std::string& startingValue);
	CVarWrapper registerCvar_Number(const CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper registerCvar_Color(const CvarData& cvar, const std::string& startingValue);
	void        registerCommand(const CvarData& cvar, const std::function<void(std::vector<std::string>)>& callback);
	CVarWrapper getCvar(const CvarData& cvar);

	void runCommand(const CvarData& command, float delaySeconds = 0.0f);
	void autoRunCommand(const CvarData& autoRunBool, const CvarData& command, float delaySeconds = 0.0f);
	void runCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);
	void autoRunCommandInterval(
	    const CvarData& autoRunBool, const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);

private:
	std::string h_label;
	fs::path    m_pluginFolder;

private:
	bool handleIncomingChatMessage(const FChatMessage& message, AHUDBase_TA* caller);

	// gui
private:
	void TitlePresets_Tab();
	void TourneyIcons_Tab();
	void Settings_Tab();
	void Info_Tab();

	// header/footer stuff
	static constexpr float header_height = 80.0f;
	static constexpr float footer_height = 40.0f;

public:
	void RenderSettings() override;
	void RenderWindow() override;
};
