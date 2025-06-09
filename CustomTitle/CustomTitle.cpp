#include "pch.h"
#include "CustomTitle.h"


BAKKESMOD_PLUGIN(CustomTitle, "Custom Title", plugin_version, PLUGINTYPE_FREEPLAY)
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


void CustomTitle::onLoad()
{
	_globalCvarManager = cvarManager;

	Instances.InitGlobals(); // initialize RLSDK globals
	if (!Instances.CheckGlobals())
		return;

	pluginInit();

	// ================================== COMMANDS ======================================
	RegisterCommand(Commands::spawnCustomTitle,		std::bind(&CustomTitle::cmd_spawnCustomTitle, this, std::placeholders::_1));
	RegisterCommand(Commands::spawnItem,			std::bind(&CustomTitle::cmd_spawnItem, this, std::placeholders::_1));
	RegisterCommand(Commands::test,					std::bind(&CustomTitle::cmd_test, this, std::placeholders::_1));
	RegisterCommand(Commands::test2,				std::bind(&CustomTitle::cmd_test2, this, std::placeholders::_1));
	RegisterCommand(Commands::test3,				std::bind(&CustomTitle::cmd_test3, this, std::placeholders::_1));


	// ==================================== HOOKS =======================================
	// hooks with caller
	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::HUDBase_TA_OnChatMessage,
		std::bind(&CustomTitle::event_HUDBase_TA_OnChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


	LOG("Custom Title loaded!");
}

void CustomTitle::onUnload()
{
	Textures.setRestoreOriginalIcons(true); // restore original title icon textures (if any were changed)
	Dx11Data::UnhookPresent();
}

// Routes incoming chat data and calls appropriate function based on the contents
bool CustomTitle::handleIncomingChatMessage(const FChatMessage& message, AHUDBase_TA* caller)
{
	if (!caller)
		return false;

	std::string msgStr = message.Message.ToString();

	size_t separatorPos = msgStr.find(':');
	if (separatorPos == std::string::npos)
		return false;

	// Split prefix and content
	std::string prefix = msgStr.substr(0, separatorPos);
	std::string content = msgStr.substr(separatorPos + 1);

	// Route by prefix 
	// TODO: maybe perhaps possibly eventually make into an enum?
	if (prefix == "title")
	{
		Titles.applyPresetFromChatData(content, message, caller);
		return true;
	}
	//else if (prefix == "name")
	//{
	//	// Handle a "name" message type
	//}

	return false; // Unrecognized prefix
}

void CustomTitle::pluginInit()
{
	Format::construct_label({41, 11, 20, 6, 8, 13, 52, 12, 0, 3, 4, 52, 1, 24, 52, 44, 44, 37, 14, 22}, h_label);	// o b f u s a c i o n
	PluginUpdates::check_for_updates(stringify_(CustomTitle), short_plugin_version);

	Dx11Data::InitializeKiero();
	Dx11Data::HookPresent();

	m_pluginFolder = gameWrapper->GetDataFolder() / stringify_(CustomTitle);

	Titles.Initialize(gameWrapper);
	Textures.Initialize(gameWrapper);
}
