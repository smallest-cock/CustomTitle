#pragma once
#include "boilerplate/GuiBase.hpp"
#include "boilerplate/PluginHelperBase.hpp"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "Version.hpp"
#include <minwindef.h>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(
    VERSION_BUILD);

constexpr auto plugin_version_display = "v" VERSION_STR;

class CustomTitle : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase, public PluginWindowBase, public PluginHelperBase {
	void                                onLoad() override;
	void                                onUnload() override;
	BakkesMod::Plugin::BakkesModPlugin *getPlugin() override;

private:
	// init
	void pluginInit();
	void initCvars();
	void initCommands();
	void initHooks();

private:
	std::string h_label;
	fs::path    m_pluginFolder;

private:
	bool handleIncomingChatMessage(const FChatMessage &message, AHUDBase_TA *caller);

	// gui
public:
	void RenderSettings() override;
	void RenderWindow() override;

private:
	void TitlePresets_Tab();
	void TourneyIcons_Tab();
	void Settings_Tab();
	void Info_Tab();
};
