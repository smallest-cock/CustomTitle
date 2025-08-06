#pragma once
#include "Component.hpp"

// returns a AARRGGBB packed 32-bit int
inline uint32_t PackColor(const FColor& col)
{
	return (static_cast<uint32_t>(col.A) << 24) | (static_cast<uint32_t>(col.R) << 16) | (static_cast<uint32_t>(col.G) << 8) |
	       static_cast<uint32_t>(col.B);
}

// expects a AARRGGBB packed 32-bit int
inline FColor UnpackColor(uint32_t packed)
{
	return {
	    static_cast<uint8_t>(packed & 0xFF),         // B
	    static_cast<uint8_t>((packed >> 8) & 0xFF),  // G
	    static_cast<uint8_t>((packed >> 16) & 0xFF), // R
	    static_cast<uint8_t>((packed >> 24) & 0xFF)  // A
	};
}

// class TitleAppearance // i wan make class but too lazy switch to using getters/setters in TitlesComponent class
struct TitleAppearance
{
	std::string m_text                 = "{legend} {grandchampion} example {gold} {champion}";
	FColor      m_textColor            = {255, 255, 255, 255};
	FColor      m_glowColor            = {255, 255, 255, 255};
	bool        m_sameTextAndGlowColor = true;

public:
	TitleAppearance() {}
	TitleAppearance(const std::string& text, const FColor& textColor, const FColor& glowColor, bool sameTextAndGlow = true)
	    : m_text(text), m_textColor(textColor), m_glowColor(glowColor), m_sameTextAndGlowColor(sameTextAndGlow)
	{
	}
	TitleAppearance(const FPlayerTitleData& data) { updateFromPlayerTitleData(data); }

	void updateFromPlayerTitleData(const FPlayerTitleData& data)
	{
		m_text                 = data.Text.ToString();
		m_textColor            = data.Color;
		m_glowColor            = data.GlowColor;
		m_sameTextAndGlowColor = (m_textColor.R == m_glowColor.R) && (m_textColor.G == m_glowColor.G) && (m_textColor.B == m_glowColor.B) &&
		                         (m_textColor.A == m_glowColor.A);
	}

	FPlayerTitleData toTitleData(const FName& id) const
	{
		FPlayerTitleData data{};
		data.Text      = getTextFStr();
		data.Id        = id;
		data.Category  = L"None";
		data.Color     = getTextFColor();
		data.GlowColor = getGlowFColor();
		return data;
	}

	// getters
	std::string getText() const { return m_text; }

	FString getTextFStr() const { return FString::create(m_text); }

	FColor getTextFColor() const { return m_textColor; }

	void getTextColor(float (&outArray)[4]) const
	{
		outArray[0] = m_textColor.R / 255.0f;
		outArray[1] = m_textColor.G / 255.0f;
		outArray[2] = m_textColor.B / 255.0f;
		outArray[3] = m_textColor.A / 255.0f;
	}

	ImVec4 getImGuiTextColor() const
	{
		return {m_textColor.R / 255.0f, m_textColor.G / 255.0f, m_textColor.B / 255.0f, m_textColor.A / 255.0f};
	}

	int32_t getIntTextColor() const { return UObject::ColorToInt(m_textColor); }

	inline FColor getGlowFColor() const { return m_glowColor; }

	void getGlowColor(float (&outArray)[4]) const
	{
		outArray[0] = m_glowColor.R / 255.0f;
		outArray[1] = m_glowColor.G / 255.0f;
		outArray[2] = m_glowColor.B / 255.0f;
		outArray[3] = m_glowColor.A / 255.0f;
	}

	ImVec4 getImGuiGlowColor() const
	{
		return {m_glowColor.R / 255.0f, m_glowColor.G / 255.0f, m_glowColor.B / 255.0f, m_glowColor.A / 255.0f};
	}

	int32_t getIntGlowColor() const { return UObject::ColorToInt(m_glowColor); }

	// setters
	void setText(const std::string& str) { m_text = str; }

	void setTextColor(const FColor& newCol) { m_textColor = newCol; }
	void setTextColor(const float (&newCol)[4]) { m_textColor = Colors::toFColor(newCol); }

	void setGlowColor(const FColor& newCol) { m_glowColor = newCol; }
	void setGlowColor(const float (&newCol)[4]) { m_glowColor = Colors::toFColor(newCol); }

	void setSameTextAndGlowColor(bool val) { m_sameTextAndGlowColor = val; }

	// static functions (TODO: move to ModUtils)

	// FColor --> AARRGGBB hex string
	static std::string ColorToHex(const FColor& col)
	{
		uint32_t           packedInt = PackColor(col);
		std::ostringstream ss;
		ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << packedInt;
		return ss.str();
	}

	// AARRGGBB hex string --> FColor
	static FColor HexToColor(const std::string& hex)
	{
		if (hex.size() != 8)
		{
			LOG("ERROR: Color hex string \"{}\" isn't 8 characters. Falling back to white...");
			return {255, 255, 255, 255}; // fallback to white
		}

		uint32_t packed = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
		return UnpackColor(packed);
	}

	std::string getDebugTextColorStr() const
	{
		return std::format("R:{}-G:{}-B:{}-A:{}", m_textColor.R, m_textColor.G, m_textColor.B, m_textColor.A);
	}

	std::string getDebugGlowColorStr() const
	{
		return std::format("R:{}-G:{}-B:{}-A:{}", m_glowColor.R, m_glowColor.G, m_glowColor.B, m_glowColor.A);
	}

	// Will output "title:My custom text|FF8040FF|00FF00FF" ... or "title:My custom text|FF8040FF|-" if m_sameTextAndGlowColor is true
	std::string toEncodedString() const
	{
		constexpr const char* prefix = "title:";

		return std::format("{}{}|{}|{}", prefix, m_text, ColorToHex(m_textColor), m_sameTextAndGlowColor ? "-" : ColorToHex(m_glowColor));
	}

	json toJson() const
	{
		json j;

		j["text"]                 = m_text;
		j["textColor"]            = Colors::fcolorToHexRGBA(m_textColor);
		j["glowColor"]            = Colors::fcolorToHexRGBA(m_glowColor);
		j["sameTextAndGlowColor"] = m_sameTextAndGlowColor;

		return j;
	}

	void fromJson(const json& j)
	{
		m_text                 = j.value("text", "{legend} {grandchampion} example {gold} {champion}");
		m_textColor            = Colors::hexRGBAtoFColor(j.value("textColor", "0xFFFFFFFF"));
		m_glowColor            = Colors::hexRGBAtoFColor(j.value("glowColor", "0xFFFFFFFF"));
		m_sameTextAndGlowColor = j.value("sameTextAndGlowColor", true);
	}

	bool operator==(const TitleAppearance& other) const
	{
		return (m_text == other.m_text) &&
		       (m_textColor.R == other.m_textColor.R && m_textColor.G == other.m_textColor.G && m_textColor.B == other.m_textColor.B) &&
		       (m_glowColor.R == other.m_glowColor.R && m_glowColor.G == other.m_glowColor.G && m_glowColor.B == other.m_glowColor.B);
	}
};

struct GameTitleAppearance : TitleAppearance
{
	int32_t idFnameEntry = -1;

	GameTitleAppearance() {}
	GameTitleAppearance(int32_t id, const FPlayerTitleData& data) : TitleAppearance(data), idFnameEntry(id) {}
};

class TitlesComponent : Component<TitlesComponent>
{
public:
	TitlesComponent() {}
	~TitlesComponent() {}

	static constexpr const char* component_name = "Titles";
	void                         Initialize(std::shared_ptr<GameWrapper> gw);

private:
	void hookFunctions();
	void setFilePaths();
	void initCvars();

private:
	static constexpr int MAX_TEXT_LENGTH = 64; // max characters that'll show up in UI, excluding {brace} symbols

	fs::path m_pluginFolder;
	fs::path m_titlePresetsJson;

	// cvar flags
	std::shared_ptr<bool> m_showOtherPlayerTitles         = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_showTitleToOthers             = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_filterOtherPlayerTitles       = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_useHueColorPicker             = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_notifyWhenApplyingOthersTitle = std::make_shared<bool>(false);

	bool                             m_enabled           = false;
	int                              m_activePresetIndex = 0;
	TitleAppearance                  m_currentOgAppearance;
	std::vector<TitleAppearance>     m_titlePresets;
	std::vector<GameTitleAppearance> m_gameTitles; // vector preserves the order that they were found in the title config
	std::unordered_map<UGFxData_PRI_TA*, TitleAppearance>
	    m_ingameCustomPresets; // <--- or APRI_TA* as the key if that works better, with the chat data nd all that

	bool        m_shouldOverwriteGetTitleDataReturnVal = false; // what a name
	std::string m_selectedTitleId;

	// experimental
	std::vector<UGFxData_PRI_TA*> m_playersWithRGBPresets; // TODO: use in similar way to m_ingameCustomPresets

private:
	void            addNewPreset();
	void            deletePreset(int index);
	void            writePresetsToJson(bool notification = true) const;
	void            addPresetsFromJson();
	inline fs::path getPluginFolder() const { return m_pluginFolder; }

	void refreshPriTitlePresets(AGFxHUD_TA* hud = nullptr);
	void updateGameTitleAppearances(UTitleConfig_X* config = nullptr, bool forceSearch = false);

	FName getCustomTitleId();

	bool spawn(const std::string& spawn_id, bool animation = true, const std::string& spawn_msg = "");
	bool spawn(const FName& spawn_id, bool animation = true, const std::string& spawn_msg = "");
	bool spawn(const FString& spawn_id, bool animation = true, const std::string& spawn_msg = "");

	// static
	static void applyPresetToBanner(const TitleAppearance& title, UGFxData_PlayerTitles_TA* pt = nullptr, bool log = false);
	static void applyPresetToPri(UGFxData_PRI_TA* pri, const TitleAppearance& title);

	static FPlayerTitleData& getTitleFromConfig(int index, UTitleConfig_X* config = nullptr);
	static UTitleConfig_X*   getTitleConfig(bool forceSearch = false);
	static AGFxHUD_TA*       getGFxHUD();
	// static UGFxData_PlayerTitles_TA* getGFxPlayerTitles();	// banner
	static UGFxData_PRI_TA* getUserGFxPRI(); // in-game
	static UGFxData_PRI_TA* getGFxPriFromChatData(APlayerReplicationInfo* priBase, AHUDBase_TA* hudBase);

	static bool decodeAppearance(const std::string& inStr, TitleAppearance& outAppearance);
	static void sendTitleDataChat(const TitleAppearance& appearance, APlayerController* pc = nullptr);

public:
	TitleAppearance* getActivePreset();

	void spawnSelectedPreset(bool log = false);
	void applySelectedAppearanceToUser();
	void applyPresetFromChatData(std::string data, const FChatMessage& msg, AHUDBase_TA* caller);

	// testing
	bool test1();
	bool test2();

public:
	// gui
	void display_titlePresetList();
	void display_titlePresetInfo();
	void display_gameTitlesDropdown();
};

extern class TitlesComponent Titles;