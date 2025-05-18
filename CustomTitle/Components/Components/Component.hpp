#pragma once
#include "Macros.hpp"
#include "Events.hpp"
#include "Cvars.hpp"
#include <ModUtils/wrappers/GFxWrapper.hpp>

template <typename Derived> class Component
{
  private:
  public:
    std::shared_ptr<GameWrapper> gameWrapper;
    // static constexpr std::string_view component_name = "Component";

    template <typename... Args>
    static void LOG(std::string_view format_str, Args&&... args) // overload LOG function to add component name prefix
    {
        std::string str_with_component_name = std::format("[{}] {}", Derived::component_name, format_str);
        ::LOG(std::vformat(str_with_component_name, std::make_format_args(args...)));
    }

    CVarWrapper getCvar(const CvarData& cvar) { return _globalCvarManager->getCvar(cvar.name); }

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

    void hookWithCaller(const char* function_name,
                       std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
    {
        gameWrapper->HookEventWithCaller<ActorWrapper>(function_name, callback);
        LOG("Hooked function pre: \"{}\"", function_name);
    }

    void hookWithCallerPost(const char* function_name,
                            std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
    {
        gameWrapper->HookEventWithCallerPost<ActorWrapper>(function_name, callback);
        LOG("Hooked function post: \"{}\"", function_name);
    }

    // void hook_functions();
};