#ifndef GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H
#define GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H

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
	MERGE_BUDGET,
	ACTIVE_ITEM,
};

enum class ESettingsWarmupMissReason
{
	NONE,
	PAGE_FBO_UNSUPPORTED,
	PAGE_FBO_NOT_READY,
	SECTION_FBO_NOT_READY,
	RESOURCE_PLAN_PENDING,
	JOB_RESULT_PENDING,
	GPU_UPLOAD_BUDGET,
	TEXT_BUDGET,
	ACTIVE_ITEM,
	INVALID_RUNTIME_KEY,
};

struct SSettingsWarmupFrameBudget
{
	int m_MaxTextContainers = 8;
	int m_MaxRenderTargetRecords = 1;
	int m_MaxGpuUploads = 1;
	int m_MaxJobResultMerges = 1;
	ESettingsWarmupStopReason m_StopReason = ESettingsWarmupStopReason::NONE;
};

struct SSettingsPageRuntimeRegistry
{
	std::vector<int> m_vPages;
};

SSettingsPageRuntimeRegistry BuildSettingsPageRuntimeRegistry();
bool SettingsPageRuntimeRegistryContains(const SSettingsPageRuntimeRegistry &Registry, int Page);
bool SettingsWarmupConsumeBudget(SSettingsWarmupFrameBudget &Budget, ESettingsWarmupCost Cost);
const char *SettingsWarmupMissReasonName(ESettingsWarmupMissReason Reason);
std::string SettingsPageCacheKey(int Page, int Tab);
std::string SettingsSectionCacheKey(int Page, int Tab, const char *pSection);
std::string SettingsTextCacheKey(int Page, int Tab, const char *pTextId);
std::string SettingsResourceCacheKey(int Page, const char *pResourceId);

#endif // GAME_CLIENT_COMPONENTS_SETTINGS_RUNTIME_CACHE_H
