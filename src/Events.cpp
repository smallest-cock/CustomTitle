#include "pch.h"
#include "CustomTitle.h"
#include "Events.hpp"
#include "Components/Components/Titles.hpp"


void CustomTitle::initHooks()
{
	hookWithCallerPost(Events::HUDBase_TA_OnChatMessage, [this](ActorWrapper Caller, void* Params, ...)
	{
		auto params = reinterpret_cast<AHUDBase_TA_execOnChatMessage_Params*>(Params);
		if (!params)
			return;

		FChatMessage& msg = params->NewMsg;
		if (!msg.bPreset)
			return;

		auto caller = reinterpret_cast<AHUDBase_TA*>(Caller.memory_address);
		if (!caller)
			return;

		handleIncomingChatMessage(msg, caller);
	});
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