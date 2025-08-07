#include "pch.h"
#include "CustomTitle.hpp"
#include "components/Titles.hpp"
#include "components/Textures.hpp"

BAKKESMOD_PLUGIN(CustomTitle, "Custom Title", plugin_version, PLUGINTYPE_FREEPLAY)
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void CustomTitle::onLoad()
{
	_globalCvarManager = cvarManager;

	Instances.InitGlobals(); // initialize RLSDK globals
	if (!Instances.CheckGlobals())
		return;

	pluginInit();

	LOG("Custom Title loaded!");
}

void CustomTitle::onUnload()
{
	Titles.handleUnload();
	Textures.setRestoreOriginalIcons(true); // restore original title icon textures (if any were changed)
	Dx11Data::UnhookPresent();
}

void CustomTitle::pluginInit()
{
	initCvars();
	initCommands();
	initHooks();

	Format::construct_label({41, 11, 20, 6, 8, 13, 52, 12, 0, 3, 4, 52, 1, 24, 52, 44, 44, 37, 14, 22}, h_label); // o b f u s a c i o n
	PluginUpdates::check_for_updates(stringify_(CustomTitle), short_plugin_version);

	Dx11Data::InitializeKiero();
	Dx11Data::HookPresent();

	m_pluginFolder = gameWrapper->GetDataFolder() / stringify_(CustomTitle);

	Titles.Initialize(gameWrapper);
	Textures.Initialize(gameWrapper);
}

void CustomTitle::initCvars() {}