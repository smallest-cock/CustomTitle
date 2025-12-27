#pragma once

namespace Events
{
	constexpr auto GFxData_PlayerTitles_TA__HandleLoadoutSaveLoaded_0x1 =
	    "Function TAGame.GFxData_PlayerTitles_TA.__GFxData_PlayerTitles_TA__HandleLoadoutSaveLoaded_0x1";
	constexpr auto GFxData_PlayerTitles_TA_UpdateSelectedTitle = "Function TAGame.GFxData_PlayerTitles_TA.UpdateSelectedTitle";
	constexpr auto GFxData_PlayerTitles_TA_UpdatePlayerTitles  = "Function TAGame.GFxData_PlayerTitles_TA.UpdatePlayerTitles";
	constexpr auto GFxData_PlayerTitles_TA_GetTitleData        = "Function TAGame.GFxData_PlayerTitles_TA.GetTitleData";
	constexpr auto GFxData_PRI_TA_SetPlayerTitle               = "Function TAGame.GFxData_PRI_TA.SetPlayerTitle";
	constexpr auto GFxHUD_TA_OpenScoreboard                    = "Function TAGame.GFxHUD_TA.OpenScoreboard";
	constexpr auto GFxHUD_TA_Tick                              = "Function TAGame.GFxHUD_TA.Tick";
	constexpr auto GFxHUD_TA_HandleTeamChanged                 = "Function TAGame.GFxHUD_TA.HandleTeamChanged";
	constexpr auto HUDBase_TA_OnChatMessage                    = "Function TAGame.HUDBase_TA.OnChatMessage";
	constexpr auto HUDBase_TA_DrawHUD                          = "Function TAGame.HUDBase_TA.DrawHUD";
	constexpr auto GFxData_StartMenu_TA_ProgressToMainMenu     = "Function TAGame.GFxData_StartMenu_TA.ProgressToMainMenu";
	constexpr auto PlayerController_EnterStartState            = "Function Engine.PlayerController.EnterStartState";
	constexpr auto PlayerTitleConfig_X_GetTitleData            = "Function ProjectX.PlayerTitleConfig_X.GetTitleData";

	constexpr auto EngineShare_X_EventPreLoadMap      = "Function ProjectX.EngineShare_X.EventPreLoadMap";
	constexpr auto LoadingScreen_TA_HandlePostLoadMap = "Function TAGame.LoadingScreen_TA.HandlePostLoadMap";

	constexpr auto GameViewportClient_PostRender = "Function Engine.GameViewportClient.PostRender";

	constexpr auto GameEvent_TA_Countdown_BeginState     = "Function GameEvent_TA.Countdown.BeginState";
	constexpr auto GameEvent_Soccar_TA_Active_StartRound = "Function GameEvent_Soccar_TA.Active.StartRound";
	constexpr auto GFxHUD_TA_HandleReplaceBot            = "Function TAGame.GFxHUD_TA.HandleReplaceBot";
}
