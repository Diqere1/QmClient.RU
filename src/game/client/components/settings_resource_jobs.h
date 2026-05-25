#ifndef GAME_CLIENT_COMPONENTS_SETTINGS_RESOURCE_JOBS_H
#define GAME_CLIENT_COMPONENTS_SETTINGS_RESOURCE_JOBS_H

#include <game/client/components/settings_runtime_cache.h>

#include <string>
#include <vector>

struct SSettingsSkinListEntry
{
	std::string m_Name;
	bool m_Selected = false;
	bool m_Favorite = false;
};

struct SSettingsSkinListPlan
{
	std::vector<std::string> m_vNames;
};

struct SSettingsSkinListPlanResult
{
	int m_Generation = 0;
	SSettingsSkinListPlan m_Plan;
};

struct SSettingsResourceMergeBudget
{
	int m_MaxListEntries = 64;
	int m_MaxGpuUploads = 1;
	ESettingsWarmupStopReason m_StopReason = ESettingsWarmupStopReason::NONE;
	bool m_FrameMergeBudgetConsumed = false;
};

SSettingsSkinListPlan BuildSettingsSkinListPlan(std::vector<SSettingsSkinListEntry> vEntries);
std::vector<int> BuildSettingsCountryFlagWarmupPlan(const std::vector<int> &vCountryCodes);
bool SettingsResourceConsumeMergeEntry(SSettingsResourceMergeBudget &Budget);
bool SettingsResourceConsumeGpuUpload(SSettingsResourceMergeBudget &Budget);
bool SettingsResourceConsumeMergeEntry(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget);
bool SettingsResourceConsumeGpuUpload(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget);
bool SettingsResourceConsumeGpuUploads(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget, int Count);
bool SettingsSkinListPlanGenerationMatches(const SSettingsSkinListPlanResult &Result, int CurrentGeneration);
bool SettingsAssetListJobGenerationMatches(int JobGeneration, int CurrentGeneration);
bool SettingsSkinListShouldPublishMergedList(size_t Cursor, size_t Total);
bool SettingsSkinListShouldRequestImmediateLoad(bool Visible);
bool SettingsAssetListShouldShowBlockingLoading(bool Loading, int VisibleEntries);
bool SettingsAssetListCanStartPreviewDecode(bool Loading, bool Merging, bool Loaded);
bool SettingsAssetPreviewShouldDeferFinalize(int FinalizedThisFrame, double ElapsedMs, int MaxFinalizesPerFrame, double MaxFinalizeMsPerFrame);
bool SettingsAssetPreviewShouldPrioritizeVisibleRange(int Index, int FirstVisibleIndex, int LastVisibleIndex);
bool SettingsWorkshopThumbShouldStartHighPriority(int VisibleDownloadableIndex, int FirstVisibleDownloadableIndex, int LastVisibleDownloadableIndex);
bool SettingsPageCacheCanUseRecordedResources(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord);
bool SettingsPageCanUsePageFbo(int Page, int AssetsPage);
const char *SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason StopReason);
bool SettingsAssetWarmupAllTabsReady(const bool *pReadyTabs, int TabCount);
int SettingsAssetWarmupNextTab(int CurrentTab, int TabCount);

#endif // GAME_CLIENT_COMPONENTS_SETTINGS_RESOURCE_JOBS_H
