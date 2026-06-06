#ifndef GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H
#define GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H

#include <cstdint>
#include <string>
#include <vector>

enum class ESettingsWarmupCost
{
	TEXT_CONTAINER,
	RENDER_TARGET_RECORD,
	GPU_UPLOAD,
	JOB_RESULT_MERGE,
};

enum class ESettingsWarmupStopReason
{
	NONE,
	TEXT_BUDGET,
	FBO_BUDGET,
	GPU_UPLOAD_BUDGET,
	GPU_READBACK_BUDGET,
	PREVIEW_CACHE_IO_BUDGET,
	MERGE_BUDGET,
	ACTIVE_ITEM,
};

enum class ESettingsWarmupMissReason
{
	NONE,
	PAGE_FBO_UNSUPPORTED,
	PAGE_FBO_NOT_READY,
	SECTION_FBO_NOT_READY,
	DEPENDENCY_NOT_READY,
	RESOURCE_PLAN_PENDING,
	JOB_RESULT_PENDING,
	GPU_UPLOAD_BUDGET,
	SHARED_HEAVY_BUDGET,
	UPLOAD_BYTES_BUDGET,
	OVERSIZED_UPLOAD_DEFERRED,
	TEXT_BUDGET,
	ACTIVE_ITEM,
	INVALID_RUNTIME_KEY,
};

enum class ETClientSettingsPerfStage
{
	TAB_SHELL,
	SECTION_LAYOUT,
	TEXT_CACHE,
	RESOURCE_PRETRIGGER,
	STATIC_LAYER,
	INTERACTIVE_LAYER,
};

enum class ESettingsInvalidationReason
{
	LANGUAGE_CHANGED,
	FONT_CHANGED,
	BACKEND_CHANGED,
	WINDOW_OR_SCALE_CHANGED,
	CONFIG_HASH_CHANGED,
	SECTION_SIZE_CHANGED,
	RESOURCE_DIRECTORY_CHANGED,
};

struct SSettingsWarmupFrameBudget
{
	int m_MaxTextContainers = 8;
	int m_MaxRenderTargetRecords = 1;
	int m_MaxGpuUploads = 14;
	int m_MaxGpuReadbacks = 1;
	int m_MaxPreviewCacheIo = 1;
	int m_MaxJobResultMerges = 1;
	ESettingsWarmupStopReason m_StopReason = ESettingsWarmupStopReason::NONE;
};

struct SSettingsPageRuntimeRegistry
{
	std::vector<int> m_vPages;
};

struct SSettingsSectionEntry
{
	int m_Page = -1;
	int m_Tab = -1;
	std::string m_Id;
	bool m_HasStaticRenderer = false;
	bool m_HasInteractiveRenderer = false;
};

struct SSettingsSectionRegistry
{
	std::vector<SSettingsSectionEntry> m_vSections;
};

struct SSettingsRuntimeCacheKey
{
	uint64_t m_LanguageHash = 0;
	uint64_t m_FontGeneration = 0;
	uint64_t m_BackendGeneration = 0;
	int m_WindowWidth = 0;
	int m_WindowHeight = 0;
	int m_UiScale = 100;
	uint64_t m_ConfigHash = 0;
};

struct SSettingsRuntimeCacheMetadata
{
	int m_LastPage = -1;
	int m_LastTClientTab = 0;
	int m_LastQmTab = 0;
	int m_LastScrollPage = -1;
	float m_LastScrollY = 0.0f;
	SSettingsRuntimeCacheKey m_RuntimeKey;
	bool m_Valid = false;
};

struct SSettingsWarmupPageJob
{
	int m_Page = -1;
	int m_Tab = -1;
	float m_ScrollY = 0.0f;
};

struct SSettingsWarmupStartupPlan
{
	std::vector<SSettingsWarmupPageJob> m_vPageJobs;
};

constexpr int SETTINGS_PAGE_RUNTIME_CACHE_SLOTS = 32;

SSettingsPageRuntimeRegistry BuildSettingsPageRuntimeRegistry();
bool SettingsPageRuntimeRegistryContains(const SSettingsPageRuntimeRegistry &Registry, int Page);
int SettingsCanonicalPage(int Page);
bool SettingsPageVisibleInRightTabBar(int Page);
SSettingsSectionRegistry BuildSettingsSectionRegistry();
bool SettingsSectionRegistryContains(const SSettingsSectionRegistry &Registry, int Page, const char *pSection);
bool SettingsSectionCanRecordStaticFbo(const SSettingsSectionRegistry &Registry, int Page, int Tab, const char *pSection);
int SettingsPageRuntimeCacheSlot(int Page, int Tab);
bool SettingsRuntimeCacheKeyMatches(const SSettingsRuntimeCacheKey &A, const SSettingsRuntimeCacheKey &B);
bool SettingsPageUsesRuntimeScroll(int Page);
SSettingsWarmupStartupPlan BuildSettingsWarmupStartupPlan(const SSettingsRuntimeCacheMetadata &Metadata, const SSettingsPageRuntimeRegistry &Registry);
bool SettingsWarmupPlanContainsPage(const SSettingsWarmupStartupPlan &Plan, int Page);
bool SettingsWarmupConsumeBudget(SSettingsWarmupFrameBudget &Budget, ESettingsWarmupCost Cost);
const char *SettingsWarmupMissReasonName(ESettingsWarmupMissReason Reason);
const char *SettingsTClientPerfStageName(ETClientSettingsPerfStage Stage);
const char *SettingsInvalidationReasonName(ESettingsInvalidationReason Reason);
bool SettingsRuntimeCacheAllowsVisibleCompactText(const char *pRenderName);
void LogSettingsResourcePerf(int Page, const char *pJob, int Count, int Budget, int Remaining, ESettingsWarmupMissReason Reason, double DurationMs);
bool SettingsInvalidationClearsTextPool(ESettingsInvalidationReason Reason);
bool SettingsInvalidationClearsSectionFbo(ESettingsInvalidationReason Reason);
bool SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason Reason);
bool SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason Reason, int Page, int AssetsPage);
bool SettingsInvalidationClearsResourcePlan(ESettingsInvalidationReason Reason);
bool SettingsWarmupEnabled(int PrewarmConfig, int FboConfig);
bool SettingsRuntimeCachingEnabled(int PrewarmConfig, int FboConfig, int NewUiConfig);
std::string SettingsPageCacheKey(int Page, int Tab);
std::string SettingsSectionCacheKey(int Page, int Tab, const char *pSection);
std::string SettingsTextCacheKey(int Page, int Tab, const char *pTextId);
std::string SettingsResourceCacheKey(int Page, const char *pResourceId);

#endif // GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H
