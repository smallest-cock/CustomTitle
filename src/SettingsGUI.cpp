#include "pch.h"
#include "Cvars.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "CustomTitle.hpp"
#include "Macros.hpp"
#include "components/Titles.hpp"
#include "components/Textures.hpp"

void CustomTitle::RenderSettings()
{
	const float content_height = ImGui::GetContentRegionAvail().y - footer_height; // available height after accounting for footer

	if (ImGui::BeginChild("MainSettingsSection", ImVec2(0, content_height)))
	{
		GUI::alt_settings_header(h_label.c_str(), pretty_plugin_version, gameWrapper);

		GUI::Spacing(4);

		// open bindings window button
		std::string openMenuCommand = "togglemenu " + GetMenuName();
		if (ImGui::Button("Open Menu"))
			GAME_THREAD_EXECUTE({ cvarManager->executeCommand(openMenuCommand); }, openMenuCommand);

		GUI::Spacing(4);

		ImGui::Text("or bind this command:  ");
		ImGui::SameLine();
		ImGui::PushItemWidth(150);
		ImGui::InputText("##openMenuCommand", &openMenuCommand, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();

		GUI::Spacing(8);

		if (ImGui::Button(std::format("Open {} folder", stringify_(CustomTitle)).c_str()))
			Files::OpenFolder(m_pluginFolder);

		GUI::Spacing(8);

		std::string spawnTitleCommand = Commands::spawnCustomTitle.name;
		ImGui::Text("Bind this command to spawn your custom title with a keypress:  ");
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		ImGui::InputText("##spawnCommand", &spawnTitleCommand, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();
	}
	ImGui::EndChild();

	GUI::alt_settings_footer("Need help? Join the Discord", "https://discord.gg/d5ahhQmJbJ");
}

void CustomTitle::RenderWindow()
{
	if (ImGui::BeginTabBar("##Tabs"))
	{
		if (ImGui::BeginTabItem("Presets"))
		{
			TitlePresets_Tab();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Icons"))
		{
			TourneyIcons_Tab();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Settings"))
		{
			Settings_Tab();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Info"))
		{
			Info_Tab();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void CustomTitle::Settings_Tab()
{
	auto showTitleToOthers_cvar        = getCvar(Cvars::showTitleToOthers);
	auto showOtherPlayerTitles_cvar    = getCvar(Cvars::showOtherPlayerTitles);
	auto filterOtherPlayerTitles_cvar  = getCvar(Cvars::filterOtherPlayerTitles);
	auto applyOthersTitleNotif_cvar    = getCvar(Cvars::applyOthersTitleNotif);
	auto useHueColorPicker_cvar        = getCvar(Cvars::useHueColorPicker);
	auto showEquippedTitleDetails_cvar = getCvar(Cvars::showEquippedTitleDetails);
	auto rgbSpeed_cvar                 = getCvar(Cvars::rgbSpeed);
	if (!showTitleToOthers_cvar)
		return;

	bool showTitleToOthers = showTitleToOthers_cvar.getBoolValue();
	if (ImGui::Checkbox("Make my title visible to other players with the mod", &showTitleToOthers))
		showTitleToOthers_cvar.setValue(showTitleToOthers);

	bool showOtherPlayerTitles = showOtherPlayerTitles_cvar.getBoolValue();
	if (ImGui::Checkbox("Show the titles of other players with the mod", &showOtherPlayerTitles))
		showOtherPlayerTitles_cvar.setValue(showOtherPlayerTitles);

	bool filterOtherPlayerTitles = filterOtherPlayerTitles_cvar.getBoolValue();
	if (ImGui::Checkbox("Censor other players' custom titles", &filterOtherPlayerTitles))
		filterOtherPlayerTitles_cvar.setValue(filterOtherPlayerTitles);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Word filtering isn't foolproof");

	bool applyOthersTitleNotif = applyOthersTitleNotif_cvar.getBoolValue();
	if (ImGui::Checkbox("Show notification when applying other players' titles", &applyOthersTitleNotif))
		applyOthersTitleNotif_cvar.setValue(applyOthersTitleNotif);

	bool useHueColorPicker = useHueColorPicker_cvar.getBoolValue();
	if (ImGui::Checkbox("Use hue wheel for color picker", &useHueColorPicker))
		useHueColorPicker_cvar.setValue(useHueColorPicker);

	bool showEquippedTitleDetails = showEquippedTitleDetails_cvar.getBoolValue();
	if (ImGui::Checkbox("Show extra info about equipped title in Presets tab", &showEquippedTitleDetails))
		showEquippedTitleDetails_cvar.setValue(showEquippedTitleDetails);

	GUI::Spacing(4);

	// determine slider text
	int         rgbSpeed = rgbSpeed_cvar.getIntValue();
	std::string formatStr;

	if (rgbSpeed == TitlesComponent::DEFAULT_RGB_SPEED)
		formatStr = "default";
	else if (rgbSpeed < TitlesComponent::DEFAULT_RGB_SPEED)
		formatStr = std::format("{}x slower", std::abs(rgbSpeed) + 1);
	else
		formatStr = std::format("{}x faster", std::abs(rgbSpeed) + 1);

	if (ImGui::SliderInt("RGB speed", &rgbSpeed, -14, 9, formatStr.c_str()))
		rgbSpeed_cvar.setValue(rgbSpeed);

	GUI::SameLineSpacing_relative(25);

	if (ImGui::Button("Reset##rgbSpeed"))
		rgbSpeed_cvar.setValue(TitlesComponent::DEFAULT_RGB_SPEED);
}

void CustomTitle::TitlePresets_Tab()
{
	ImVec2 total_size = ImGui::GetContentRegionAvail();

	{
		GUI::ScopedChild c{"PresetList", ImVec2(total_size.x * 0.3f, 0), true};

		Titles.display_titlePresetList();
	}

	ImGui::SameLine();

	{
		GUI::ScopedChild c{"PresetInfo", ImGui::GetContentRegionAvail(), true};

		Titles.display_titlePresetInfo();
	}
}

void CustomTitle::TourneyIcons_Tab()
{
	GUI::Spacing(2);

	Textures.display_iconCustomizations();
}

void CustomTitle::Info_Tab()
{
	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::Yellow, "Important plugin info:");

	GUI::Spacing(4);

	ImGui::TextColored(GUI::Colors::LightGreen, "This mod simply alters the appearance of your equipped title (client-side)");
	ImGui::BulletText("It doesn't create new titles");
	ImGui::BulletText("It doesn't give you any more titles than you already own");

	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::LightGreen, "The presets exist as a way for you to save your designs and easily switch between them");
	ImGui::BulletText("You can think of it like applying different decals (presets) to a single car (your equipped title)");

	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::LightGreen, "The \"Spawn\" button is just a fun visual effect. It doesn't actually give you a title");
	ImGui::BulletText("Always press OK after spawning a title (not EQUIP NOW) to avoid buggy behavior");
	ImGui::BulletText("You may notice 2 \"custom\" titles in your inventory after pressing the spawn button. Don't try to equip them.\n"
	                  "One is your currently equipped title (with the custom appearance applied), the other is a fake dummy title\n"
	                  "that was used to create the spawn effect. Equipping the dummy title can cause buggy behavior.");

	GUI::Spacing(8);

	GUI::centerTextColoredX(GUI::Colors::Orange, "This plugin is incompatible with AlphaConsole's custom title feature.");
	GUI::centerTextColoredX(GUI::Colors::Orange, "Make sure to disable it (dont have text in it) if you're using AlphaConsole.");
}