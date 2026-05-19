#ifndef GAME_CLIENT_COMPONENTS_QMCLIENT_MODES_H
#define GAME_CLIENT_COMPONENTS_QMCLIENT_MODES_H

struct SQmAirJumpEffectDecision
{
	bool m_SpawnParticles = false;
	bool m_PlaySound = false;
};

struct SQmFocusModeConfig
{
	bool m_FocusActive = false;
	bool m_LegacyHideEffects = false;
	bool m_HideJumpEffects = false;
	bool m_HideKillEffects = false;
	bool m_HideExplosionEffects = false;
	bool m_HideFreezeEffects = false;
	bool m_HideHammerEffects = false;
	bool m_MuteJumpSounds = false;
	bool m_MuteHammerSounds = false;
	bool m_SoundEnabled = true;
	bool m_HideMapProgress = false;
	bool m_MapProgressEnabled = false;
	int m_MapProgressStyle = 0;
	bool m_PlayerStatsHudEnabled = false;
	bool m_GoresMapProgressEnabled = false;
	bool m_HideHud = false;
	bool m_HideScoreboard = false;
	bool m_HideNames = false;
	bool m_HideInfoMessages = false;
	bool m_HideDirectionIndicators = false;
	bool m_HideGuideLines = false;
	bool m_HidePlayerMessages = false;
	bool m_HideSystemMessages = false;
	bool m_HideEchoMessages = false;
};

struct SQmFocusModeDecisions
{
	SQmAirJumpEffectDecision m_AirJump;
	bool m_HideKillEffects = false;
	bool m_HideExplosionEffects = false;
	bool m_HideFreezeEffects = false;
	bool m_HideHammerEffects = false;
	bool m_MuteHammerSounds = false;
	bool m_RenderMapProgressBar = false;
	bool m_HideHud = false;
	bool m_HideScoreboard = false;
	bool m_HideNames = false;
	bool m_HideInfoMessages = false;
	bool m_HideDirectionIndicators = false;
	bool m_HideGuideLines = false;
	bool m_HidePlayerMessages = false;
	bool m_HideSystemMessages = false;
	bool m_HideEchoMessages = false;
};

struct SQmFocusConfigOverrideState
{
	bool m_WasActive = false;
	int m_SavedValue = 0;
	bool m_AutoChangedValue = false;
};

int ApplyQmFocusConfigOverride(SQmFocusConfigOverrideState &State, bool HideActive, int CurrentValue, int HiddenValue, bool &Changed);
bool ApplyQmGoresLinkedConfig(bool GoresActive, bool AutoToggle, bool CurrentValue, bool &Changed);

bool ShouldHideGoresGuide(bool GoresEnabled, bool HideGuidesEnabled, bool ManualGuideVisible);
bool ShouldRenderGoresDebugRoute(bool Online, bool DebugRouteEnabled, bool GoresMapProgressEnabled);

bool ShouldHideFocusUiOverlays(bool FocusActive, bool HideUi);
bool ShouldHideFocusHud(bool FocusActive, bool HideHud);
bool ShouldHideFocusScoreboard(bool FocusActive, bool HideScoreboard);
bool ShouldHideFocusNames(bool FocusActive, bool HideNames);
bool ShouldHideFocusJumpEffects(bool FocusActive, bool HideJumpEffects);
bool ShouldHideFocusKillEffects(bool FocusActive, bool HideKillEffects);
bool ShouldHideFocusExplosionEffects(bool FocusActive, bool HideExplosionEffects);
bool ShouldHideFocusFreezeEffects(bool FocusActive, bool HideFreezeEffects);
bool ShouldHideFocusHammerEffects(bool FocusActive, bool HideHammerEffects);
bool ShouldMuteFocusJumpSounds(bool FocusActive, bool MuteJumpSounds);
bool ShouldMuteFocusHammerSounds(bool FocusActive, bool MuteHammerSounds);
bool ShouldPlayFocusJumpSound(bool FocusActive, bool MuteJumpSounds, bool SoundEnabled);
SQmAirJumpEffectDecision GetQmAirJumpEffectDecision(bool FocusActive, bool HideJumpEffects, bool MuteJumpSounds, bool SoundEnabled);
bool ShouldHideFocusMapProgress(bool FocusActive, bool HideMapProgress);
bool ShouldRenderMapProgressBar(bool MapProgressEnabled, int MapProgressStyle, bool PlayerStatsHudEnabled, bool GoresMapProgressEnabled);
bool ShouldHideFocusInfoMessages(bool FocusActive, bool HideInfoMessages);
bool ShouldHideFocusDirectionIndicators(bool FocusActive, bool HideDirectionIndicators);
bool ShouldHideFocusGuideLines(bool FocusActive, bool HideGuideLines);
bool ShouldRenderFocusFilteredChatLine(bool FocusHidePlayerMessages, bool FocusHideSystemMessages, bool FocusHideEcho, int ClientId, bool ForceVisible);
bool ShouldRenderAnyFocusFilteredChat(bool FocusHidePlayerMessages, bool FocusHideSystemMessages, bool FocusHideEcho, bool HasForceVisibleLine);
SQmFocusModeDecisions GetQmFocusModeDecisions(const SQmFocusModeConfig &Config);

#endif
