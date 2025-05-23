#include "pch.h"
#include "CustomTitle.h"


void CustomTitle::RenderSettings()
{
	const float content_height = ImGui::GetContentRegionAvail().y - footer_height;  // available height after accounting for footer

	if (ImGui::BeginChild("MainSettingsSection", ImVec2(0, content_height)))
	{
		GUI::alt_settings_header(h_label.c_str(), pretty_plugin_version);

		GUI::Spacing(4);

		// open bindings window button
		std::string openMenuCommand = "togglemenu " + GetMenuName();
		if (ImGui::Button("Open Menu"))
		{
			GAME_THREAD_EXECUTE_CAPTURE(
				cvarManager->executeCommand(openMenuCommand);
			, openMenuCommand);
		}

		GUI::Spacing(8);

		ImGui::Text("or bind this command:  ");
		ImGui::SameLine();
		ImGui::PushItemWidth(150);
		ImGui::InputText("", &openMenuCommand, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();
		
		GUI::Spacing(8);

		if (ImGui::Button(std::format("Open {} folder", stringify_(CustomTitle)).c_str()))
			Files::OpenFolder(m_pluginFolder);
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

		ImGui::EndTabBar();
	}
}

void CustomTitle::Settings_Tab()
{
	auto showTitleToOthers_cvar =			GetCvar(Cvars::showTitleToOthers);
	auto showOtherPlayerTitles_cvar =		GetCvar(Cvars::showOtherPlayerTitles);
	auto filterOtherPlayerTitles_cvar =		GetCvar(Cvars::filterOtherPlayerTitles);
	auto applyOthersTitleNotif_cvar =		GetCvar(Cvars::applyOthersTitleNotif);
	if (!showTitleToOthers_cvar)
		return;

	bool showTitleToOthers = showTitleToOthers_cvar.getBoolValue();
	if (ImGui::Checkbox("Make my title visible to other players with the mod", &showTitleToOthers))
		showTitleToOthers_cvar.setValue(showTitleToOthers);

	bool showOtherPlayerTitles = showOtherPlayerTitles_cvar.getBoolValue();
	if (ImGui::Checkbox("Show titles of other players with the mod", &showOtherPlayerTitles))
		showOtherPlayerTitles_cvar.setValue(showOtherPlayerTitles);

	bool filterOtherPlayerTitles = filterOtherPlayerTitles_cvar.getBoolValue();
	if (ImGui::Checkbox("Censor other players' custom titles", &filterOtherPlayerTitles))
		filterOtherPlayerTitles_cvar.setValue(filterOtherPlayerTitles);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Word filtering isn't foolproof. Some stuff might slip through...");

	bool applyOthersTitleNotif = applyOthersTitleNotif_cvar.getBoolValue();
	if (ImGui::Checkbox("Show notification when applying other players' titles", &applyOthersTitleNotif))
		applyOthersTitleNotif_cvar.setValue(applyOthersTitleNotif);
}

void CustomTitle::TitlePresets_Tab()
{
	ImVec2 total_size = ImGui::GetContentRegionAvail();

	{
		GUI::ScopedChild c{ "PresetList", ImVec2(total_size.x * 0.3f, 0), true };

		Titles.display_titlePresetList();
	}

	ImGui::SameLine();

	{
		GUI::ScopedChild c{ "PresetInfo", ImGui::GetContentRegionAvail(), true };
	
        Titles.display_titlePresetInfo();
	}
}

void CustomTitle::TourneyIcons_Tab()
{
	GUI::Spacing(2);

	Textures.display_iconCustomizations();
}