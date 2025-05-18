#include "pch.h"
#include "CustomTitle.h"


void CustomTitle::event_HUDBase_TA_OnChatMessage(ActorWrapper Caller, void* Params, std::string eventName)
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
}
