#include "pch.h"
#include "Titles.hpp"
#include "Items.hpp"
#include <ModUtils/wrappers/GFxWrapper.hpp>


// ##############################################################################################################
// ################################################    INIT    ##################################################
// ##############################################################################################################

void TitlesComponent::Initialize(std::shared_ptr<GameWrapper> gw)
{
	gameWrapper = gw;

	initCvars();
	setFilePaths();
	addPresetsFromJson();
	hookFunctions();
	updateGameTitleAppearances();
	applySelectedAppearanceToUser();
}

void TitlesComponent::setFilePaths()
{
	m_pluginFolder = gameWrapper->GetDataFolder() / "CustomTitle";
	m_titlePresetsJson = m_pluginFolder / "title_presets.json";

	if (!fs::exists(m_titlePresetsJson))
	{
		LOG("File not found: \"{}\"!", m_titlePresetsJson.filename().string());

		fs::create_directories(m_titlePresetsJson.parent_path());

		std::ofstream NewFile(m_titlePresetsJson);

		NewFile << "{ \"presets\": [], \"activePresetIndex\": 0 }";
		NewFile.close();

		LOG("... so we created it :)");
	}
	else
	{
		LOG("Found '{}'", m_titlePresetsJson.filename().string());
	}
}

void TitlesComponent::hookFunctions()
{
	hookWithCallerPost(Events::GFxData_PlayerTitles_TA_UpdateSelectedTitle,
		std::bind(&TitlesComponent::event_GFxData_PlayerTitles_TA_UpdateSelectedTitle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	hookWithCallerPost(Events::GFxData_PlayerTitles_TA_GetTitleData, [this](ActorWrapper Caller, void* Params, std::string eventName)
		{
		    if (!m_shouldOverwriteGetTitleDataReturnVal)
			    return;

		    auto caller = reinterpret_cast<UGFxData_PlayerTitles_TA*>(Caller.memory_address);
			if (!caller)
			 return;

			auto params = reinterpret_cast<UGFxData_PlayerTitles_TA_execGetTitleData_Params*>(Params);
			if (!params)
			 return;
		
		/*
			// debug
			std::string titleId = params->TitleId.ToString();
		    LOG("GFxData_PlayerTitles_TA.GetTitleData(\"{}\") was called", titleId);
		*/

			if (params->TitleId != m_selectedTitleId.c_str())
				return;

			TitleAppearance* preset = getActivePreset();
		    if (!preset)
			    return;
		    params->ReturnValue = preset->toTitleData(params->TitleId);
			LOG("Set custom FPlayerTitleData return value for \"{}\"", params->TitleId.ToString());

			m_shouldOverwriteGetTitleDataReturnVal = false;
		    LOG("Set m_shouldOverwriteGetTitleDataReturnVal: {}", m_shouldOverwriteGetTitleDataReturnVal);
		});

	hookWithCallerPost(Events::PlayerTitleConfig_X_GetTitleData, [this](ActorWrapper Caller, void* Params, ...)
		{
		    return;

			auto caller = reinterpret_cast<UTitleConfig_X*>(Caller.memory_address);
			if (!caller)
			 return;

			auto params = reinterpret_cast<UTitleConfig_X_execGetTitleData_Params*>(Params);
			if (!params)
			 return;

		/*
			// debug
			std::string titleId = params->TitleId.ToString();
			LOG("PlayerTitleConfig_X.GetTitleData(\"{}\") was called", titleId);
		*/

			if (params->TitleId != m_selectedTitleId.c_str())
				return;

			TitleAppearance* preset = getActivePreset();
			if (!preset)
				return;
			params->ReturnValue = preset->toTitleData(params->TitleId);
			LOG("Set custom FPlayerTitleData return value for \"{}\"", params->TitleId.ToString());
		});

	hookEventPost(Events::GFxData_StartMenu_TA_ProgressToMainMenu, [this](...) { applySelectedAppearanceToUser(); });

	auto refreshPriTitlePresetsUsingHud = [this](ActorWrapper Caller, void* params, std::string eventName)
	{
		auto caller = reinterpret_cast<AGFxHUD_TA*>(Caller.memory_address);
		if (!caller)
			return;

		refreshPriTitlePresets(caller);
	};
	hookWithCallerPost(Events::GFxHUD_TA_HandleTeamChanged, refreshPriTitlePresetsUsingHud);

	hookEventPost(Events::GFxData_PRI_TA_SetPlayerTitle, [this](...) { refreshPriTitlePresets(); });

	hookEventPost(Events::EngineShare_X_EventPreLoadMap, [this](...){ m_ingameCustomPresets.clear(); });

	hookEvent(Events::PlayerController_EnterStartState, [this](...)
		{
			// TODO: check cvar if custom title is enabled

			auto userPri = getUserGFxPRI();
			if (!userPri)
				return;

			auto title = getActivePreset();
			if (!title)
				return;

			m_ingameCustomPresets[userPri] = *title;

			LOG("Added user gfxPri to m_ingameCustomPresets...");
		});
	
	hookWithCallerPost(Events::PlayerController_EnterStartState, [this](ActorWrapper Caller, ...)
		{
			auto showTitleToOthers_cvar = getCvar(Cvars::showTitleToOthers);
			if (!showTitleToOthers_cvar || !showTitleToOthers_cvar.getBoolValue())
				return;

			auto caller = reinterpret_cast<APlayerController*>(Caller.memory_address);
			if (!caller)
				return;

			TitleAppearance* preset = getActivePreset();
			if (!preset)
				return;

			sendTitleDataChat(*preset, caller);
		});
}

void TitlesComponent::initCvars()
{
	registerCvar_bool(Cvars::showOtherPlayerTitles,		true).bindTo(m_showOtherPlayerTitles);
	registerCvar_bool(Cvars::showTitleToOthers,			true).bindTo(m_showTitleToOthers);
	registerCvar_bool(Cvars::filterOtherPlayerTitles,	false).bindTo(m_filterOtherPlayerTitles);
	registerCvar_bool(Cvars::useHueColorPicker,			true).bindTo(m_useHueColorPicker);
	registerCvar_bool(Cvars::applyOthersTitleNotif,		false).bindTo(m_notifyWhenApplyingOthersTitle);
}



// ##############################################################################################################
// ############################################### EVENT CALLBACKS ##############################################
// ##############################################################################################################

void TitlesComponent::event_GFxData_PlayerTitles_TA_UpdateSelectedTitle(ActorWrapper Caller, void* Params, std::string eventName)
{
	auto caller = reinterpret_cast<UGFxData_PlayerTitles_TA*>(Caller.memory_address);
	if (!caller)
		return;

	auto params = reinterpret_cast<UGFxData_PlayerTitles_TA_execUpdateSelectedTitle_Params*>(Params);
	if (!params)
		return;

	std::string titleId = params->Title.ToString();
	m_selectedTitleId   = StringUtils::ToWideString(titleId);
	LOG("Updated m_selectedTitleId: \"{}\"", titleId);

	// this bool exists so we only modify the FPlayerTitleData of selected title if user has just selected it via the RL title picker
	// bc we dont wanna overwrite it EVERY time FPlayerTitleData is requested (via UGFxData_PlayerTitles_TA::GetTitleData)
	// like, for example, if GetTitleData() is called to get the title data for a DIFFERENT player in the lobby
	// ... but im not even sure if that's what happens. still need to test ig
	m_shouldOverwriteGetTitleDataReturnVal = true;
	LOG("Set m_shouldOverwriteGetTitleDataReturnVal: {}", m_shouldOverwriteGetTitleDataReturnVal);

	// We call UpdatePlayerTitles() to undo the custom appearance done to the old selected title...
	// which causes a bunch of GetTitleData() side-effect functions to fire...
	// which are handled by other hooks to set the custom appearance on the new selected title
	caller->UpdatePlayerTitles();
}



// ##############################################################################################################
// ###############################################    FUNCTIONS    ##############################################
// ##############################################################################################################

void TitlesComponent::updateGameTitleAppearances(UTitleConfig_X* config, bool forceSearch)
{
	if (forceSearch || !config)
	{
		config = getTitleConfig(forceSearch);
		if (!config)
			return;
	}

	m_gameTitles.clear(); // yeet old data

	LOG("UTitleConfig_X has {} titles", config->Titles.size());
	for (const FPlayerTitleData& title_data : config->Titles)
	{
		if (title_data.Id == L"None" || title_data.Text.empty())
			continue;

		m_gameTitles.emplace_back(title_data);
	}

	LOG("Stored appearance of {} game titles", m_gameTitles.size());
}

void TitlesComponent::refreshPriTitlePresets(AGFxHUD_TA* hud)
{
	if (!validUObject(hud))
	{
		hud = getGFxHUD();
		if (!hud)
			return;
	}

	// first reset all pri rows to their default titles
	for (auto pri : hud->PRIData)
	{
		if (!validUObject(pri))
			continue;

		pri->HandleTitleChanged(pri->PRI);
	}

	auto applyExistingPresetsToPris = [this]()
		{
			AGFxHUD_TA* hud = getGFxHUD();
			if (!hud)
				return;

			// loop thru pris and apply custom title presets to the matching pridatas in m_ingameCustomPresets
			for (auto pri : hud->PRIData)
			{
				if (!pri)
					continue;

				auto it = m_ingameCustomPresets.find(pri);
				if (it == m_ingameCustomPresets.end())
					continue;

				applyPresetToPri(pri, it->second);
			}
		};

	// lil 0.1s delay to allow side effect functions from HandleTitleChanged to finish executing
	DELAY_CAPTURE(0.1f,
		applyExistingPresetsToPris();
	, applyExistingPresetsToPris);
}

TitleAppearance* TitlesComponent::getActivePreset()
{
	if (m_titlePresets.empty())
	{
		LOG("There are no existing title presets...");
		return nullptr;
	}
	if (m_activePresetIndex >= m_titlePresets.size())
	{
		LOG("ERROR: Active preset index is out of range: {}", m_activePresetIndex);
		return nullptr;
	}

	return &m_titlePresets[m_activePresetIndex];
}

void TitlesComponent::applySelectedAppearanceToUser()
{
	TitleAppearance* title = getActivePreset();
	if (!title)
		return;

	// apply to banner
	applyPresetToBanner(*title);

	// apply to pri (in-game)
	auto pri = getUserGFxPRI();
	if (!pri)
		return;

	m_ingameCustomPresets[pri] = *title;	// update active preset in m_ingameCustomPresets 
	applyPresetToPri(pri, *title);
}

void TitlesComponent::applyPresetFromChatData(std::string data, const FChatMessage& msg, AHUDBase_TA* caller)
{
#define DONT_APPLY_TO_USER

	if (!validUObject(caller) || !caller->IsA<AGFxHUD_TA>())
		return;
	auto hud = static_cast<AGFxHUD_TA*>(caller);

	if (!*m_showOtherPlayerTitles)
		return;

	LOG("Recieved chat data string: \"{}\"", Format::EscapeBraces(data));
	

#ifdef DONT_APPLY_TO_USER
	FUniqueNetId& senderId = msg.PRI->UniqueId;
	FUniqueNetId userId = Instances.GetUniqueID();
	if (senderId.EpicAccountId == userId.EpicAccountId && senderId.Uid == userId.Uid)	// skip applying title appearance if chat was from user
		return;
#endif


	auto gfxPri = getGFxPriFromChatData(msg.PRI, hud);
	if (!gfxPri)
		return;

	TitleAppearance appearance;	// default appearance
	if (!decodeAppearance(data, appearance))
	{
		LOG("ERROR: Unable to parse title appearance from string: \"{}\"", Format::EscapeBraces(data));
		return;
	}

	std::string senderName = msg.PlayerName.ToString();
	std::string successMsg = "Applied title appearance for " + senderName;

	if (*m_filterOtherPlayerTitles)
	{
		CurlRequest req;
		req.url = "https://www.purgomalum.com/service/plain?text=" + Format::EscapeForHTMLIncludingSpaces(appearance.getText());

		auto responseCallback = [this, gfxPri, appearance, senderName, successMsg, hud](int code, std::string result)
			{
				if (code != 200)
				{
					LOG("ERROR: Check for update HTTP Request Failed! Response code: {}", code);
					return;
				}

				std::string oldText = appearance.getText();
				TitleAppearance title = appearance;	// bc capture variables are const references i think, and we want non-const so we can call setText(...)
				title.setText(result);

				GAME_THREAD_EXECUTE_CAPTURE(
					if (!gfxPri)
						return;
					
					LOG("Censored title text for {}: \"{}\" ---> \"{}\"", senderName, Format::EscapeBraces(oldText), Format::EscapeBraces(title.getText()));

					m_ingameCustomPresets[gfxPri] = title;
					refreshPriTitlePresets(hud);
					//applyPresetToPri(gfxPri, title);

					LOG(successMsg);
					if (*m_notifyWhenApplyingOthersTitle)
						Instances.SpawnNotification("custom title", successMsg, 3, true);

				, gfxPri, oldText, title, senderName, successMsg, hud);
			};

		HttpWrapper::SendCurlRequest(req, responseCallback);
		
		LOG("Sent curl request: \"{}\"", Format::EscapeBraces(req.url));
	}
	else
	{
		m_ingameCustomPresets[gfxPri] = appearance;
		refreshPriTitlePresets(hud);
		//applyPresetToPri(gfxPri, appearance);

		LOG(successMsg);
		if (*m_notifyWhenApplyingOthersTitle)
			Instances.SpawnNotification("custom title", successMsg, 3, true);
	}
}

void TitlesComponent::spawnSelectedPreset(bool log)
{
	if (log) // for debugging weird thing where the spawned title doesnt match active preset
	{
		for (int i = 0; i < m_titlePresets.size(); ++i)
		{
			const auto& preset = m_titlePresets[i];
			std::string log = i == m_activePresetIndex ? preset.getText() + "\t<---" : preset.getText();
			LOG("[{}] {}", i, Format::EscapeBraces(log));
		}
	}

	FName customTitleId = getCustomTitleId();
	if (!customTitleId.IsValid())
		return;

	UTitleConfig_X* config = getTitleConfig();
	if (!validUObject(config))
		return;

	TitleAppearance* selectedPreset = getActivePreset();
	if (!selectedPreset)
		return;

	LOG("Using this ID for spawning: \"{}\"", customTitleId.ToString());
	LOG("Finna spawn title with text: \"{}\"", Format::EscapeBraces(selectedPreset->getText()));

	FPlayerTitleData titleData = config->GetTitleData(customTitleId);
	if (titleData.Id != customTitleId)	// aka if config doesnt contain a FPlayerTitleData with the given ID... then we add it
	{
		titleData.Id =			customTitleId;
		titleData.Category =	L"XP";
		titleData.Text =		selectedPreset->getTextFStr();
		titleData.Color =		selectedPreset->getTextFColor();
		titleData.GlowColor =	selectedPreset->getGlowFColor();
		config->Titles.push_back(titleData);

		titleData = config->GetTitleData(customTitleId);
	}
	
	FPlayerTitleData& spawnableTitle = getTitleFromConfig(config->Titles.size() - 1, config);

	spawnableTitle.Text =		selectedPreset->getTextFStr();
	spawnableTitle.Color =		selectedPreset->getTextFColor();
	spawnableTitle.GlowColor =	selectedPreset->getGlowFColor();

	spawn(FString(spawnableTitle.Id.GetDisplayNameEntry().GetWideName()));
}


FName TitlesComponent::getCustomTitleId()
{
	static int id = 0;

	const int gnamesSize = GNames->size();
	if (id >= gnamesSize)
		return FName(-1);

	UTitleConfig_X* config = getTitleConfig();
	if (!validUObject(config))
	{
		LOG("Oh no UTitleConfig_X* is invalid");
		return FName(-1);
	}

	FName name{ id };

	// if the FName we've added to the config is still there, use that
	if (name != L"None" && config->GetTitleData(name).Id == name)
		return name;
	// otherwise find an FName that doesn't alr exist in the config (aka dont use one for an existing title)
	// and use that (which will eventually get added to the config.. and should trigger the above condition next time this func is called)
	else 
	{
		while ((!name.GetEntry() || config->GetTitleData(name).Id == name) && id < gnamesSize - 1)
			name = FName(++id);
	}

	return name;
}

bool TitlesComponent::spawn(const FString& spawn_id, bool animation, const std::string& spawn_msg)
{
	TArray<FOnlineProductAttribute> attributes;
	attributes.push_back({ L"TitleId", spawn_id }); // "TitleId" FNameentryId is 41313

	LOG("animation: {}", animation);
	if (!animation)
	{
		auto noNotify = Instances.GetDefaultInstanceOf<UProductAttribute_NoNotify_TA>();
		if (noNotify)
		{
			attributes.push_back(noNotify->InstanceOnlineProductAttribute());
			LOG("Added UProductAttribute_NoNotify_TA to title attributes array");
		}
		else
			LOG("ERROR: UProductAttribute_NoNotify_TA instance is null");
	}

	if (Items.SpawnProduct(3036, attributes, 0, 0, false, spawn_msg, animation))
	{
		LOG("Spawned Title using ID: {}", spawn_id.ToString());
		return true;
	}
	else
	{
		LOG("Failed to Spawn Title using ID: {}", spawn_id.ToString());
		return false;
	}
}

bool TitlesComponent::spawn(const FName& spawn_id, bool animation, const std::string& spawn_msg)
{
	FString fstr{ spawn_id.GetDisplayNameEntry().GetWideName() };
	return spawn(fstr, animation, spawn_msg);
}

bool TitlesComponent::spawn(const std::string& spawn_id, bool animation, const std::string& spawn_msg)
{
	FString fstr = FString::create(spawn_id);
	return spawn(fstr, animation, spawn_msg);
}



// ##############################################################################################################
// ######################################   PERSISTENCE/STATE FUNCTIONS    ######################################
// ##############################################################################################################

void TitlesComponent::addNewPreset()
{
	FColor white{ 255, 255, 255, 255 };
	m_titlePresets.emplace_back(std::format("{{legend}} title preset {} {{diamond}}", m_titlePresets.size() + 1), white, white);
	m_activePresetIndex = m_titlePresets.size() - 1;
	applySelectedAppearanceToUser();
}

void TitlesComponent::deletePreset(int index)
{
	if (m_titlePresets.empty())
		return;
	m_titlePresets.erase(m_titlePresets.begin() + index);	// erase binding at given index
	m_activePresetIndex = m_titlePresets.empty() ? 0 : m_titlePresets.size() - 1;	// reset selected binding index
}

void TitlesComponent::writePresetsToJson(bool notification) const
{
	json j;
	j["presets"] = json::array();

	for (const auto& preset : m_titlePresets)
	{
		j["presets"].push_back(preset.toJson());
	}

	j["activePresetIndex"] = m_activePresetIndex;

	std::string msg;
	if (Files::write_json(m_titlePresetsJson, j))
		msg = std::format("Saved title presets to \"{}\"", m_titlePresetsJson.filename().string());
	else
		msg = std::format("[ERROR] Failed to save title presets to \"{}\"", m_titlePresetsJson.filename().string());
	LOG(msg);

	if (notification)
	{
		GAME_THREAD_EXECUTE_CAPTURE(
			Instances.SpawnNotification("custom title", msg, 3.0f, true);
		, msg);
	}
}

void TitlesComponent::addPresetsFromJson()
{
	json j = Files::get_json(m_titlePresetsJson);
	if (j.empty())
		return;

	auto presets = j["presets"];
	if (presets.is_null() || !presets.is_array())
		return;

	for (const auto& presetJson : presets)
	{
		TitleAppearance title;
		title.fromJson(presetJson);
		m_titlePresets.push_back(title);
	}

	int selectedIndex = j.value("activePresetIndex", 0);
	if (selectedIndex >= 0 && selectedIndex < m_titlePresets.size())
		m_activePresetIndex = selectedIndex;

	LOG("Added {} title presets from JSON", presets.size());
}



// ##############################################################################################################
// ##########################################   STATIC FUNCTIONS    #############################################
// ##############################################################################################################

UTitleConfig_X* TitlesComponent::getTitleConfig(bool forceSearch)
{
	static UTitleConfig_X* config = Instances.GetInstanceOf<UTitleConfig_X>();

	if (forceSearch || !validUObject(config))
		config = Instances.GetInstanceOf<UTitleConfig_X>();

	return config;
}

FPlayerTitleData& TitlesComponent::getTitleFromConfig(int index, UTitleConfig_X* config)
{
	if (!config)
	{
		config = getTitleConfig();
		if (!config)
			throw std::exception("UTitleConfig_X* is null");
	}
	if (index < 0 || index >= config->Titles.size())
		throw std::exception(std::format("Title at index {} doesn't exist! Titles size: {}", index, config->Titles.size()).c_str());

	return config->Titles.at(index);
}

AGFxHUD_TA* TitlesComponent::getGFxHUD()
{
	auto lp = Instances.IULocalPlayer();
	if (!validUObject(lp))
	{
		LOG("ERROR: Instances.IULocalPlayer() is null");
		return nullptr;
	}

	auto pc = lp->Actor;
	if (!validUObject(pc))
	{
		LOG("ERROR: lp->Actor is null");
		return nullptr;
	}

	if (!validUObject(pc->myHUD) || !pc->myHUD->IsA<AGFxHUD_TA>())
	{
		//LOG("ERROR: pc->myHUD is null or invalid"); // <--- gets called on every title text/color change while in main menu... cluttering console :(
		return nullptr;
	}

	return static_cast<AGFxHUD_TA*>(pc->myHUD);
}

UGFxData_PRI_TA* TitlesComponent::getUserGFxPRI()
{
	auto gfxHud = getGFxHUD();
	if (!gfxHud)
		return nullptr;

	if (!validUObject(gfxHud->OwnerPRI))
	{
		LOG("ERROR: gfxHud->OwnerPRI is null or invalid");
		return nullptr;
	}

	int idx = gfxHud->GetPRIDataIndex(gfxHud->OwnerPRI); // or use any other player's PRI here (like... maybe from a chat message ;D)
	if (idx >= gfxHud->PRIData.size())
		return nullptr;

	auto userGfxPri = gfxHud->PRIData.at(idx);
	if (!validUObject(userGfxPri))
		return nullptr;

	return userGfxPri;
}

UGFxData_PRI_TA* TitlesComponent::getGFxPriFromChatData(APlayerReplicationInfo* priBase, AHUDBase_TA* hudBase)
{
	if (!validUObject(priBase))
	{
		LOG("ERROR: APlayerReplicationInfo* from chat data is invalid");
		return nullptr;
	}
	if (!validUObject(hudBase))
		return nullptr;
		
	if (!priBase->IsA<APRI_TA>())
	{
		LOG("ERROR: APlayerReplicationInfo instance isn't a APRI_TA");
		return nullptr;
	}
	if (!hudBase->IsA<AGFxHUD_TA>())
	{
		LOG("ERROR: AHUDBase_TA instance isn't a AGFxHUD_TA");
		return nullptr;
	}

	auto pri = static_cast<APRI_TA*>(priBase);
	auto hud = static_cast<AGFxHUD_TA*>(hudBase);

	auto gfxPriIndex = hud->GetPRIDataIndex(pri);
	if (gfxPriIndex >= hud->PRIData.size())
	{
		LOG("ERROR: PRIData index is out of bounds");
		return nullptr;
	}

	auto gfxPri = hud->PRIData.at(gfxPriIndex);
	if (!validUObject(gfxPri))
		return nullptr;

	return gfxPri;
}

void TitlesComponent::applyPresetToPri(UGFxData_PRI_TA* pri, const TitleAppearance& title)
{
	if (!validUObject(pri))
		return;

	GfxWrapper gfxPri{ pri };

	gfxPri.set_string(L"XPTitle", title.getTextFStr());
	gfxPri.set_int(L"TitleColor", title.getIntTextColor());
	gfxPri.set_int(L"TitleGlowColor", title.getIntGlowColor());

	LOG("Applied title preset for {}... (UGFxData_PRI_TA: {})", pri->PlayerName.ToString(), Format::ToHexString(pri));
}

void TitlesComponent::applyPresetToBanner(const TitleAppearance& title, UGFxData_PlayerTitles_TA* pt, bool log)
{
	if (!validUObject(pt))
	{
		pt = Instances.GetInstanceOf<UGFxData_PlayerTitles_TA>();
		if (!validUObject(pt))
			return;
	}

	GfxWrapper ptGfx{ pt };
	auto ds = ptGfx.get_datastore();
	if (!validUObject(ds))
		return;

	int32_t row = pt->SelectedTitle;
	FName table{ L"PlayerTitlesPlayerTitles" };

	ds->SetStringValue(table, row, L"Text", title.getTextFStr());
	ds->SetIntValue(table, row, L"Color", title.getIntTextColor());
	ds->SetIntValue(table, row, L"GlowColor", title.getIntGlowColor());

	if (log)
		LOG("Applied title preset to banner");
}

void TitlesComponent::sendTitleDataChat(const TitleAppearance& appearance, APlayerController* pc)
{
	if (!pc)
	{
		auto lp = Instances.IULocalPlayer();
		if (!lp)
		{
			LOG("ERROR: Instances.IULocalPlayer() is null");
			return;
		}

		if (!lp->Actor || !lp->Actor->IsA<APlayerController_TA>())
		{
			LOG("ERROR: lp->Actor is null or isnt a APlayerController_TA");
			return;
		}
		pc = lp->Actor;
	}

	auto pcTA = static_cast<APlayerController_TA*>(pc);

	std::string titleDataStr = appearance.toEncodedString();

	pcTA->Say_TA(FString::create(titleDataStr), EChatChannel::EChatChannel_Match, FUniqueNetId{}, true, false);

	LOG("Sent title data chat: \"{}\"", Format::EscapeBraces(titleDataStr));
}

bool TitlesComponent::decodeAppearance(const std::string& inStr, TitleAppearance& outAppearance)
{
	auto parts = Format::SplitStr(inStr, '|');
	if (parts.size() < 3)  // invalid format, ignore
		return false;

/*
	LOG("parts[0]: \"{}\"", parts[0]);
	LOG("parts[1]: \"{}\"", parts[1]);
	LOG("parts[2]: \"{}\"", parts[2]);
*/

	outAppearance.setText(parts[0]);
	outAppearance.setTextColor(TitleAppearance::HexToColor(parts[1]));

	if (parts[2] == "-")
	{
		outAppearance.setGlowColor(outAppearance.getTextFColor());
		outAppearance.setSameTextAndGlowColor(true);
	}
	else
	{
		outAppearance.setGlowColor(TitleAppearance::HexToColor(parts[2]));
		outAppearance.setSameTextAndGlowColor(false);
	}

	return true;
}



// ##############################################################################################################
// ##########################################   DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void TitlesComponent::display_titlePresetList()
{
	{
		GUI::ScopedChild c{ "List", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f) };

		ImGui::TextUnformatted("Presets:");
		ImGui::Separator();
		auto titleCustomizationsSize = m_titlePresets.size();

		for (int i = 0; i < titleCustomizationsSize; ++i)
		{
			auto& appearancePreset = m_titlePresets[i];

			GUI::ScopedID id{&appearancePreset};

			ImVec4 textCol = appearancePreset.getImGuiTextColor();
			textCol.w      = 1.0f;	// set alpha channel to 1 when rendering menu so title is always visible
			ImGui::PushStyleColor(ImGuiCol_Text, textCol);

			if (ImGui::Selectable(appearancePreset.m_text.c_str(), m_activePresetIndex == i))
			{
				m_activePresetIndex = i;
				GAME_THREAD_EXECUTE(
					applySelectedAppearanceToUser();
					writePresetsToJson(false); // to save the active preset index
				);
			}

			ImGui::PopStyleColor();
		}
	}

	GUI::Spacing(2);

	{
		GUI::ScopedChild c{ "AddPreset" };
	
		if (ImGui::Button("Add New Preset", ImGui::GetContentRegionAvail()))
		{
			GAME_THREAD_EXECUTE(
				addNewPreset();
				//writePresetsToJson();
			);
		}
	}
}

void TitlesComponent::display_titlePresetInfo()
{
	static ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel;
	colorEditFlags = *m_useHueColorPicker ?
		(ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel) :
		ImGuiColorEditFlags_NoInputs;

	{
		GUI::ScopedChild c{ "PrestInfo", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.8f) };

		if (m_titlePresets.empty())
		{
			ImGui::Text("Add a title preset....");
			return;
		}
		if (m_activePresetIndex >= m_titlePresets.size())
		{
			m_activePresetIndex = 0;
		}

		TitleAppearance& appearance = m_titlePresets[m_activePresetIndex];


		// text
		char textBuffer[MAX_TEXT_LENGTH + 1];  // +1 for null terminator
		std::strncpy(textBuffer, appearance.m_text.c_str(), MAX_TEXT_LENGTH);
		textBuffer[MAX_TEXT_LENGTH] = '\0'; // Make sure it's null-terminated

		if (std::strlen(textBuffer) >= MAX_TEXT_LENGTH)
			ImGui::TextColored(GUI::Colors::Yellow, "Max text length: %i characters", MAX_TEXT_LENGTH);

		ImGui::Text("%d/%d characters", static_cast<int>(std::strlen(textBuffer)), MAX_TEXT_LENGTH);
		if (ImGui::InputText("Text", textBuffer, MAX_TEXT_LENGTH + 1))
		{
			appearance.m_text = textBuffer;

			GAME_THREAD_EXECUTE(
				applySelectedAppearanceToUser();
			);
		}

		GUI::Spacing(4);

		if (ImGui::Checkbox("Same text & glow color", &appearance.m_sameTextAndGlowColor))
		{
			if (appearance.m_sameTextAndGlowColor)
			{
				appearance.setGlowColor(appearance.m_textColor);

				GAME_THREAD_EXECUTE(
					applySelectedAppearanceToUser();
				);
			}
		}

		GUI::Spacing(2);

		if (appearance.m_sameTextAndGlowColor)
		{
			float color[4] = { 0, 0, 0, 0 };
			appearance.getTextColor(color);

			if (ImGui::ColorEdit4("Color", &color[0], colorEditFlags))
			{
				appearance.setTextColor(color);
				appearance.setGlowColor(color);

				GAME_THREAD_EXECUTE(
					applySelectedAppearanceToUser();
				);
			}
		}
		else
		{
			// text color picker
			float textCol[4] = { 0, 0, 0, 0 };
			appearance.getTextColor(textCol);

			if (ImGui::ColorEdit4("Text color", &textCol[0], colorEditFlags))
			{
				appearance.setTextColor(textCol);

				GAME_THREAD_EXECUTE(
					applySelectedAppearanceToUser();
				);
			}

			GUI::Spacing(2);

			// glow color picker
			float glowCol[4] = { 0, 0, 0, 0 };
			appearance.getGlowColor(glowCol);

			if (ImGui::ColorEdit4("Glow color", &glowCol[0], colorEditFlags))
			{
				appearance.setGlowColor(glowCol);

				GAME_THREAD_EXECUTE(
					applySelectedAppearanceToUser();
				);
			}
		}

		GUI::Spacing(8);

		display_gameTitlesDropdown();

		GUI::SameLineSpacing_relative(20);

		if (ImGui::Button("Refresh"))
		{
			GAME_THREAD_EXECUTE(
				updateGameTitleAppearances(nullptr, true);
				Instances.SpawnNotification("custom title", "Updated game title presets", 3);
			);
		}

		ImGui::Text("%zu title presets", m_gameTitles.size());
	}

	GUI::Spacing(2);

	{
		GUI::ScopedChild c{ "ButtonsSection" };

		{
			GUI::ScopedChild c{ "SpawnButton", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f) };
		
			if (ImGui::Button("Spawn", ImGui::GetContentRegionAvail()))
			{
				_globalCvarManager->executeCommand(Commands::spawnCustomTitle.name); // <--- i thought executeCommand is supposed to always run in game thread, ig we'll see...
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Press \"OK\" at the spawn prompt to prevent potential weird behavior");
		}

		{
			GUI::ScopedChild c{ "SaveOrDelete" };

			{
				GUI::ScopedChild c{ "SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0) };

				if (ImGui::Button("Save", ImGui::GetContentRegionAvail()))
				{
					GAME_THREAD_EXECUTE(
						writePresetsToJson();
					);
				}
			}

			ImGui::SameLine();

			{
				GUI::ScopedChild c{ "DeleteButton", ImGui::GetContentRegionAvail() };

				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

				if (ImGui::Button("Delete Preset", ImGui::GetContentRegionAvail()))
				{
					GAME_THREAD_EXECUTE(
						deletePreset(m_activePresetIndex);	// <--- index param may not be necessary (bc the index would always just be m_activePresetIndex)
						writePresetsToJson(false);
					);
				}

				ImGui::PopStyleColor(3);
			}
		}
	}
}

void TitlesComponent::display_gameTitlesDropdown()
{
	TitleAppearance* currentTitle = getActivePreset();
	if (!currentTitle)
		return;

	const std::string& currentTitleText = currentTitle->getText();

	char searchBuffer[128] = "";  // text buffer for search input

	if (ImGui::BeginSearchableCombo("Game presets", currentTitleText.c_str(), searchBuffer, sizeof(searchBuffer), "search..."))
	{
		std::string searchQuery = Format::ToLower(searchBuffer); // convert search text to lower

		for (const auto& title : m_gameTitles)
		{
			GUI::ScopedID id{ &title };

			const std::string& titleText = title.getText();
			const std::string titleTextLower = Format::ToLower(title.getText());

			if (!searchQuery.empty())	// only render option if there's text in search box & it matches the key name
			{
				if (titleTextLower.find(searchQuery) == std::string::npos)
					continue;

				ImGui::PushStyleColor(ImGuiCol_Text, title.getImGuiTextColor());

				if (ImGui::Selectable(titleText.c_str(), titleText == currentTitleText))
				{
					*currentTitle = title;
					GAME_THREAD_EXECUTE(
						applySelectedAppearanceToUser();
					);
				}

				ImGui::PopStyleColor();
			}
			else	// if there's no text in search box, render all possible key options
			{
				ImGui::PushStyleColor(ImGuiCol_Text, title.getImGuiTextColor());

				if (ImGui::Selectable(titleText.c_str(), titleText == currentTitleText))
				{
					*currentTitle = title;
					GAME_THREAD_EXECUTE(
						applySelectedAppearanceToUser();
					);
				}

				ImGui::PopStyleColor();
			}
		}

		ImGui::EndCombo();
	}
}



// ##############################################################################################################
// #############################################    TESTING    ##################################################
// ##############################################################################################################

bool TitlesComponent::test1()
{
	TitleAppearance* title = getActivePreset();
	if (!title)
		return false;

	TitleAppearance& preset = *title;

	LOG("============ BEFORE CHAT ==========");

	LOG("Active preset text color: {}", preset.getDebugTextColorStr());
	LOG("Active preset glow color: {}", preset.getDebugGlowColorStr());

	// send encoded chat
	sendTitleDataChat(preset);


	return true;
}

bool TitlesComponent::test2()
{
	return true;
}


class TitlesComponent Titles {};