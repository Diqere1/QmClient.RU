#ifndef GAME_CLIENT_COMPONENTS_SECTION_LOADER_H
#define GAME_CLIENT_COMPONENTS_SECTION_LOADER_H

#include <cstdint>
#include <functional>
#include <vector>

#include <game/client/ui_rect.h>

enum class ESettingsSectionState : uint8_t
{
	UNINITIALIZED,
	MEASURING,
	COMPACT,
	FULL,
};

/**
 * A single section of a settings page.
 *
 * Each section provides three render callbacks:
 *   - MeasureFn:  calculate height without any rendering
 *   - RenderCompactFn: render a compact placeholder (title only)
 *   - RenderFullFn:    render the full interactive section
 *
 * All callbacks receive a CUIRect for the available column space and return
 * the consumed height. The caller is responsible for advancing the column rect.
 *
 * Dependencies track which g_Config values affect this section's output.
 * When the config hash matches the last rendered version, the section is
 * considered clean and FULL rendering is skipped (dirty-flag optimization).
 */
struct SSettingsSection
{
	const char *m_pName;
	ESettingsSectionState m_State = ESettingsSectionState::UNINITIALIZED;
	float m_CachedHeight = 0.0f;

	std::function<float(CUIRect &)> m_MeasureFn;
	std::function<float(CUIRect &)> m_RenderCompactFn;
	std::function<float(CUIRect &)> m_RenderFullFn;

	std::vector<const int *> m_DependencyConfigInts;
	std::vector<const unsigned *> m_DependencyConfigCols;
	uint64_t m_LastConfigHash = 0;
	bool m_bDirty = true; // force render on first frame
};

/**
 * Session UI cache saved to disk across sessions.
 *
 * Stores the last active tab and scroll position so the next launch can
 * pre-warm the relevant sections during the loading screen.
 */
struct SSessionUiCache
{
	int m_LastTClientTab = -1;
	int m_LastQmTab = -1;
	float m_LastScrollY = 0.0f;
	bool m_bValid = false;
};

/**
 * Drives progressive rendering of settings-page sections.
 *
 * Usage:
 *   1. Register() all sections once.
 *   2. Call Begin() when the settings page opens.
 *   3. Call Process() every frame; returns true while there is still work.
 *   4. Call Reset() on tab switch.
 *   5. Optionally call Warmup() during the loading screen.
 *
 * The loader advances sections through a four-state machine with a
 * per-frame time budget:
 *
 *   UNINITIALIZED  →  measure height (negligible cost)
 *   MEASURING      →  render compact placeholder (if in/near viewport)
 *   COMPACT        →  render full interactive section (1–2 per frame max)
 *   FULL           →  re-render only when dirty (config changed)
 *
 * Viewport priority ensures that sections near the current scroll position
 * are promoted before off-screen sections.
 */
class CSectionLoader
{
public:
	CSectionLoader();

	/** Register the full set of sections. Call once per settings-page instance. */
	void Register(std::vector<SSettingsSection> vSections);

	/**
	 * Start progressive rendering for the given viewport rect.
	 * @param MainView     Full available area for the sections.
	 * @param TimeBudgetMs Per-frame CPU budget in milliseconds (default 5.0).
	 */
	void Begin(CUIRect MainView, float TimeBudgetMs = 5.0f);

	/** Advance one frame. Returns true when there is still work left. */
	bool Process();

	/** All visible sections have reached FULL state. */
	bool IsComplete() const;

	/** Reset the state machine (e.g. when switching tabs). */
	void Reset();

	/** Lightweight mode: simple frame counter without section registration.
	    Call BeginLightweight(InitialFrames, budget) then Process() each frame.
	    GetFramesRemaining() returns the diminishing counter. */
	void BeginLightweight(int InitialFrames, float TimeBudgetMs = 5.0f);
	int GetFramesRemaining() const;

	// -- Pre-warming (loading screen) --

	/**
	 * Pre-render the compact pass for sections that were visible in the last
	 * session, so glyph atlases are populated before the user opens settings.
	 * Call once per frame during the loading screen.
	 * @param pCache       Session cache from last run (null = skip).
	 * @param TimeBudgetMs Per-frame CPU budget in milliseconds (default 3.0).
	 * @returns true when warmup is finished.
	 */
	bool Warmup(const SSessionUiCache *pCache, float TimeBudgetMs = 3.0f);
	bool IsWarmupComplete() const;

	// -- Cache invalidation --

	/** Invalidate all section caches (e.g. after language change or resize). */
	void InvalidateCache();

	/** Mark sections dirty that depend on the given config pointer. */
	void SetDirtyByConfig(const void *pConfigVar);

	// -- Session cache I/O --

	static bool LoadSessionCache(SSessionUiCache &Cache, const char *pFilename, class IStorage *pStorage);
	static void SaveSessionCache(const SSessionUiCache &Cache, const char *pFilename, class IStorage *pStorage);

	// -- State exposed for the rendering loop (updated externally) --

	int m_ActiveTab = -1;
	float m_ScrollY = 0.0f;

	// -- Profiling --

	const char *GetPerfReport() const;

private:
	std::vector<SSettingsSection> m_vSections;
	CUIRect m_MainView;
	double m_BudgetPerFrameMs = 5.0;

	int m_CurrentIndex = 0;
	bool m_bInitialized = false;
	bool m_bComplete = false;
	bool m_bLightweight = false;
	int m_LightweightFramesInitial = 0;
	int m_LightweightFramesRemaining = 0;

	// Warmup state
	bool m_bWarmupActive = false;
	int m_WarmupIndex = 0;
	float m_WarmupBudgetMs = 0.0f;

	// Profiling
	double m_TotalFrameTimeMs = 0.0;

	/** 0 = in viewport, 1 = near, 2 = far. */
	int ComputeViewportPriority(const CUIRect &SectionRect) const;

	static uint64_t ComputeConfigHash(const SSettingsSection &Section);
};

#endif // GAME_CLIENT_COMPONENTS_SECTION_LOADER_H
