#include "pch.h"
#include "CustomTitle.hpp"

// cvars
CVarWrapper CustomTitle::registerCvar_Bool(const CvarData& cvar, bool startingValue)
{
	std::string value = startingValue ? "1" : "0";

	return cvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
}

CVarWrapper CustomTitle::registerCvar_String(const CvarData& cvar, const std::string& startingValue)
{
	return cvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

CVarWrapper CustomTitle::registerCvar_Number(const CvarData& cvar, float startingValue, bool hasMinMax, float min, float max)
{
	std::string numberStr = std::to_string(startingValue);

	if (hasMinMax)
		return cvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
	else
		return cvarManager->registerCvar(cvar.name, numberStr, cvar.description);
}

CVarWrapper CustomTitle::registerCvar_Color(const CvarData& cvar, const std::string& startingValue)
{
	return cvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

void CustomTitle::registerCommand(const CvarData& cvar, const std::function<void(std::vector<std::string>)>& callback)
{
	cvarManager->registerNotifier(cvar.name, callback, cvar.description, PERMISSION_ALL);
}

CVarWrapper CustomTitle::getCvar(const CvarData& cvar) { return cvarManager->getCvar(cvar.name); }

// commands
void CustomTitle::runCommand(const CvarData& command, float delaySeconds)
{
	if (delaySeconds == 0)
		cvarManager->executeCommand(command.name);
	else if (delaySeconds > 0)
		gameWrapper->SetTimeout([this, command](GameWrapper* gw) { cvarManager->executeCommand(command.name); }, delaySeconds);
}

void CustomTitle::runCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand)
{
	if (!delayFirstCommand)
	{
		runCommand(command);
		numIntervals--;
	}

	for (int i = 1; i <= numIntervals; i++)
	{
		runCommand(command, delaySeconds * i);
	}
}

void CustomTitle::autoRunCommand(const CvarData& autoRunBool, const CvarData& command, float delaySeconds)
{
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommand(command, delaySeconds);
}

void CustomTitle::autoRunCommandInterval(
    const CvarData& autoRunBool, const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand)
{
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommandInterval(command, numIntervals, delaySeconds, delayFirstCommand);
}
