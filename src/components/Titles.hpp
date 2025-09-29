#pragma once
#include "Component.hpp"
#include <optional>
#include <string_view>

// class TitleAppearance // i wan make class but too lazy switch to using getters/setters in TitlesComponent class
struct TitleAppearance
{
	std::string m_text                 = "{legend} {grandchampion} example {gold} {champion}";
	FColor      m_textColor            = {255, 255, 255, 255};
	FColor      m_glowColor            = {255, 255, 255, 255};
	bool        m_sameTextAndGlowColor = true;
	bool        m_useRGB               = false;

public:
	TitleAppearance() {}
	TitleAppearance(const std::string& text, const FColor& textColor, const FColor& glowColor, bool sameTextAndGlow = true)
	    : m_text(text), m_textColor(textColor), m_glowColor(glowColor), m_sameTextAndGlowColor(sameTextAndGlow)
	{
	}
	TitleAppearance(const FPlayerTitleData& data) { updateFromPlayerTitleData(data); }

	void                                  updateFromPlayerTitleData(const FPlayerTitleData& data);
	static std::optional<TitleAppearance> fromEncodedStr(const std::string& str); // factory func
	FPlayerTitleData                      toTitleData(const FName& id) const;

	// getters
	std::string getText() const { return m_text; }
	FString     getTextFStr() const { return FString::create(m_text); }
	FColor      getTextFColor() const { return m_textColor; }
	FColor      getGlowFColor() const { return m_glowColor; }
	void        getTextColor(float (&outArray)[4]) const;
	void        getGlowColor(float (&outArray)[4]) const;
	ImVec4      getImGuiTextColor() const;
	ImVec4      getImGuiGlowColor() const;
	int32_t     getIntTextColor() const;
	int32_t     getIntGlowColor() const;
	bool        useRGB() const { return m_useRGB; }

	// setters
	void setText(const std::string& str) { m_text = str; }
	void setTextColor(const FColor& newCol) { m_textColor = newCol; }
	void setTextColor(const float (&newCol)[4]) { m_textColor = Colors::toFColor(newCol); }
	void setGlowColor(const FColor& newCol) { m_glowColor = newCol; }
	void setGlowColor(const float (&newCol)[4]) { m_glowColor = Colors::toFColor(newCol); }
	void setSameTextAndGlowColor(bool val) { m_sameTextAndGlowColor = val; }
	void setUseRGB(bool val) { m_useRGB = val; }

	void        clear();
	std::string getDebugTextColorStr() const;
	std::string getDebugGlowColorStr() const;
	std::string toEncodedString() const;
	json        toJson() const;
	void        fromJson(const json& j);
	bool        operator==(const TitleAppearance& other) const;
};

struct GameTitleAppearance : TitleAppearance
{
	int32_t idFnameEntry = -1;

	GameTitleAppearance() {}
	GameTitleAppearance(int32_t id, const FPlayerTitleData& data) : TitleAppearance(data), idFnameEntry(id) {}
};

class InGamePresetManager
{
public:
	using PresetMap     = std::unordered_map<UGFxData_PRI_TA*, TitleAppearance>;
	using Iterator      = PresetMap::iterator;
	using ConstIterator = PresetMap::const_iterator;

	void          addPreset(UGFxData_PRI_TA* player, const TitleAppearance& appearance);
	void          removePreset(UGFxData_PRI_TA* player);
	bool          contains(UGFxData_PRI_TA* player) const;
	ConstIterator find(UGFxData_PRI_TA* player) const;
	ConstIterator end() const;
	void          clear();
	bool          rgbPresetsExist() const;

	// Per-frame iteration over RGB presets
	template <typename Func> void forEachRGBPreset(Func&& func)
	{
		for (auto it : m_rgbPresets)
			func(it->first, it->second);
	}

private:
	PresetMap             m_allPresets;
	std::vector<Iterator> m_rgbPresets;

	bool inRGBList(Iterator it);
	void removeFromRGBList(Iterator it);
};

class TitlesComponent : Component<TitlesComponent>
{
public:
	TitlesComponent() {}
	~TitlesComponent() {}

	static constexpr std::string_view componentName = "Titles";
	void                              init(const std::shared_ptr<GameWrapper>& gw);

private:
	void initHooks();
	void setFilePaths();
	void initCvars();

private:
	static constexpr int MAX_TEXT_LENGTH = 64; // max characters that'll show up in UI, excluding {brace} symbols

	fs::path m_pluginFolder;
	fs::path m_titlePresetsJson;

	// cvar values
	std::shared_ptr<bool> m_enabled                       = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_showOtherPlayerTitles         = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_showTitleToOthers             = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_filterOtherPlayerTitles       = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_useHueColorPicker             = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_notifyWhenApplyingOthersTitle = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_showEquippedTitleDetails      = std::make_shared<bool>(false);
	std::shared_ptr<int>  m_rgbSpeed                      = std::make_shared<int>(0);

	int                              m_activePresetIndex = 0;
	TitleAppearance                  m_currentOgAppearance;
	std::vector<TitleAppearance>     m_titlePresets;
	std::vector<GameTitleAppearance> m_gameTitles; // vector preserves the order that they were found in the title config
	InGamePresetManager              m_ingamePresets;

	bool        m_shouldOverwriteGetTitleDataReturnVal = false; // what a name
	std::string m_selectedTitleId;

private:
	void            addNewPreset();
	void            tickRGB();
	void            deletePreset(int index);
	void            writePresetsToJson(bool notification = true) const;
	void            addPresetsFromJson();
	inline fs::path getPluginFolder() const { return m_pluginFolder; }

	void removeUserCustomTitle();

	void refreshPriTitlePresets(AGFxHUD_TA* hud = nullptr);
	void updateGameTitleAppearances(UTitleConfig_X* config = nullptr, bool forceSearch = false);

	FName getCustomTitleId();

	bool spawn(const std::string& spawn_id, bool animation = true, const std::string& spawn_msg = "");
	bool spawn(const FName& spawn_id, bool animation = true, const std::string& spawn_msg = "");
	bool spawn(const FString& spawn_id, bool animation = true, const std::string& spawn_msg = "");

	// static
	static void applyPresetToBanner(const TitleAppearance& title, UGFxDataRow_X* gfxRow = nullptr, bool log = false);
	static void applyPresetToPri(UGFxData_PRI_TA* pri, const TitleAppearance& title);

	static FPlayerTitleData& getTitleFromConfig(int index, UTitleConfig_X* config = nullptr);
	static UTitleConfig_X*   getTitleConfig(bool forceSearch = false);
	static AGFxHUD_TA*       getGFxHUD();
	// static UGFxData_PlayerTitles_TA* getGFxPlayerTitles();	// banner
	static UGFxData_PRI_TA* getUserGFxPRI(); // in-game
	static UGFxData_PRI_TA* getGFxPriFromChatData(APlayerReplicationInfo* priBase, AHUDBase_TA* hudBase);

	static void sendTitleDataChat(const TitleAppearance& appearance, APlayerController* pc = nullptr);

public:
	static constexpr int DEFAULT_RGB_SPEED = 0;

	void             handleUnload();
	TitleAppearance* getActivePreset(bool log = true);

	void spawnSelectedPreset(bool log = false);
	void applySelectedAppearanceToUser(bool sendTitleSyncChat = false);
	void applyPresetFromChatData(std::string data, const FChatMessage& msg, AHUDBase_TA* caller);

	// testing
	bool test1();
	bool test2();

public:
	// gui
	void display_titlePresetList();
	void display_titlePresetInfo();
	void display_gameTitlesDropdown();
	void display_enabledCheckbox();
};

extern class TitlesComponent Titles;