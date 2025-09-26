#pragma once
#include "Cvars.hpp"
#include <ModUtils/wrappers/GFxWrapper.hpp>

template <typename Derived> class Component
{
protected:
	std::shared_ptr<GameWrapper> gameWrapper;

	template <typename... Args>
	static void LOG(std::string_view format_str, Args&&... args) // overload LOG function to add component name prefix
	{
		std::string strWithComponentName = std::format("[{}] {}", Derived::componentName, format_str);
		::LOG(strWithComponentName, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LOGERROR(std::string_view format_str, Args&&... args) // overload LOG function to add component name prefix
	{
		std::string strWithComponentName = std::format("[{}] ERROR: {}", Derived::componentName, format_str);
		::LOG(strWithComponentName, std::forward<Args>(args)...);
	}

	// hooks
	void hookEvent(const char* funcName, std::function<void(std::string eventName)> callback)
	{
		gameWrapper->HookEvent(funcName, callback);
		LOG("Hooked function pre: \"{}\"", funcName);
	}

	void hookEventPost(const char* funcName, std::function<void(std::string eventName)> callback)
	{
		gameWrapper->HookEventPost(funcName, callback);
		LOG("Hooked function post: \"{}\"", funcName);
	}

	void hookWithCaller(const char* function_name, std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
	{
		gameWrapper->HookEventWithCaller<ActorWrapper>(function_name, callback);
		LOG("Hooked function pre: \"{}\"", function_name);
	}

	void hookWithCallerPost(
	    const char* function_name, std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
	{
		gameWrapper->HookEventWithCallerPost<ActorWrapper>(function_name, callback);
		LOG("Hooked function post: \"{}\"", function_name);
	}

	// cvars
	CVarWrapper getCvar(const CvarData& cvar) { return _globalCvarManager->getCvar(cvar.name); }

	CVarWrapper registerCvar_bool(const CvarData& cvar, bool startingValue, bool log = true)
	{
		std::string value = startingValue ? "1" : "0";

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
	}

	CVarWrapper registerCvar_string(const CvarData& cvar, const std::string& startingValue, bool log = true)
	{
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	CVarWrapper registerCvar_number(const CvarData& cvar, float startingValue, bool hasMinMax, float min, float max, bool log = true)
	{
		std::string numberStr = std::to_string(startingValue);

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		if (hasMinMax)
		{
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
		}
		else
		{
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description);
		}
	}

	CVarWrapper registerCvar_color(const CvarData& cvar, const std::string& startingValue, bool log = true)
	{
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	// void hook_functions();
};