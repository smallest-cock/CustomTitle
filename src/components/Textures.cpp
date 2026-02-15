#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include "Textures.hpp"
#include <ModUtils/util/Utils.hpp>
#include <ModUtils/gui/GuiTools.hpp>
#include "util/Instances.hpp"
#include "util/Macros.hpp"

void IconCustomizationState::constructTextureName() { textureName = texNamePrefix + Format::toCamelCase(iconName); }

void IconCustomizationState::fromJson(const json &data) {
	if (!data.contains("iconName")) {
		LOGERROR("JSON data doesnt contain the key \"iconName\"");
		return;
	}

	iconName = data.at("iconName").get<std::string>();
	if (data.contains("imagePath")) {
		fs::path imgPath{data.at("imagePath").get<std::string>()};
		if (fs::exists(imgPath))
			imagePath = imgPath;
		else {
			LOG("WARNING: \"imagePath\" in JSON is invalid or does not exist: {}", imgPath.string());
			imgPath.clear();
		}
	}
	if (data.contains("enabled"))
		enabled = data.at("enabled").get<bool>();
}

json IconCustomizationState::toJson() const {
	json j = {{"iconName", iconName}, {"enabled", enabled}};

	if (!imagePath.empty()) {
		j["imagePath"] = imagePath.string();
	}

	return j;
}

// ##############################################################################################################
// ################################################    INIT    ##################################################
// ##############################################################################################################

void TexturesComponent::init(const std::shared_ptr<GameWrapper> &gw) {
	gameWrapper = gw;

	setFilePaths();
	findImages();
	initIconCustomizations();
	updateCustomizationsFromJson();
	m_applyIconCustomizations.store(true); // signal to apply all enabled customizations in render thread
}

void TexturesComponent::setFilePaths() {
	fs::path pluginFolder    = gameWrapper->GetDataFolder() / "CustomTitle";
	m_titleIconsFolder       = pluginFolder / "TitleIcons";
	m_iconCustomizationsJson = pluginFolder / "icon_customizations.json";

	if (!fs::exists(m_titleIconsFolder)) {
		LOG("Folder not found: \"{}\"", m_titleIconsFolder.string());
		LOG("Creating it...");
		fs::create_directories(m_titleIconsFolder);
	}

	if (!fs::exists(m_iconCustomizationsJson)) {
		LOG("File not found: \"{}\"", m_iconCustomizationsJson.string());
		LOG("Creating it...");

		fs::create_directories(m_iconCustomizationsJson.parent_path());
		std::ofstream jsonFile(m_iconCustomizationsJson);
		jsonFile << "[]";
		jsonFile.close();
	}
}

void TexturesComponent::findImages(bool notification) {
	m_images.clear();
	Files::FindImages<std::map<std::string, fs::path>>(m_titleIconsFolder, m_images);

	std::string logMsg = std::format("Found {} images", m_images.size());
	LOG(logMsg);

	if (notification)
		GAME_THREAD_EXECUTE({ Instances.spawnNotification("custom title", logMsg, 3); }, logMsg);
}

void TexturesComponent::initIconCustomizations() {
	for (int i = 0; i < m_iconNames.size(); ++i) {
		IconCustomizationState &custer = m_iconCustomizations[i];
		custer.iconName                = m_iconNames[i];
		custer.constructTextureName();
	}
}

void TexturesComponent::applyAllIconCustomizations() {
	for (const auto &state : m_iconCustomizations) {
		if (state.imagePath.empty())
			continue;

		const std::string currentImgFilename = state.imagePath.filename().string();
		auto              it                 = m_images.find(currentImgFilename);
		if (it == m_images.end())
			continue;

		applyCustomTexture(state); // set custom icons.... but we need to do this in the render thread
	}
}

void TexturesComponent::restoreAllIconsToOriginals() {

	for (const auto &state : m_iconCustomizations) {
		if (state.imagePath.empty())
			continue;

		applyOriginalTexture(state); // reset to default icon if it was previously customized
	}
}

// ##############################################################################################################
// ###########################################    FUNCTIONS    ##################################################
// ##############################################################################################################

/*
    Assuming the json is structured like this:

    [
      {
        "iconName": "Bronze",
        "enabled": true,
        "imagePath": "/path/to/bronze_image.png"
      },
      {
        "iconName": "Silver",
        "enabled": false,
      },
      {
        "iconName": "Gold",
        "enabled": true,
        "imagePath": "/path/to/gold_image.png"
      },
      ...
    ]
*/
void TexturesComponent::updateCustomizationsFromJson() {
	json data = Files::get_json(m_iconCustomizationsJson);
	if (data.empty())
		return;

	if (!data.is_array()) {
		LOGERROR("JSON data from file isn't an array");
		return;
	}
	if (data.size() != m_iconCustomizations.size()) {
		LOGERROR("JSON array size isn't {}", m_iconCustomizations.size());
		return;
	}

	// Iterate through the JSON array and populate the customizations
	for (size_t i = 0; i < m_iconCustomizations.size(); ++i) {
		auto &customization = m_iconCustomizations[i];

		const auto &iconData = data[i];
		if (!iconData.is_object()) {
			LOGERROR("Data at index {} of JSON array isn't an object. Skipping this entry...", i);
			continue;
		}

		customization.fromJson(iconData);
	}
}

void TexturesComponent::writeCustomizationsToJson(bool notification) const {
	json data;

	for (const auto &customization : m_iconCustomizations) {
		json customizationJson = customization.toJson();
		data.push_back(customizationJson);
	}

	Files::write_json(m_iconCustomizationsJson, data);

	if (notification) {
		GAME_THREAD_EXECUTE({
			Instances.spawnNotification("custom title", std::format("Updated \"{}\"", m_iconCustomizationsJson.filename().string()), 3);
		});
	}
}

UTexture *TexturesComponent::findIconUTexture(const IconCustomizationState &icon) {
	UTexture *tex = Instances.findObject<UTexture2D>(icon.textureName);
	if (!tex) {
		LOGERROR("Unable to find UTexture for icon using name: \"{}\"", icon.textureName);
		return nullptr;
	}
	if (tex->ObjectFlags & RF_BadObjectFlags) {
		LOGERROR("UTexture is invalid: \"{}\"", icon.textureName);
		return nullptr;
	}

	return tex;
}

void TexturesComponent::applyCustomTexture(const IconCustomizationState &icon) {
	UTexture *tex = findIconUTexture(icon);
	if (!tex)
		return;

	auto it = m_loadedCustomTextures.find(icon.imagePath);
	if (it != m_loadedCustomTextures.end()) {
		LOG("Using cached texture for {} ...", icon.imagePath.filename().string());
		applyCustomImgToTexture(tex, it->second);
	} else
		applyCustomImgToTexture(tex, icon.imagePath);
}

void TexturesComponent::applyOriginalTexture(const IconCustomizationState &icon) {
	UTexture *tex = findIconUTexture(icon);
	if (!tex)
		return;

	auto it = m_originalTextureBackups.find(tex);
	if (it != m_originalTextureBackups.end())
		applyCustomImgToTexture(tex, it->second);
}

void TexturesComponent::applyCustomImgToTexture(UTexture *target, const Microsoft::WRL::ComPtr<ID3D11Texture2D> &dxTex) {
	if (!target || target->ObjectFlags & RF_BadObjectFlags) {
		LOGERROR("UTexture is invalid... unable to change texture");
		return;
	}

	ID3D11Texture2D *targetDxTex = getDxTexture2D(target);
	if (!targetDxTex) {
		LOGERROR("Failed to get ID3D11Texture2D from UTexture");
		return;
	}

	// backup original texture before we overwrite it
	ID3D11Texture2D     *backupTexture = nullptr;
	D3D11_TEXTURE2D_DESC desc;
	targetDxTex->GetDesc(&desc);

	HRESULT hr = Dx11Data::pd3dDevice->CreateTexture2D(&desc, nullptr, &backupTexture);
	if (SUCCEEDED(hr)) {
		Dx11Data::pd3dDeviceContext->CopyResource(backupTexture, targetDxTex);
		if (!m_originalTextureBackups.contains(target)) {
			m_originalTextureBackups[target] = backupTexture; // ComPtr handles ref count
			LOG("Cached original GPU texture for {}", target->GetName());
		}
		backupTexture->Release();
	}

	// now we can change it...
	ID3D11Texture2D *newTex = dxTex.Get();
	Dx11Data::pd3dDeviceContext->CopyResource(targetDxTex, newTex);
}

bool InitializeScratchImageFromImage(const DirectX::Image &srcImage, DirectX::ScratchImage &scratch) {
	HRESULT hr = scratch.Initialize2D(srcImage.format,
	    srcImage.width,
	    srcImage.height,
	    1, // array size
	    1  // mip levels
	);

	if (FAILED(hr)) {
		LOGERROR("Failed to initialize DirectX::ScratchImage using Initialize2D");
		return false;
	}

	// Copy pixel data from source image to scratch image
	const DirectX::Image *destImage = scratch.GetImage(0, 0, 0);
	if (destImage && destImage->pixels && srcImage.pixels) {
		memcpy(destImage->pixels, srcImage.pixels, srcImage.rowPitch * srcImage.height);
		return true;
	}

	return false;
}

void TexturesComponent::applyCustomImgToTexture(UTexture *target, const fs::path &path) {
	if (!target || target->ObjectFlags & RF_BadObjectFlags) {
		LOGERROR("UTexture is invalid... unable to change texture");
		return;
	}
	if (!fs::exists(path)) {
		LOGERROR("Image doesn't exist: \"{}\"", path.string());
		return;
	}

	CustomImageData customImg{};
	if (!createImageDataFromPath(path, customImg))
		return;

	ID3D11Texture2D *targetDxTex = getDxTexture2D(target);
	if (!targetDxTex) {
		LOGERROR("Failed to get ID3D11Texture2D from UTexture");
		return;
	}

	// Get original texture metadata
	D3D11_TEXTURE2D_DESC originalDesc;
	targetDxTex->GetDesc(&originalDesc);
	DXGI_FORMAT originalFormat = originalDesc.Format;
	uint32_t    ogFormatInt    = static_cast<uint32_t>(originalFormat);
	LOG("Original texture format: {} (Compressed: {}, SRGB: {})",
	    ogFormatInt,
	    DirectX::IsCompressed(originalFormat),
	    DirectX::IsSRGB(originalFormat));

	// Prepare new DirectXTex Image
	DirectX::Image customImageSource = {};
	customImageSource.format         = DXGI_FORMAT_R8G8B8A8_UNORM; // default output of stbi_load
	customImageSource.pixels         = customImg.pixelData.get();
	customImageSource.width          = customImg.width;
	customImageSource.height         = customImg.height;
	customImageSource.rowPitch       = customImg.width * customImg.channels;
	customImageSource.slicePitch     = customImageSource.rowPitch * customImg.height;

	DirectX::ScratchImage processedImage; // <-- what will become the image to replace original, using CopyResource
	if (!InitializeScratchImageFromImage(customImageSource, processedImage))
		return;

	HRESULT hr;

	// ################################################################################
	// ########################### STEP 1: RESIZE (IF NEEDED) #########################
	// ################################################################################

	if (customImageSource.width != originalDesc.Width || customImageSource.height != originalDesc.Height) {
		LOG("Custom image dimensions dont match original texture. Resizing {}x{} --> {}x{}",
		    customImageSource.width,
		    customImageSource.height,
		    originalDesc.Width,
		    originalDesc.Height);

		DirectX::ScratchImage resizedImage;
		hr = DirectX::Resize(customImageSource, originalDesc.Width, originalDesc.Height, DirectX::TEX_FILTER_DEFAULT, resizedImage);
		if (FAILED(hr)) {
			LOGERROR("Failed to resize image (HRESULT: {})", Format::ToHexString(hr));
			return;
		}
		processedImage = std::move(resizedImage);

		const DirectX::TexMetadata &processedImgData = processedImage.GetMetadata();
		LOG("New custom image dimensions: {}x{} .... matches original: {}",
		    processedImgData.width,
		    processedImgData.height,
		    processedImgData.width == originalDesc.Width && processedImgData.height == originalDesc.Height);
	}

	// ################################################################################
	// ################## STEP 2: COMPRESS TO MATCH ORIGINAL FORMAT ###################
	// ################################################################################

	LOG("Compressing custom texture to match DGXI format {}...", ogFormatInt);

	const DirectX::TexMetadata &processedImgData = processedImage.GetMetadata();
	if (processedImgData.width % 4 != 0 || processedImgData.height % 4 != 0)
		LOG("WARNING: Custom image dimensions are not multiples of 4. BC compression might be fricked");

	const DirectX::Image *processedImg = processedImage.GetImage(0, 0, 0);
	if (!processedImg) {
		LOGERROR("DirectX::Image* from processedImage.GetImage(0, 0, 0) is null");
		return;
	}

	DirectX::ScratchImage compressedImage;
	hr = DirectX::Compress(*processedImg, originalFormat, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, compressedImage);
	if (FAILED(hr)) {
		LOGERROR("Compression failed (HRESULT: {})", Format::ToHexString(hr));
		return;
	}

	processedImage = std::move(compressedImage);
	LOG("Compressed custom image to DXGI format {} to match original. Matches original format: {}",
	    (uint32_t)processedImage.GetMetadata().format,
	    (uint32_t)processedImage.GetMetadata().format == originalFormat);

	// sanity checks
	auto &finalProcessedImgData = processedImage.GetMetadata();
	if (finalProcessedImgData.width != originalDesc.Width || finalProcessedImgData.height != originalDesc.Height) {
		LOG("WARNING: Final custom image dimensions don't match original texture: {}x{} != {}x{}",
		    finalProcessedImgData.width,
		    finalProcessedImgData.height,
		    originalDesc.Width,
		    originalDesc.Height);
	}
	if (finalProcessedImgData.format != originalFormat) {
		LOG("WARNING: Final custom image format doesn't match original texture: {} != {}",
		    (uint32_t)finalProcessedImgData.format,
		    ogFormatInt);
	}

	// ################################################################################
	// ######### STEP 3: CREATE NEW GPU TEXTURE AND COPY RESOURCE TO ORIGINAL #########
	// ################################################################################

	ID3D11Texture2D     *newTexture = nullptr;
	D3D11_TEXTURE2D_DESC newDesc    = originalDesc;

	newDesc.Format         = finalProcessedImgData.format;
	newDesc.Usage          = D3D11_USAGE_DEFAULT;
	newDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
	newDesc.MipLevels      = 1; // just to be explicit
	newDesc.ArraySize      = 1;
	newDesc.CPUAccessFlags = 0;
	newDesc.MiscFlags      = 0;

	const DirectX::Image *finalImage = processedImage.GetImage(0, 0, 0);
	LOG("Final processed DXGI format: {}", (uint32_t)finalImage->format);

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem          = finalImage->pixels;
	initData.SysMemPitch      = finalImage->rowPitch;
	initData.SysMemSlicePitch = finalImage->slicePitch;

	hr = Dx11Data::pd3dDevice->CreateTexture2D(&newDesc, &initData, &newTexture);
	if (FAILED(hr)) {
		LOGERROR("Failed to create temp texture (HRESULT: {}).", Format::ToHexString(hr));
		return;
	}

	// cache newly created custom texture
	m_loadedCustomTextures[path] = newTexture; // ComPtr handles ref count
	LOG("Cached custom GPU texture for \"{}\"", path.filename().string());

	// backup original texture before we overwrite it
	ID3D11Texture2D     *backupTexture = nullptr;
	D3D11_TEXTURE2D_DESC desc;
	targetDxTex->GetDesc(&desc);

	hr = Dx11Data::pd3dDevice->CreateTexture2D(&desc, nullptr, &backupTexture);
	if (SUCCEEDED(hr)) {
		Dx11Data::pd3dDeviceContext->CopyResource(backupTexture, targetDxTex);
		if (!m_originalTextureBackups.contains(target)) {
			m_originalTextureBackups[target] = backupTexture; // ComPtr handles ref count
			LOG("Saved original GPU texture for {}", target->GetName());
		}
		backupTexture->Release();
	}

	// now we can change the texture...
	Dx11Data::pd3dDeviceContext->CopyResource(targetDxTex, newTexture);
	newTexture->Release();
	LOG("Texture updated via CopyResource");
}

bool TexturesComponent::createImageDataFromPath(const fs::path &imgPath, CustomImageData &outData) {
	outData = {};

	if (!fs::exists(imgPath)) {
		LOGERROR("Filepath doesn't exist: \"{}\"", imgPath.string());
		return false;
	}

	int      width, height, channels;
	stbi_uc *imagePixelData = stbi_load(imgPath.string().c_str(), &width, &height, &channels, 4); // Force 4 channels
	if (!imagePixelData) {
		LOG("Failed to load image '{}': {}", imgPath.string(), stbi_failure_reason());
		return false;
	}
	const size_t numPixels = width * height;

	// set data
	outData.pixelData.reset(imagePixelData);
	outData.width    = width;
	outData.height   = height;
	outData.channels = channels;

	LOG("Loaded {}x{} image ({} bytes) from \"{}\"", width, height, (numPixels * channels), imgPath.filename().string());

	return true;
}

// ##############################################################################################################
// ########################################    STATIC FUNCTIONS    ##############################################
// ##############################################################################################################

FD3D11Texture2D *TexturesComponent::getDxTextureData(UTexture *tex) {
	if (!tex) {
		LOGERROR("UTexture* is null");
		return nullptr;
	} else if (tex->ObjectFlags & RF_BadObjectFlags) {
		LOGERROR("UTexture has bad object flags");
		return nullptr;
	}

	FTextureResource *texResource = reinterpret_cast<FTextureResource *>(tex->Resource.Dummy);
	if (!texResource) {
		LOGERROR("FTextureResource* from UTexture is null");
		return nullptr;
	}

	return texResource->TextureRHI;
}

ID3D11Texture2D *TexturesComponent::getDxTexture2D(UTexture *tex) {
	FD3D11Texture2D *dxTexData = getDxTextureData(tex);
	if (!dxTexData) {
		LOGERROR("FD3D11Texture2D* from UTexture is null");
		return nullptr;
	}

	return dxTexData->Resource;
}

ID3D11ShaderResourceView *TexturesComponent::getDxSRV(UTexture *tex) {
	FD3D11Texture2D *dxTexData = getDxTextureData(tex);
	if (!dxTexData) {
		LOGERROR("FD3D11Texture2D* from UTexture is null");
		return nullptr;
	}

	return dxTexData->View;
}

// ##############################################################################################################
// #########################################    DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void TexturesComponent::display_iconCustomizations() {
	if (ImGui::Button("Open TitleIcons folder"))
		Files::OpenFolder(m_titleIconsFolder);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Default icon size is 72x72 pixels");

	GUI::Spacing(2);

	ImGui::Text("%zu valid images found", m_images.size());

	GUI::SameLineSpacing_relative(20);

	if (ImGui::Button("Refresh"))
		findImages(true);

	GUI::Spacing(4);

	static std::vector<std::string> imgOptions;
	imgOptions.clear();
	imgOptions.emplace_back("None");
	for (const auto &[label, path] : m_images)
		imgOptions.push_back(label.c_str());

	const float start = ImGui::GetCursorPosX();

	for (auto &iconCustomization : m_iconCustomizations) {
		GUI::ScopedID id{&iconCustomization};

		ImGui::TextUnformatted(iconCustomization.iconName.c_str());
		GUI::SameLineSpacing_absolute(start + SAMELINE_DROPDOWN_SPACING);
		display_iconCustomizationDropdown(iconCustomization, imgOptions);
	}

	/*
	    // debug
	    GUI::Spacing(4);

	    if (ImGui::CollapsingHeader("Debug"))
	    {
	        ImGui::Text("m_loadedCustomTextures size: %i", m_loadedCustomTextures.size());
	        for (const auto& [path, comptr] : m_loadedCustomTextures)
	        {
	            ImGui::BulletText("%s\t-->\t%s", path.filename().string().c_str(),
	                              Format::ToHexString(comptr.Get()).c_str());
	        }

	        ImGui::Text("m_originalTextureBackups size: %i", m_originalTextureBackups.size());
	        for (const auto& [tex, comptr] : m_originalTextureBackups)
	        {
	            if (!tex || tex->ObjectFlags & RF_BadObjectFlags)
	            {
	                ImGui::BulletText("INVALID OBJECT");
	                continue;
	            }

	            ImGui::BulletText("%s\t-->\t%s", tex->GetName().c_str(),
	                              Format::ToHexString(comptr.Get()).c_str());
	        }
	    }
	*/
}

void TexturesComponent::display_iconCustomizationDropdown(IconCustomizationState &state, const std::vector<std::string> &options) {
	const std::string currentImgFilename = state.imagePath.empty() ? "" : state.imagePath.filename().string();
	if (ImGui::BeginCombo("##icon_dropdown", currentImgFilename.c_str())) {
		for (const auto &imgOption : options) {
			GUI::ScopedID id{&imgOption};

			if (ImGui::Selectable(imgOption.c_str(), currentImgFilename == (imgOption == "None" ? "" : imgOption))) {
				if (imgOption == "None") {
					state.enabled = false;
					state.imagePath.clear();

					applyOriginalTexture(state); // reset to default icon if it was previously customized
				} else {
					auto it = m_images.find(imgOption);
					if (it == m_images.end())
						continue;
					state.imagePath = it->second;
					state.enabled   = true;

					applyCustomTexture(state); // set custom icon
				}

				writeCustomizationsToJson();
			}
		}

		ImGui::EndCombo();
	}
}

class TexturesComponent Textures{};

namespace Dx11Data {
	ID3D11Device        *pd3dDevice        = nullptr;
	ID3D11DeviceContext *pd3dDeviceContext = nullptr;

	using PresentFn    = HRESULT(__stdcall *)(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags);
	PresentFn oPresent = nullptr;

	HRESULT __stdcall hkPresent(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags) {
		if (!pd3dDevice) {
			// Retrieve the device and context once when Present is first called
			pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pd3dDevice);
			pd3dDevice->GetImmediateContext(&pd3dDeviceContext);
			LOG("Hooked IDXGISwapChain::Present! Obtained ID3D11Device and ID3D11DeviceContext.");
		}

		// shitty lil system of running stuff in the render thread
		if (Textures.shouldApplyIconCustomizations()) {
			Textures.applyAllIconCustomizations();
			Textures.setApplyIconCustomizations(false);
		} else if (Textures.shouldRestoreOriginalIcons()) {
			Textures.restoreAllIconsToOriginals();
			Textures.setRestoreOriginalIcons(false);
		}

		return oPresent(pSwapChain, SyncInterval, Flags);
	}

	void InitializeKiero() {
		kiero::init(kiero::RenderType::D3D11); // Initialize Kiero with D3D11
		LOG("Initialized kiero");
	}

	void HookPresent() {
		// IDXGISwapChain::Present is usually at index 8 in the vtable
		if (kiero::bind(8, (void **)&oPresent, (void *)hkPresent) == kiero::Status::Success)
			LOG("Successfully hooked IDXGISwapChain::Present function");
		else
			LOG("Failed to hook IDXGISwapChain::Present function");
	}

	void UnhookPresent() {
		kiero::unbind(8);
		LOG("Unooked IDXGISwapChain::Present function");
	}
} // namespace Dx11Data
