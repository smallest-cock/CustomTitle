#include "pch.h"
#include "Cvars.hpp"
#include <ModUtils/util/Utils.hpp>
#include <ModUtils/gui/GuiTools.hpp>
#include <ModUtils/wrappers/GFxWrapper.hpp>
#include "Titles.hpp"
#include "Items.hpp"
#include "Instances.hpp"
#include "Macros.hpp"
#include "Events.hpp"
#include "HookManager.hpp"

// ##############################################################################################################
// ################################################    INIT    ##################################################
// ##############################################################################################################

void TitlesComponent::init(const std::shared_ptr<GameWrapper>& gw)
{
	gameWrapper = gw;

	initCvars();
	setFilePaths();
	addPresetsFromJson();
	initHooks();
	updateGameTitleAppearances();
	applySelectedAppearanceToUser();
}

void TitlesComponent::setFilePaths()
{
	m_pluginFolder     = gameWrapper->GetDataFolder() / "CustomTitle";
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
		LOG("Found '{}'", m_titlePresetsJson.filename().string());
}

void TitlesComponent::initHooks()
{
	Hooks.hookEvent(Events::HUDBase_TA_DrawHUD,
	    HookType::Post,
	    [this](ActorWrapper Caller, ...)
	    {
		    TitleAppearance* preset            = getActivePreset(false);
		    bool             bannerHasRGB      = preset && preset->useRGB();
		    bool             gameHasRGBPresets = m_ingamePresets.rgbPresetsExist();

		    if (!bannerHasRGB && !gameHasRGBPresets)
			    return;

		    tickRGB();

		    // update RGB preset on banner
		    if (*m_enabled && bannerHasRGB)
		    {
			    auto* caller = reinterpret_cast<AHUDBase_TA*>(Caller.memory_address);
			    if (!validUObject(caller))
				    return;
			    applyPresetToBanner(*preset, caller->Shell);
		    }

		    // update RGB presets in-game
		    if (gameHasRGBPresets)
		    {
			    auto    userId   = Instances.GetUniqueID();
			    int32_t rgbColor = GRainbowColor::GetDecimal();
			    m_ingamePresets.forEachRGBPreset(
			        [this, rgbColor, userId](UGFxData_PRI_TA* pri, const TitleAppearance& preset)
			        {
				        if (!validUObject(pri))
					        return;

				        if (!*m_enabled && sameId(pri->PlayerID, userId))
					        return;

				        GfxWrapper gfxPri{pri};
				        gfxPri.set_string(L"XPTitle", preset.getTextFStr());
				        gfxPri.set_int(L"TitleColor", rgbColor);
				        gfxPri.set_int(L"TitleGlowColor", rgbColor);
			        });
		    }
	    });

	Hooks.hookEvent(Events::GFxData_PlayerTitles_TA_UpdateSelectedTitle,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (!*m_enabled)
			    return;

		    auto* caller = reinterpret_cast<UGFxData_PlayerTitles_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    auto* params = reinterpret_cast<UGFxData_PlayerTitles_TA_execUpdateSelectedTitle_Params*>(Params);
		    if (!params)
			    return;

		    // undo custom appearance on current title before we equip a different one
		    applyPresetToBanner(m_currentOgAppearance);
		    LOG("Undid customization on old equipped title...");
	    });

	Hooks.hookEvent(Events::GFxData_PlayerTitles_TA_UpdateSelectedTitle,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* caller = reinterpret_cast<UGFxData_PlayerTitles_TA*>(Caller.memory_address);
		    if (!validUObject(caller))
			    return;

		    auto* params = reinterpret_cast<UGFxData_PlayerTitles_TA_execUpdateSelectedTitle_Params*>(Params);
		    if (!params)
			    return;

		    m_selectedTitleId = params->Title.ToString();
		    LOG("Updated m_selectedTitleId: \"{}\"", m_selectedTitleId);

		    if (m_selectedTitleId != "None")
		    {
			    // update stored OG appearance for current title
			    int32_t idFnameIdx = params->Title.GetDisplayIndex();
			    for (const auto& title : m_gameTitles)
			    {
				    if (title.idFnameEntry != idFnameIdx)
					    continue;

				    m_currentOgAppearance = static_cast<TitleAppearance>(title);
				    break;
			    }
		    }
		    else
		    {
			    TitleAppearance noneAppearance{};
			    noneAppearance.setText("");
			    noneAppearance.setTextColor({});
			    noneAppearance.setGlowColor({});
			    m_currentOgAppearance = noneAppearance;
		    }

		    if (!*m_enabled)
			    return;

		    // here we only apply the active preset to banner, bc pretty sure UpdateSelectedTitle wont be called anywhere except main menu
		    TitleAppearance* title = getActivePreset();
		    if (!title)
			    return;
		    applyPresetToBanner(*title, caller);

		    // this bool exists so we only modify FPlayerTitleData of our equipped title if user has just selected it via the title picker
		    // bc we dont wanna overwrite EVERY FPlayerTitleData that's requested by the game... bc it might affect other players too (idk)
		    m_shouldOverwriteGetTitleDataReturnVal = true;
		    LOG("Set m_shouldOverwriteGetTitleDataReturnVal: {}", m_shouldOverwriteGetTitleDataReturnVal);
	    });

	/*
	hookEventPost(Events::GFxData_PlayerTitles_TA__HandleLoadoutSaveLoaded_0x1,
	[this](...)
	{
	    if (m_selectedTitleId == "None")
	    return;

	    TitleAppearance* preset = getActivePreset();
	    if (!preset)
	    return;
	    applyPresetToBanner(*preset);
	});
	*/

	Hooks.hookEvent(Events::GFxData_PlayerTitles_TA_GetTitleData,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (!*m_enabled || !m_shouldOverwriteGetTitleDataReturnVal)
			    return;

		    auto* caller = reinterpret_cast<UGFxData_PlayerTitles_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    auto* params = reinterpret_cast<UGFxData_PlayerTitles_TA_execGetTitleData_Params*>(Params);
		    if (!params)
			    return;

		    /*
		    // debug
		    std::string titleId = params->TitleId.ToString();
		    LOG("(debug) GFxData_PlayerTitles_TA.GetTitleData(\"{}\") was called", titleId);
		    */

		    if (params->TitleId.ToString() != m_selectedTitleId)
			    return;

		    TitleAppearance* preset = getActivePreset();
		    if (!preset)
			    return;

		    if (m_selectedTitleId == "None")
			    DELAY(
			        0.1f,
			        {
				        if (!preset || !validUObject(caller))
					        return;
				        applyPresetToBanner(*preset, caller);
			        },
			        preset,
			        caller);
		    else
		    {
			    params->ReturnValue = preset->toTitleData(params->TitleId);
			    LOG("Set custom FPlayerTitleData return value for \"{}\"", params->TitleId.ToString());

			    m_shouldOverwriteGetTitleDataReturnVal = false;
			    LOG("Set m_shouldOverwriteGetTitleDataReturnVal: {}", m_shouldOverwriteGetTitleDataReturnVal);
		    }
	    });

	/*
	hookWithCallerPost(Events::PlayerTitleConfig_X_GetTitleData,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
	        return;

	        auto* caller = reinterpret_cast<UTitleConfig_X*>(Caller.memory_address);
	        if (!caller)
	            return;

	        auto* params = reinterpret_cast<UTitleConfig_X_execGetTitleData_Params*>(Params);
	        if (!params)
	            return;

	        //// debug
	        // std::string titleId = params->TitleId.ToString();
	        // LOG("PlayerTitleConfig_X.GetTitleData(\"{}\") was called", titleId);

	        std::string titleId = params->TitleId.ToString();
	        if (titleId != m_selectedTitleId)
	            return;

	        TitleAppearance* preset = getActivePreset();
	        if (!preset)
	            return;
	        params->ReturnValue = preset->toTitleData(params->TitleId);
	        LOG("Set custom FPlayerTitleData return value for \"{}\"", titleId);
	    });
	*/

	Hooks.hookEvent(Events::GFxData_StartMenu_TA_ProgressToMainMenu,
	    HookType::Post,
	    [this](std::string)
	    {
		    if (!*m_enabled)
			    return;
		    applySelectedAppearanceToUser();
	    });

	auto refreshPriTitlePresetsUsingHud = [this](ActorWrapper Caller, void* params, std::string eventName)
	{
		auto* caller = reinterpret_cast<AGFxHUD_TA*>(Caller.memory_address);
		if (!caller)
			return;

		refreshPriTitlePresets(caller);
	};

	Hooks.hookEvent(Events::GFxHUD_TA_HandleTeamChanged, HookType::Post, refreshPriTitlePresetsUsingHud);

	Hooks.hookEvent(Events::GFxData_PRI_TA_SetPlayerTitle, HookType::Post, [this](std::string) { refreshPriTitlePresets(); });

	Hooks.hookEvent(Events::EngineShare_X_EventPreLoadMap, HookType::Post, [this](std::string) { m_ingamePresets.clear(); });

	Hooks.hookEvent(Events::PlayerController_EnterStartState,
	    HookType::Pre,
	    [this](std::string)
	    {
		    if (!*m_enabled)
			    return;

		    auto* userPri = getUserGFxPRI();
		    if (!userPri)
			    return;

		    auto* title = getActivePreset();
		    if (!title)
			    return;

		    m_ingamePresets.addPreset(userPri, *title);

		    LOG("Added user gfxPri to m_ingameCustomPresets...");
	    });

	Hooks.hookEvent(Events::PlayerController_EnterStartState,
	    HookType::Post,
	    [this](ActorWrapper Caller, ...)
	    {
		    if (!*m_enabled)
			    return;

		    auto showTitleToOthers_cvar = getCvar(Cvars::showTitleToOthers);
		    if (!showTitleToOthers_cvar || !showTitleToOthers_cvar.getBoolValue())
			    return;

		    auto* caller = reinterpret_cast<APlayerController*>(Caller.memory_address);
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
	auto enabled_cvar = registerCvar_bool(Cvars::enabled, true);
	enabled_cvar.bindTo(m_enabled);
	enabled_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar)
	    {
		    bool enabled = updatedCvar.getBoolValue();
		    if (enabled)
			    GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(true); });
		    else
			    GAME_THREAD_EXECUTE({ removeUserCustomTitle(); });

		    LOG("Custom title {}", enabled ? "ENABLED" : "DISABLED");
	    });

	registerCvar_bool(Cvars::showOtherPlayerTitles, true).bindTo(m_showOtherPlayerTitles);
	registerCvar_bool(Cvars::showTitleToOthers, true).bindTo(m_showTitleToOthers);
	registerCvar_bool(Cvars::filterOtherPlayerTitles, false).bindTo(m_filterOtherPlayerTitles);
	registerCvar_bool(Cvars::useHueColorPicker, true).bindTo(m_useHueColorPicker);
	registerCvar_bool(Cvars::applyOthersTitleNotif, false).bindTo(m_notifyWhenApplyingOthersTitle);
	registerCvar_bool(Cvars::showEquippedTitleDetails, false).bindTo(m_showEquippedTitleDetails);

	registerCvar_number(Cvars::rgbSpeed, 0, true, -14, 9).bindTo(m_rgbSpeed);

	// commands
	registerCommand(Commands::toggleEnabled,
	    [this](...)
	    {
		    auto enabled_cvar = getCvar(Cvars::enabled);
		    if (!enabled_cvar)
			    return;
		    enabled_cvar.setValue(!enabled_cvar.getBoolValue());
	    });

	registerCommand(Commands::spawnCustomTitle,
	    [this](...)
	    {
		    GAME_THREAD_EXECUTE({
			    if (!*m_enabled)
			    {
				    Instances.SpawnNotification("Custom Title", "Spawning requires custom title to be enabled!", 4, true);
				    return;
			    }
			    spawnSelectedPreset();
		    });
	    });
}

// ##############################################################################################################
// ###############################################    FUNCTIONS    ##############################################
// ##############################################################################################################

// remove user's custom title (while keeping other players' custom titles in tact)
void TitlesComponent::removeUserCustomTitle()
{
	FUniqueNetId userId = Instances.GetUniqueID();

	// in-game
	auto* hud = getGFxHUD();
	if (hud)
	{
		for (auto* pri : hud->PRIData)
		{
			if (!validUObject(pri) || !sameId(pri->PlayerID, userId))
				continue;

			pri->HandleTitleChanged(pri->PRI); // resets to default title
			LOG("Restored original in-game title appearance for user PRI: {}", pri->PlayerName.ToString());
		}
	}

	// banner
	auto* pt = Instances.GetInstanceOf<UGFxData_PlayerTitles_TA>();
	if (pt)
	{
		pt->UpdatePlayerTitles(); // triggers the UpdateSelectedTitle hooks
		LOG("Restored OG banner appearance");
	}

	if (m_selectedTitleId.empty())
		return;
	applyPresetToBanner(m_currentOgAppearance);
}

void TitlesComponent::tickRGB() { GRainbowColor::TickRGB(*m_rgbSpeed, DEFAULT_RGB_SPEED); }

void TitlesComponent::handleUnload()
{
	// in-game
	auto* hud = getGFxHUD();
	if (hud)
	{
		for (auto* pri : hud->PRIData)
		{
			if (!validUObject(pri))
				continue;

			pri->HandleTitleChanged(pri->PRI); // resets to default title
			LOG("(unload) Restored OG in-game appearance for PRI: {}", pri->PlayerName.ToString());
		}
	}

	// restore equipped title's OG appearance...
	// banner
	auto* pt = Instances.GetInstanceOf<UGFxData_PlayerTitles_TA>();
	if (pt)
	{
		pt->UpdatePlayerTitles(); // triggers the UpdateSelectedTitle hooks
		LOG("(unload) Restored OG banner appearance");
	}

	if (m_selectedTitleId.empty())
		return;
	applyPresetToBanner(m_currentOgAppearance);
}

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
	for (const FPlayerTitleData& titleData : config->Titles)
	{
		if (titleData.Id == L"None" || titleData.Text.empty())
			continue;

		m_gameTitles.emplace_back(titleData.Id.GetDisplayIndex(), titleData);
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
	for (auto* pri : hud->PRIData)
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
		for (auto* pri : hud->PRIData)
		{
			if (!pri)
				continue;

			// skip applying preset to user PRI if custom title is disabled
			if (!*m_enabled && pri->PRI == hud->OwnerPRI)
				continue;

			auto it = m_ingamePresets.find(pri);
			if (it == m_ingamePresets.end())
				continue;

			applyPresetToPri(pri, it->second);
		}
	};

	// lil 0.1s delay to allow side effect functions from HandleTitleChanged to finish executing
	DELAY(0.1f, { applyExistingPresetsToPris(); }, applyExistingPresetsToPris);
}

TitleAppearance* TitlesComponent::getActivePreset(bool log)
{
	if (m_titlePresets.empty())
	{
		if (log)
			LOG("There are no existing title presets...");
		return nullptr;
	}
	if (m_activePresetIndex >= m_titlePresets.size())
	{
		if (log)
			LOGERROR("Active preset index is out of range: {}", m_activePresetIndex);
		return nullptr;
	}

	return &m_titlePresets[m_activePresetIndex];
}

void TitlesComponent::applySelectedAppearanceToUser(bool sendTitleSyncChat)
{
	TitleAppearance* title = getActivePreset();
	if (!title)
		return;

	// apply to banner
	applyPresetToBanner(*title);

	// apply to pri (in-game)
	auto* pri = getUserGFxPRI();
	if (!pri)
		return;

	m_ingamePresets.addPreset(pri, *title);
	if (!title->useRGB())
		applyPresetToPri(pri, *title);

	// send title sync chat
	if (sendTitleSyncChat)
		sendTitleDataChat(*title);
}

void TitlesComponent::applyPresetFromChatData(std::string data, const FChatMessage& msg, AHUDBase_TA* caller)
{
#define DONT_APPLY_TO_USER

	if (!validUObject(caller) || !caller->IsA<AGFxHUD_TA>())
		return;
	auto* hud = static_cast<AGFxHUD_TA*>(caller);

	if (!*m_showOtherPlayerTitles)
		return;

	LOG("Recieved chat data string: \"{}\"", Format::EscapeBraces(data));

#ifdef DONT_APPLY_TO_USER
	if (!validUObject(msg.PRI))
	{
		LOGERROR("msg.PRI from title sync chat is invalid");
		return;
	}
	FUniqueNetId& senderId = msg.PRI->UniqueId;
	FUniqueNetId  userId   = Instances.GetUniqueID();
	if (sameId(senderId, userId)) // skip applying title appearance if chat was from user
	{
		LOG("Title sync chat from {} is user. Skipped applying title appearance...", msg.PRI->PlayerName.ToString());
		return;
	}
#endif

	auto* gfxPri = getGFxPriFromChatData(msg.PRI, hud);
	if (!gfxPri)
		return;

	auto appearanceOpt = TitleAppearance::fromEncodedStr(data);
	if (!appearanceOpt)
	{
		LOGERROR("Unable to parse title appearance from string: \"{}\"", Format::EscapeBraces(data));
		return;
	}
	TitleAppearance appearance = *appearanceOpt;

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
				LOGERROR("Check for update HTTP Request Failed! Response code: {}", code);
				return;
			}

			std::string     oldText = appearance.getText();
			TitleAppearance title =
			    appearance; // bc capture variables are const references i think, and we want non-const so we can call setText(...)
			title.setText(result);

			GAME_THREAD_EXECUTE(
			    {
				    if (!gfxPri)
					    return;

				    LOG("Censored title text for {}: \"{}\" ---> \"{}\"",
				        senderName,
				        Format::EscapeBraces(oldText),
				        Format::EscapeBraces(title.getText()));

				    m_ingamePresets.addPreset(gfxPri, title);
				    refreshPriTitlePresets(hud);
				    // applyPresetToPri(gfxPri, title);

				    LOG(successMsg);
				    if (*m_notifyWhenApplyingOthersTitle)
					    Instances.SpawnNotification("custom title", successMsg, 3, true);
			    }

			    ,
			    gfxPri,
			    oldText,
			    title,
			    senderName,
			    successMsg,
			    hud);
		};

		HttpWrapper::SendCurlRequest(req, responseCallback);

		LOG("Sent curl request: \"{}\"", Format::EscapeBraces(req.url));
	}
	else
	{
		m_ingamePresets.addPreset(gfxPri, appearance);
		refreshPriTitlePresets(hud);
		// applyPresetToPri(gfxPri, appearance);

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
			std::string log    = i == m_activePresetIndex ? preset.getText() + "\t<---" : preset.getText();
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
	if (titleData.Id != customTitleId) // aka if config doesnt contain a FPlayerTitleData with the given ID... then we add it
	{
		titleData.Id        = customTitleId;
		titleData.Category  = L"XP";
		titleData.Text      = selectedPreset->getTextFStr();
		titleData.Color     = selectedPreset->getTextFColor();
		titleData.GlowColor = selectedPreset->getGlowFColor();
		config->Titles.push_back(titleData);

		titleData = config->GetTitleData(customTitleId);
	}

	FPlayerTitleData& spawnableTitle = getTitleFromConfig(config->Titles.size() - 1, config);

	spawnableTitle.Text      = selectedPreset->getTextFStr();
	spawnableTitle.Color     = selectedPreset->getTextFColor();
	spawnableTitle.GlowColor = selectedPreset->getGlowFColor();

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

	FName name{id};

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
	attributes.push_back({L"TitleId", spawn_id}); // "TitleId" FNameentryId is 41313

	LOG("animation: {}", animation);
	if (!animation)
	{
		auto* noNotify = Instances.GetDefaultInstanceOf<UProductAttribute_NoNotify_TA>();
		if (noNotify)
		{
			attributes.push_back(noNotify->InstanceOnlineProductAttribute());
			LOG("Added UProductAttribute_NoNotify_TA to title attributes array");
		}
		else
			LOGERROR("UProductAttribute_NoNotify_TA instance is null");
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
	FString fstr{spawn_id.GetDisplayNameEntry().GetWideName()};
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
	FColor white{255, 255, 255, 255};
	m_titlePresets.emplace_back(std::format("{{legend}} title preset {} {{diamond}}", m_titlePresets.size() + 1), white, white);
	m_activePresetIndex = m_titlePresets.size() - 1;
	applySelectedAppearanceToUser();
}

void TitlesComponent::deletePreset(int index)
{
	if (m_titlePresets.empty())
		return;
	m_titlePresets.erase(m_titlePresets.begin() + index);                         // erase binding at given index
	m_activePresetIndex = m_titlePresets.empty() ? 0 : m_titlePresets.size() - 1; // reset selected binding index
}

void TitlesComponent::writePresetsToJson(bool notification) const
{
	json j;
	j["presets"] = json::array();

	for (const auto& preset : m_titlePresets)
		j["presets"].push_back(preset.toJson());

	j["activePresetIndex"] = m_activePresetIndex;

	std::string msg;
	if (Files::write_json(m_titlePresetsJson, j))
		msg = std::format("Saved title presets to \"{}\"", m_titlePresetsJson.filename().string());
	else
		msg = std::format("[ERROR] Failed to save title presets to \"{}\"", m_titlePresetsJson.filename().string());
	LOG(msg);

	if (notification)
		GAME_THREAD_EXECUTE({ Instances.SpawnNotification("custom title", msg, 3.0f, true); }, msg);
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
	auto* lp = Instances.IULocalPlayer();
	if (!validUObject(lp))
	{
		LOGERROR("Instances.IULocalPlayer() is null");
		return nullptr;
	}

	auto* pc = lp->Actor;
	if (!validUObject(pc))
	{
		LOGERROR("lp->Actor is null");
		return nullptr;
	}

	if (!validUObject(pc->myHUD) || !pc->myHUD->IsA<AGFxHUD_TA>())
	{
		// LOGERROR("pc->myHUD is null or invalid"); // <--- gets called on every title text/color change while in main menu... cluttering
		// console :(
		return nullptr;
	}

	return static_cast<AGFxHUD_TA*>(pc->myHUD);
}

UGFxData_PRI_TA* TitlesComponent::getUserGFxPRI()
{
	auto* gfxHud = getGFxHUD();
	if (!gfxHud)
		return nullptr;

	if (!validUObject(gfxHud->OwnerPRI))
	{
		LOGERROR("gfxHud->OwnerPRI is null or invalid");
		return nullptr;
	}

	int idx = gfxHud->GetPRIDataIndex(gfxHud->OwnerPRI); // or use any other player's PRI here (like... maybe from a chat message ;D)
	if (idx >= gfxHud->PRIData.size())
		return nullptr;

	auto* userGfxPri = gfxHud->PRIData.at(idx);
	if (!validUObject(userGfxPri))
		return nullptr;

	return userGfxPri;
}

UGFxData_PRI_TA* TitlesComponent::getGFxPriFromChatData(APlayerReplicationInfo* priBase, AHUDBase_TA* hudBase)
{
	if (!validUObject(priBase))
	{
		LOGERROR("APlayerReplicationInfo* from chat data is invalid");
		return nullptr;
	}
	if (!validUObject(hudBase))
		return nullptr;

	if (!priBase->IsA<APRI_TA>())
	{
		LOGERROR("APlayerReplicationInfo instance isn't a APRI_TA");
		return nullptr;
	}
	if (!hudBase->IsA<AGFxHUD_TA>())
	{
		LOGERROR("AHUDBase_TA instance isn't a AGFxHUD_TA");
		return nullptr;
	}

	auto* pri = static_cast<APRI_TA*>(priBase);
	auto* hud = static_cast<AGFxHUD_TA*>(hudBase);

	auto gfxPriIndex = hud->GetPRIDataIndex(pri);
	if (gfxPriIndex >= hud->PRIData.size())
	{
		LOGERROR("PRIData index is out of bounds");
		return nullptr;
	}

	auto* gfxPri = hud->PRIData.at(gfxPriIndex);
	if (!validUObject(gfxPri))
		return nullptr;

	return gfxPri;
}

void TitlesComponent::applyPresetToPri(UGFxData_PRI_TA* pri, const TitleAppearance& title)
{
	if (!validUObject(pri))
		return;

	GfxWrapper gfxPri{pri};

	gfxPri.set_string(L"XPTitle", title.getTextFStr());
	gfxPri.set_int(L"TitleColor", title.getIntTextColor());
	gfxPri.set_int(L"TitleGlowColor", title.getIntGlowColor());

	LOG("Applied title preset for {}... (UGFxData_PRI_TA: {})", pri->PlayerName.ToString(), Format::ToHexString(pri));
}

void TitlesComponent::applyPresetToBanner(const TitleAppearance& title, UGFxDataRow_X* gfxRow, bool log)
{
	if (!validUObject(gfxRow))
	{
		gfxRow = Instances.GetInstanceOf<UGFxData_PlayerTitles_TA>();
		if (!validUObject(gfxRow))
			return;
	}

	GfxWrapper gfx{gfxRow};
	auto*      ds = gfx.get_datastore();
	if (!validUObject(ds))
		return;

	int32_t row = ds->GetValue(L"PlayerTitles", NULL, L"SelectedTitle").I; // selected title index is the row index
	// ... or:
	// int32_t row = pt->SelectedTitle;
	FName table{L"PlayerTitlesPlayerTitles"};

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
		auto* lp = Instances.IULocalPlayer();
		if (!lp)
		{
			LOGERROR("Instances.IULocalPlayer() is null");
			return;
		}

		if (!lp->Actor || !lp->Actor->IsA<APlayerController_TA>())
		{
			LOGERROR("lp->Actor is null or isnt a APlayerController_TA");
			return;
		}
		pc = lp->Actor;
	}

	auto* pcTA = static_cast<APlayerController_TA*>(pc);

	std::string titleDataStr = appearance.toEncodedString();

	pcTA->Say_TA(FString::create(titleDataStr), EChatChannel::EChatChannel_Match, FUniqueNetId{}, true, false);

	LOG("Sent title data chat: \"{}\"", Format::EscapeBraces(titleDataStr));
}

// ##############################################################################################################
// ##########################################   DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void TitlesComponent::display_titlePresetList()
{
	{
		GUI::ScopedChild c{"List", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f)};

		ImGui::TextUnformatted("Presets:");
		ImGui::Separator();
		auto titleCustomizationsSize = m_titlePresets.size();

		for (int i = 0; i < titleCustomizationsSize; ++i)
		{
			auto& appearancePreset = m_titlePresets[i];

			GUI::ScopedID id{&appearancePreset};

			ImVec4 textCol = appearancePreset.getImGuiTextColor();
			textCol.w      = 1.0f; // set alpha channel to 1 when rendering menu so title is always visible
			ImGui::PushStyleColor(ImGuiCol_Text, textCol);

			if (ImGui::Selectable(appearancePreset.m_text.c_str(), m_activePresetIndex == i))
			{
				m_activePresetIndex = i;
				GAME_THREAD_EXECUTE({
					if (*m_enabled)
						applySelectedAppearanceToUser();
					writePresetsToJson(false); // to save the active preset index
				});
			}

			ImGui::PopStyleColor();
		}
	}

	GUI::Spacing(2);

	{
		GUI::ScopedChild c{"AddPreset"};

		if (ImGui::Button("Add New Preset", ImGui::GetContentRegionAvail()))
		{
			GAME_THREAD_EXECUTE({
				addNewPreset();
				// writePresetsToJson();
			});
		}
	}
}

void TitlesComponent::display_titlePresetInfo()
{
	static ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel;
	colorEditFlags                            = *m_useHueColorPicker ? (ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel)
	                                                                 : ImGuiColorEditFlags_NoInputs;

	static ImGuiColorEditFlags noEditFlags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs;

	{
		GUI::ScopedChild c{"PrestInfo", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.8f)};

		if (!m_selectedTitleId.empty())
		{
			if (*m_showEquippedTitleDetails)
			{
				if (ImGui::CollapsingHeader("Equipped title info"))
				{
					static constexpr float spacing = 75.0f;

					ImGui::TextColored(GUI::Colors::LightGreen, "ID:");
					GUI::SameLineSpacing_absolute(spacing);
					ImGui::Text("%s", m_selectedTitleId.c_str());

					if (m_selectedTitleId != "None")
					{
						ImGui::TextColored(GUI::Colors::LightGreen, "Text:");
						GUI::SameLineSpacing_absolute(spacing);
						ImGui::Text("%s", m_currentOgAppearance.getText().c_str());

						ImGui::TextColored(GUI::Colors::LightGreen, "Color:");
						GUI::SameLineSpacing_absolute(spacing);

						float color[4] = {0, 0, 0, 0};
						if (m_currentOgAppearance.m_sameTextAndGlowColor)
						{
							m_currentOgAppearance.getTextColor(color);
							ImGui::ColorEdit4("##singleColor", &color[0], noEditFlags);
						}
						else
						{
							m_currentOgAppearance.getTextColor(color);
							ImGui::ColorEdit4("Text", &color[0], noEditFlags);

							GUI::SameLineSpacing_relative(30.0f);

							float glowColor[4] = {0, 0, 0, 0};
							m_currentOgAppearance.getGlowColor(color);
							ImGui::ColorEdit4("Glow", &color[0], noEditFlags);
						}
					}

					GUI::Spacing(2);
				}
			}
			else
			{
				ImGui::TextColored(GUI::Colors::LightGreen, "Equipped title: ");
				ImGui::SameLine();
				ImGui::Text("%s", m_selectedTitleId.c_str());
				GUI::ToolTip("The ID of your real title\n\n(aka what's beneath the custom appearance)");
			}

			ImGui::Separator();

			GUI::Spacing(4);
		}

		if (m_titlePresets.empty())
		{
			ImGui::Text("Add a title preset....");
			return;
		}
		if (m_activePresetIndex >= m_titlePresets.size())
			m_activePresetIndex = 0;

		TitleAppearance& appearance = m_titlePresets[m_activePresetIndex];

		// text
		char textBuffer[MAX_TEXT_LENGTH + 1]; // +1 for null terminator
		std::strncpy(textBuffer, appearance.m_text.c_str(), MAX_TEXT_LENGTH);
		textBuffer[MAX_TEXT_LENGTH] = '\0'; // Make sure it's null-terminated

		if (std::strlen(textBuffer) >= MAX_TEXT_LENGTH)
			ImGui::TextColored(GUI::Colors::Yellow, "Max text length: %i characters", MAX_TEXT_LENGTH);

		ImGui::Text("%d/%d characters", static_cast<int>(std::strlen(textBuffer)), MAX_TEXT_LENGTH);
		if (ImGui::InputText("Text", textBuffer, MAX_TEXT_LENGTH + 1))
		{
			appearance.m_text = textBuffer;

			GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
		}

		GUI::SameLineSpacing_relative(20);

		if (ImGui::Checkbox("RGB", &appearance.m_useRGB))
		{
			GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
		}

		if (!appearance.m_useRGB)
		{
			GUI::Spacing(4);

			if (ImGui::Checkbox("Same text & glow color", &appearance.m_sameTextAndGlowColor))
			{
				if (appearance.m_sameTextAndGlowColor)
				{
					appearance.setGlowColor(appearance.m_textColor);

					GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
				}
			}

			GUI::Spacing(2);

			if (appearance.m_sameTextAndGlowColor)
			{
				float color[4] = {0, 0, 0, 0};
				appearance.getTextColor(color);

				if (ImGui::ColorEdit4("Color", &color[0], colorEditFlags))
				{
					appearance.setTextColor(color);
					appearance.setGlowColor(color);

					GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
				}
			}
			else
			{
				// text color picker
				float textCol[4] = {0, 0, 0, 0};
				appearance.getTextColor(textCol);

				if (ImGui::ColorEdit4("Text color", &textCol[0], colorEditFlags))
				{
					appearance.setTextColor(textCol);
					GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
				}

				GUI::Spacing(2);

				// glow color picker
				float glowCol[4] = {0, 0, 0, 0};
				appearance.getGlowColor(glowCol);

				if (ImGui::ColorEdit4("Glow color", &glowCol[0], colorEditFlags))
				{
					appearance.setGlowColor(glowCol);
					GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
				}
			}
		}

		GUI::Spacing(8);

		display_gameTitlesDropdown();

		GUI::SameLineSpacing_relative(20);

		if (ImGui::Button("Refresh"))
		{
			GAME_THREAD_EXECUTE({
				updateGameTitleAppearances(nullptr, true);
				Instances.SpawnNotification("custom title", "Updated game title presets", 3);
			});
		}

		ImGui::Text("%zu presets found", m_gameTitles.size());
	}

	GUI::Spacing(2);

	{
		GUI::ScopedChild c{"ButtonsSection"};

		{
			GUI::ScopedChild c{"SpawnButton", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f)};
			{
				GUI::ScopedDisabled disablility{m_enabled};

				if (ImGui::Button("Spawn", ImGui::GetContentRegionAvail()))
					_globalCvarManager->executeCommand(Commands::spawnCustomTitle.name);
			}
			if (*m_enabled)
				GUI::ToolTip("Press OK at the spawn prompt. Pressing EQUIP NOW can cause buggy behavior.\n\n"
				             "TIP - Bind this command to a key: %s",
				    Commands::spawnCustomTitle.name);
			else
				GUI::ToolTip("Spawning requires custom title to be enabled");
		}

		{
			GUI::ScopedChild c{"SaveOrDelete"};

			{
				GUI::ScopedChild c{"SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0)};

				if (ImGui::Button("Save", ImGui::GetContentRegionAvail()))
				{
					GAME_THREAD_EXECUTE({ writePresetsToJson(); });
				}
			}

			ImGui::SameLine();

			{
				GUI::ScopedChild c{"DeleteButton", ImGui::GetContentRegionAvail()};

				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

				if (ImGui::Button("Delete Preset", ImGui::GetContentRegionAvail()))
				{
					GAME_THREAD_EXECUTE({
						deletePreset(m_activePresetIndex); // <--- index param may not be necessary (bc the index would
						                                   // always just be m_activePresetIndex)
						writePresetsToJson(false);
					});
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

	char searchBuffer[128] = ""; // text buffer for search input

	if (ImGui::BeginSearchableCombo("Game presets", currentTitleText.c_str(), searchBuffer, sizeof(searchBuffer), "search..."))
	{
		std::string searchQuery = Format::ToLower(searchBuffer); // convert search text to lower

		for (const auto& title : m_gameTitles)
		{
			GUI::ScopedID id{&title};

			const std::string& titleText      = title.getText();
			const std::string  titleTextLower = Format::ToLower(title.getText());

			if (!searchQuery.empty() && (titleTextLower.find(searchQuery) == std::string::npos))
				continue;

			ImGui::PushStyleColor(ImGuiCol_Text, title.getImGuiTextColor());

			if (ImGui::Selectable(titleText.c_str(), titleText == currentTitleText))
			{
				*currentTitle = static_cast<TitleAppearance>(title);
				GAME_THREAD_EXECUTE({ applySelectedAppearanceToUser(); });
			}

			ImGui::PopStyleColor();
		}

		ImGui::EndCombo();
	}
}

// checkbox that sets the value of a bool cvar, which should call the bound callback function, which should
// apply/remove custom title
void TitlesComponent::display_enabledCheckbox()
{
	auto enabled_cvar = getCvar(Cvars::enabled);
	bool enabled      = enabled_cvar.getBoolValue();
	if (ImGui::Checkbox("Enable", &enabled))
		enabled_cvar.setValue(enabled);
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

bool TitlesComponent::test2() { return true; }

// ##############################################################################################################
// #########################################    TitleAppearance    ##############################################
// ##############################################################################################################

void TitleAppearance::updateFromPlayerTitleData(const FPlayerTitleData& data)
{
	m_text                 = data.Text.ToString();
	m_textColor            = data.Color;
	m_glowColor            = data.GlowColor;
	m_sameTextAndGlowColor = (m_textColor.R == m_glowColor.R) && (m_textColor.G == m_glowColor.G) && (m_textColor.B == m_glowColor.B) &&
	                         (m_textColor.A == m_glowColor.A);
}

std::optional<TitleAppearance> TitleAppearance::fromEncodedStr(const std::string& str)
{
	auto parts = Format::SplitStr(str, '|');
	if (parts.size() != 3 && parts.size() != 4)
		return std::nullopt; // invalid format

	TitleAppearance appearance;
	appearance.setText(parts[0]);
	appearance.setTextColor(Colors::hexToFColor(parts[1]));

	if (parts[2] == "-")
	{
		appearance.setGlowColor(appearance.getTextFColor());
		appearance.setSameTextAndGlowColor(true);
	}
	else
	{
		appearance.setGlowColor(Colors::hexToFColor(parts[2]));
		appearance.setSameTextAndGlowColor(false);
	}

	if (parts.size() == 4)
		appearance.setUseRGB(parts[3] == "1");
	else
		LOG("WARNING: Looks like someone is using an old version of the mod that doesn't support RGB (pre v1.1.0)");

	return appearance;
}

FPlayerTitleData TitleAppearance::toTitleData(const FName& id) const
{
	FPlayerTitleData data{};
	data.Text      = getTextFStr();
	data.Id        = id;
	data.Category  = L"None";
	data.Color     = getTextFColor();
	data.GlowColor = getGlowFColor();
	return data;
}

void TitleAppearance::getTextColor(float (&outArray)[4]) const
{
	outArray[0] = m_textColor.R / 255.0f;
	outArray[1] = m_textColor.G / 255.0f;
	outArray[2] = m_textColor.B / 255.0f;
	outArray[3] = m_textColor.A / 255.0f;
}

void TitleAppearance::getGlowColor(float (&outArray)[4]) const
{
	outArray[0] = m_glowColor.R / 255.0f;
	outArray[1] = m_glowColor.G / 255.0f;
	outArray[2] = m_glowColor.B / 255.0f;
	outArray[3] = m_glowColor.A / 255.0f;
}

ImVec4 TitleAppearance::getImGuiTextColor() const
{
	return {m_textColor.R / 255.0f, m_textColor.G / 255.0f, m_textColor.B / 255.0f, m_textColor.A / 255.0f};
}

ImVec4 TitleAppearance::getImGuiGlowColor() const
{
	return {m_glowColor.R / 255.0f, m_glowColor.G / 255.0f, m_glowColor.B / 255.0f, m_glowColor.A / 255.0f};
}

int32_t TitleAppearance::getIntTextColor() const
{
	if (m_useRGB)
		return GRainbowColor::GetDecimal();
	else
		return UObject::ColorToInt(m_textColor);
}

int32_t TitleAppearance::getIntGlowColor() const
{
	if (m_useRGB)
		return GRainbowColor::GetDecimal();
	else
		return UObject::ColorToInt(m_glowColor);
}

std::string TitleAppearance::getDebugTextColorStr() const
{
	return std::format("R:{}-G:{}-B:{}-A:{}", m_textColor.R, m_textColor.G, m_textColor.B, m_textColor.A);
}

std::string TitleAppearance::getDebugGlowColorStr() const
{
	return std::format("R:{}-G:{}-B:{}-A:{}", m_glowColor.R, m_glowColor.G, m_glowColor.B, m_glowColor.A);
}

// Will output "title:My custom text|FF8040FF|00FF00FF|0" ... or "title:My custom text|FF8040FF|-|1" if m_sameTextAndGlowColor is true
std::string TitleAppearance::toEncodedString() const
{
	return std::format("title:{}|{}|{}|{}",
	    m_text,
	    Colors::fcolorToHex(m_textColor),
	    m_sameTextAndGlowColor ? "-" : Colors::fcolorToHex(m_glowColor),
	    m_useRGB ? "1" : "0");
}

json TitleAppearance::toJson() const
{
	json j;

	j["text"]                 = m_text;
	j["textColor"]            = Colors::fcolorToHexRGBA(m_textColor);
	j["glowColor"]            = Colors::fcolorToHexRGBA(m_glowColor);
	j["sameTextAndGlowColor"] = m_sameTextAndGlowColor;
	j["useRGB"]               = m_useRGB;

	return j;
}

void TitleAppearance::fromJson(const json& j)
{
	m_text                 = j.value("text", "{legend} {grandchampion} example {gold} {champion}");
	m_textColor            = Colors::hexRGBAtoFColor(j.value("textColor", "0xFFFFFFFF"));
	m_glowColor            = Colors::hexRGBAtoFColor(j.value("glowColor", "0xFFFFFFFF"));
	m_sameTextAndGlowColor = j.value("sameTextAndGlowColor", true);
	m_useRGB               = j.value("useRGB", false);
}

bool TitleAppearance::operator==(const TitleAppearance& other) const
{
	return (m_text == other.m_text) &&
	       (m_textColor.R == other.m_textColor.R && m_textColor.G == other.m_textColor.G && m_textColor.B == other.m_textColor.B) &&
	       (m_glowColor.R == other.m_glowColor.R && m_glowColor.G == other.m_glowColor.G && m_glowColor.B == other.m_glowColor.B);
}

// ##############################################################################################################
// #######################################    InGamePresetManager    ############################################
// ##############################################################################################################

void InGamePresetManager::addPreset(UGFxData_PRI_TA* player, const TitleAppearance& appearance)
{
	auto [it, inserted] = m_allPresets.emplace(player, appearance);
	if (!inserted)
		it->second = appearance;

	if (appearance.useRGB())
	{
		if (!inRGBList(it))
			m_rgbPresets.push_back(it);
	}
	else
		removeFromRGBList(it);
}

void InGamePresetManager::removePreset(UGFxData_PRI_TA* player)
{
	auto it = m_allPresets.find(player);
	if (it == m_allPresets.end())
		return;
	removeFromRGBList(it);
	m_allPresets.erase(it);
}

bool InGamePresetManager::inRGBList(Iterator it) { return std::find(m_rgbPresets.begin(), m_rgbPresets.end(), it) != m_rgbPresets.end(); }

void InGamePresetManager::removeFromRGBList(Iterator it)
{
	m_rgbPresets.erase(std::remove(m_rgbPresets.begin(), m_rgbPresets.end(), it), m_rgbPresets.end());
}

bool InGamePresetManager::contains(UGFxData_PRI_TA* player) const { return m_allPresets.contains(player); }

InGamePresetManager::ConstIterator InGamePresetManager::find(UGFxData_PRI_TA* player) const { return m_allPresets.find(player); }
InGamePresetManager::ConstIterator InGamePresetManager::end() const { return m_allPresets.end(); }

void InGamePresetManager::clear()
{
	m_allPresets.clear();
	m_rgbPresets.clear();
}

bool InGamePresetManager::rgbPresetsExist() const { return !m_rgbPresets.empty(); }

class TitlesComponent Titles{};