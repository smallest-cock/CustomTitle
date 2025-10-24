#include "pch.h"
#include <libloaderapi.h>
#include "Instances.hpp"

InstancesComponent::InstancesComponent() { onCreate(); }

InstancesComponent::~InstancesComponent() { onDestroy(); }

void InstancesComponent::onCreate()
{
	I_UCanvas             = nullptr;
	I_AHUD                = nullptr;
	I_UGameViewportClient = nullptr;
	I_APlayerController   = nullptr;
}

void InstancesComponent::onDestroy()
{
	m_staticClasses.clear();
	m_staticFunctions.clear();

	for (UObject* uObject : m_createdObjects)
	{
		if (!uObject)
			continue;

		markForDestroy(uObject);
	}

	m_createdObjects.clear();
}

// ========================================= init RLSDK globals ===========================================

constexpr auto MODULE_NAME = L"RocketLeague.exe";

uintptr_t InstancesComponent::findPattern(HMODULE module, const unsigned char* pattern, const char* mask)
{
	MODULEINFO info = {};
	GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

	uintptr_t start  = reinterpret_cast<uintptr_t>(module);
	size_t    length = info.SizeOfImage;

	size_t pos        = 0;
	size_t maskLength = std::strlen(mask) - 1;

	for (uintptr_t retAddress = start; retAddress < start + length; retAddress++)
	{
		if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?')
		{
			if (pos == maskLength)
				return (retAddress - maskLength);
			pos++;
		}
		else
		{
			retAddress -= pos;
			pos = 0;
		}
	}
	return NULL;
}

uintptr_t InstancesComponent::findGNamesAddress()
{
	unsigned char GNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
	char          GNamesMask[]    = "??????xx??xxxxxx";

	auto GNamesAddress = findPattern(GetModuleHandle(MODULE_NAME), GNamesPattern, GNamesMask);

	return GNamesAddress;
}

uintptr_t InstancesComponent::findGObjectsAddress() { return findGNamesAddress() + 0x48; }

uintptr_t InstancesComponent::findGMallocAddr()
{
	constexpr uint8_t pattern[] = "\x48\x89\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x50\x60";
	constexpr auto    mask      = "xxx????xxxxxx";

	uintptr_t foundAddr = findPattern(GetModuleHandle(MODULE_NAME), pattern, mask);
	if (!foundAddr)
	{
		LOGERROR("We are returning NULL for GMalloc address...");
		return NULL;
	}

	// Extract the 32-bit displacement offset from the instruction
	int32_t displacement = *reinterpret_cast<int32_t*>(foundAddr + 3);

	// Calculate address using RIP-relative formula
	uintptr_t gMallocAddr = foundAddr + 7 + displacement;
	return gMallocAddr;
}

bool InstancesComponent::initGlobals()
{
	uintptr_t gnamesAddr = findGNamesAddress();
	GNames               = reinterpret_cast<TArray<FNameEntry*>*>(gnamesAddr);
	GObjects             = reinterpret_cast<TArray<UObject*>*>(gnamesAddr + 0x48);

	uintptr_t gmallocAddr = findGMallocAddr();
	if (!gmallocAddr)
	{
		LOGERROR("Failed to find GMalloc address via pattern scan");
		return false;
	}
	GMalloc = gmallocAddr;

	return checkGlobals();
}

bool InstancesComponent::areGObjectsValid()
{
	if (UObject::GObjObjects()->size() > 0 && UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size())
	{
		if (UObject::GObjObjects()->at(0)->GetFullName() == "Class Core.Config_ORS")
			return true;
	}
	return false;
}

bool InstancesComponent::areGNamesValid()
{
	if (FName::Names()->size() > 0 && FName::Names()->capacity() > FName::Names()->size())
	{
		if (FName(0).ToString() == "None")
			return true;
	}
	return false;
}

bool InstancesComponent::checkGlobals()
{
	bool gnamesValid   = GNames && areGNamesValid();
	bool gobjectsValid = GObjects && areGObjectsValid();
	if (!gnamesValid || !gobjectsValid)
	{
		LOG("(onLoad) Error: RLSDK classes are wrong... plugin needs an update :(");
		LOG(std::format("GNames valid: {} -- GObjects valid: {}", gnamesValid, gobjectsValid));
		return false;
	}

	LOG("Globals Initialized :)");
	return true;
}

// ===========================================================================================================

class UClass* InstancesComponent::findStaticClass(const std::string& className)
{
	if (m_staticClasses.empty())
	{
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++)
		{
			UObject* uObject = UObject::GObjObjects()->at(i);

			if (uObject)
			{
				if ((uObject->GetFullName().find("Class") == 0))
				{
					m_staticClasses[uObject->GetFullName()] = static_cast<UClass*>(uObject);
				}
			}
		}
	}

	if (m_staticClasses.contains(className))
	{
		return m_staticClasses[className];
	}

	return nullptr;
}

class UFunction* InstancesComponent::findStaticFunction(const std::string& className)
{
	if (m_staticFunctions.empty())
	{
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - INSTANCES_INTERATE_OFFSET); i++)
		{
			UObject* uObject = UObject::GObjObjects()->at(i);

			if (uObject)
			{
				if (uObject && uObject->IsA<UFunction>())
				{
					m_staticFunctions[uObject->GetFullName()] = static_cast<UFunction*>(uObject);
				}
			}
		}
	}

	if (m_staticFunctions.contains(className))
	{
		return m_staticFunctions[className];
	}

	return nullptr;
}

void InstancesComponent::markInvincible(class UObject* object)
{
	if (!object)
		return;

	object->ObjectFlags &= ~EObjectFlags::RF_Transient;
	object->ObjectFlags &= ~EObjectFlags::RF_TagGarbage;
	object->ObjectFlags &= ~EObjectFlags::RF_PendingKill;
	object->ObjectFlags |= EObjectFlags::RF_DisregardForGC;
	object->ObjectFlags |= EObjectFlags::RF_RootSet;
}

void InstancesComponent::markForDestroy(class UObject* object)
{
	if (!object)
		return;

	object->ObjectFlags |= EObjectFlags::RF_Transient;
	object->ObjectFlags |= EObjectFlags::RF_TagGarbage;
	object->ObjectFlags |= EObjectFlags::RF_PendingKill;

	auto objectIt = std::find(m_createdObjects.begin(), m_createdObjects.end(), object);
	if (objectIt != m_createdObjects.end())
		m_createdObjects.erase(objectIt);
}

class UEngine* InstancesComponent::IUEngine() { return UEngine::GetEngine(); }

class UAudioDevice* InstancesComponent::IUAudioDevice() { return UEngine::GetAudioDevice(); }

class AWorldInfo* InstancesComponent::IAWorldInfo() { return UEngine::GetCurrentWorldInfo(); }

class UCanvas* InstancesComponent::IUCanvas() { return I_UCanvas; }

class AHUD* InstancesComponent::IAHUD() { return I_AHUD; }

class UFileSystem* InstancesComponent::IUFileSystem() { return (UFileSystem*)UFileSystem::StaticClass(); }

class UGameViewportClient* InstancesComponent::IUGameViewportClient() { return I_UGameViewportClient; }

class ULocalPlayer* InstancesComponent::IULocalPlayer()
{
	UEngine* engine = IUEngine();

	if (engine && engine->GamePlayers[0])
	{
		return engine->GamePlayers[0];
	}

	return nullptr;
}

class APlayerController* InstancesComponent::IAPlayerController()
{
	if (!I_APlayerController)
	{
		I_APlayerController = getInstanceOf<APlayerController>();
	}

	return I_APlayerController;
}

struct FUniqueNetId InstancesComponent::GetUniqueID()
{
	ULocalPlayer* localPlayer = IULocalPlayer();

	if (localPlayer)
	{
		return localPlayer->eventGetUniqueNetId();
	}

	return FUniqueNetId{};
}

// ======================= get instance funcs =========================

AGFxHUD_TA* InstancesComponent::getHUD()
{
	if (!hud || !hud->IsA<AGFxHUD_TA>())
	{
		hud = getInstanceOf<AGFxHUD_TA>();
	}

	return hud;
}

UGFxDataStore_X* InstancesComponent::getDataStore()
{
	if (!dataStore || !dataStore->IsA<UGFxDataStore_X>())
	{
		dataStore = getInstanceOf<UGFxDataStore_X>();
	}

	return dataStore;
}

USaveData_TA* InstancesComponent::getSaveData()
{
	if (!saveData || !saveData->IsA<USaveData_TA>())
	{
		saveData = getInstanceOf<USaveData_TA>();
	}

	return saveData;
}

UOnlinePlayer_X* InstancesComponent::getOnlinePlayer()
{
	if (!onlinePlayer || !onlinePlayer->IsA<UOnlinePlayer_X>())
	{
		onlinePlayer = getInstanceOf<UOnlinePlayer_X>();
	}

	return onlinePlayer;
}

// ====================================== misc funcs ================================================

void InstancesComponent::spawnNotification(const std::string& title, const std::string& content, int duration, bool log)
{
	UNotificationManager_TA* notificationManager = Instances.getInstanceOf<UNotificationManager_TA>();
	if (!notificationManager)
		return;

	static UClass* notificationClass = nullptr;
	if (!notificationClass)
	{
		notificationClass = UGenericNotification_TA::StaticClass();
	}

	UNotification_TA* notification = notificationManager->PopUpOnlyNotification(notificationClass);
	if (!notification)
		return;

	FString titleFStr   = FString::create(title);
	FString contentFStr = FString::create(content);

	notification->SetTitle(titleFStr);
	notification->SetBody(contentFStr);
	notification->PopUpDuration = duration;

	if (log)
	{
		LOG("[{}] {}", title.c_str(), content.c_str());
	}
}

class InstancesComponent Instances{};