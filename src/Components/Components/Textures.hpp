#pragma once
#include "pch.h"
#include "Component.hpp"
#include <stb_image.h>


namespace stb
{
inline void StbiFree(void* ptr)
{
	free(ptr);
}
}

struct CustomImageData
{
	std::unique_ptr<stbi_uc, decltype(&stb::StbiFree)> pixelData = {nullptr, stb::StbiFree};
	uint32_t width        = 0;
	uint32_t height       = 0;
	uint8_t channels      = 4;	// RGBA
};


struct IconCustomizationState
{
	std::string		iconName;
	std::string		textureName;
	fs::path		imagePath;
	bool			enabled = false;

	static constexpr auto texNamePrefix = "tournament_title_icon_";

	void constructTextureName();
	void fromJson(const json& data);
	json toJson() const;
};

class TexturesComponent : Component<TexturesComponent>
{
public:
	TexturesComponent();
	~TexturesComponent();

	static constexpr const char* component_name = "Textures";
	void Initialize(std::shared_ptr<GameWrapper> gw);

private:
	void setFilePaths();

private:
	fs::path m_titleIconsFolder;
	fs::path m_iconCustomizationsJson;

	static constexpr std::array<std::string_view, 8> m_iconNames =
	{
		"Bronze",
		"Silver",
		"Gold",
		"Platinum",
		"Diamond",
		"Champion",
		"Grand Champion",
		"Supersonic Legend"
	};
	std::array<IconCustomizationState, 8> m_iconCustomizations;
	std::map<std::string, fs::path> m_images;

	std::unordered_map<fs::path, Microsoft::WRL::ComPtr<ID3D11Texture2D>> m_loadedCustomTextures;
	std::unordered_map<UTexture*, Microsoft::WRL::ComPtr<ID3D11Texture2D>> m_originalTextureBackups;

	std::atomic<bool> m_applyIconCustomizations = false;
	std::atomic<bool> m_restoreOriginalIcons = false;

private:
	// bookkeeping
	void findImages(bool notification = false);
	void initIconCustomizations();
	void updateCustomizationsFromJson();
	void writeCustomizationsToJson(bool notification = false) const;

	// texture modding
	void applyCustomTexture(const IconCustomizationState& icon);
	void applyOriginalTexture(const IconCustomizationState& icon);

	UTexture* findIconUTexture(const IconCustomizationState& icon);
	Microsoft::WRL::ComPtr<ID3D11Texture2D> getTextureFromPath(const std::string& imgPath);
	void applyCustomImgToTexture(UTexture* target, const fs::path& path);
	void applyCustomImgToTexture(UTexture* target, const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dxTex);
	bool createImageDataFromPath(const fs::path& path, CustomImageData& outData);
	//void backupOriginalTexture(UTexture* tex);
	//void revertToOriginalTexture(UTexture* tex);

	// static
	static FD3D11Texture2D* getDxTextureData(UTexture* tex);
	static ID3D11Texture2D* getDxTexture2D(UTexture* tex);
	static ID3D11ShaderResourceView* getDxSRV(UTexture* tex);

public:
	// lil api
	inline bool shouldApplyIconCustomizations() const { return m_applyIconCustomizations.load(); }
	inline bool shouldRestoreOriginalIcons() const { return m_restoreOriginalIcons.load(); }
	inline void setApplyIconCustomizations(bool val) { m_applyIconCustomizations.store(val); }
	inline void setRestoreOriginalIcons(bool val) { m_restoreOriginalIcons.store(val); }
	void applyAllIconCustomizations();
	void restoreAllIconsToOriginals();

	// gui
	static constexpr float SAMELINE_DROPDOWN_SPACING = 150.0f;

	void display_iconCustomizations();
	void display_iconCustomizationDropdown(IconCustomizationState& state, const std::vector<std::string>& options);
};

extern class TexturesComponent Textures;


namespace Dx11Data
{
	extern ID3D11Device* pd3dDevice;
	extern ID3D11DeviceContext* pd3dDeviceContext;
	
	void InitializeKiero();
	void HookPresent();
	void UnhookPresent();
}