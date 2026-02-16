#include "pch.h"
#include "CustomTitle.hpp"
#include "PluginConfig.hpp"
#include "util/Instances.hpp"
#include "components/Titles.hpp"
#include "components/Textures.hpp"
#include "Version.hpp"

BAKKESMOD_PLUGIN(CustomTitle, "Custom Title", plugin_version, PLUGINTYPE_FREEPLAY)
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void CustomTitle::onLoad() {
	_globalCvarManager = cvarManager;

	if (!Instances.initGlobals())
		return;

	pluginInit();

	LOG(PLUGIN_NAME " loaded!");
}

void CustomTitle::onUnload() {
	Textures.setRestoreOriginalIcons(true); // restore original title icon textures (if any were changed)
	Titles.handleUnload();
	Dx11Data::UnhookPresent();
}

BakkesMod::Plugin::BakkesModPlugin *CustomTitle::getPlugin() { return this; }

void CustomTitle::pluginInit() {
	g_hookManager.init(gameWrapper);

	initCvars();
	initCommands();
	initHooks();

	Format::construct_label({41, 11, 20, 6, 8, 13, 52, 12, 0, 3, 4, 52, 1, 24, 52, 44, 44, 37, 14, 22}, h_label); // o b f u s a c i o n
	PluginUpdates::checkForUpdates(PLUGIN_NAME_NO_SPACES, VERSION_STR);

	Dx11Data::InitializeKiero();
	Dx11Data::HookPresent();

	m_pluginFolder = gameWrapper->GetDataFolder() / PLUGIN_NAME_NO_SPACES;

	Titles.init(gameWrapper);
	Textures.init(gameWrapper);

	g_hookManager.commitHooks();

	UFunction::FindFunction("Dummy to trigger function cache");
}

void CustomTitle::initCvars() {}
