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

struct SSettingsResourceFrameContext
{
	bool m_ScrollActive = false;
	int m_PostScrollRecoveryFrames = 0;
};

enum class ESettingsResourcePriority
{
	BACKGROUND,
	PREFETCH,
	VISIBLE,
};

struct SSettingsAssetPreviewHandle
{
	int m_Tab = -1;
	unsigned m_Epoch = 0;
	size_t m_Index = 0;
	std::string m_Name;
};

float SettingsSkinPreviewSize(float RowHeight, float PreviewWidth, float RequestedSize);
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
bool SettingsSkinListShouldReplacePublishedEntries(int PublishedEntries, int PendingEntries, bool DirectoryScanPending, bool MergeComplete);
bool SettingsSkinListHasPendingMergeWork(bool HasPendingPlan, size_t PendingNames, size_t PendingEntries, size_t Cursor);
int SettingsSkinListFirstPageWarmupEntries(float ListHeight, float RowHeight, int ItemsPerRow, int ExtraRows);
int SettingsSkinListPrefetchCount(int FirstVisibleIndex, int LastVisibleIndex, int ItemsPerRow, int PrefetchRows, int TotalEntries);
int SettingsSkinListBackgroundWarmupCount(int TotalEntries, int MaxEntriesPerFrame);
bool SettingsSkinBackgroundWarmupShouldRun(bool PageVisible, bool VisibleBacklog, bool InputActive, bool PreviewCacheMaintenanceAllowed);
bool SettingsSkinBackgroundWarmupWindowFull(size_t Loaded, size_t Loading, size_t Pending, int LoadedMax);
bool SettingsSkinListHasProgressiveWarmEntries(int PublishedEntries, int RequestedEntries, int PlannedEntries);
bool SettingsSkinListSelectionStillValid(int SelectedIndex, int EntryCount);
bool SettingsSkinListScrollResetNeeded(int PreviousCount, int CurrentCount, bool ListActive, bool ScrollbarActive);
bool SettingsSkinListShouldRequestImmediateLoad(bool Visible, bool Prefetched);
int SettingsSkinFinalizeMaxPerFrame(bool TeeSettingsActive);
int SettingsSkinGpuUploadUnits(bool TeeSettingsActive);
void SettingsApplyActiveTeeSkinFrameBudget(SSettingsWarmupFrameBudget &Budget, bool TeeSettingsActive);
bool SettingsSkinFinalizeShouldDeferBackgroundSweep(bool ProcessedHighPrioritySkin, int ProcessedThisFrame, int MaxPerFrame);
bool SettingsAssetListShouldShowBlockingLoading(bool Loading, int VisibleEntries);
bool SettingsAssetListCanStartPreviewDecode(bool Loading, bool Merging, bool Loaded);
bool SettingsAssetPreviewShouldDeferFinalize(int FinalizedThisFrame, double ElapsedMs, int MaxFinalizesPerFrame, double MaxFinalizeMsPerFrame);
bool SettingsAssetPreviewShouldPrioritizeVisibleRange(int Index, int FirstVisibleIndex, int LastVisibleIndex);
bool SettingsAssetPreviewShouldUploadHighPriorityFirst(bool CurrentHighPriority, bool CandidateHighPriority);
bool SettingsWorkshopThumbShouldStartHighPriority(int VisibleDownloadableIndex, int FirstVisibleDownloadableIndex, int LastVisibleDownloadableIndex);
bool SettingsResourceCanUseHighPriorityBudget(int StartedThisFrame, int NormalBudget, int HighPriorityBudget, bool HighPriority);
int SettingsResourceFrameStageBudget(const SSettingsResourceFrameContext &Context, ESettingsResourcePriority Priority, int NormalBudget, int ScrollActiveVisibleBudget);
int SettingsScrollInteractionCooldown(bool ActiveThisFrame, int CurrentCooldownFrames, int CooldownFrames);
int SettingsScrollInteractionRecovery(bool ScrollActiveThisFrame, int PreviousCooldownFrames, int CurrentCooldownFrames, int CurrentRecoveryFrames, int RecoveryFrames);
int SettingsResourceSharedHeavyBudget(const SSettingsResourceFrameContext &Context, int NormalBudget, int RecoveryBudget);
bool SettingsResourceConsumeSharedHeavyBudget(int &RemainingBudget);
bool SettingsResourceUploadWithinByteBudget(int UploadedThisFrame, size_t UploadedBytesThisFrame, size_t ItemBytes, size_t MaxBytesPerFrame);
std::string SettingsAssetPreviewHandleKey(const SSettingsAssetPreviewHandle &Handle);
bool SettingsAssetPreviewHandleMatches(const SSettingsAssetPreviewHandle &Handle, int CurrentTab, unsigned CurrentEpoch, size_t CurrentIndex, const char *pName);
bool SettingsPageCacheCanUseRecordedResources(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord, bool DependenciesReadyAtRecord = true);
ESettingsWarmupMissReason SettingsPageRecordedCacheMissReason(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord, bool DependenciesReadyAtRecord = true);
bool SettingsPageCanUsePageFbo(int Page, int AssetsPage, int DynamicPreviewPage = -1, int Tab = -1);
const char *SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason StopReason);
bool SettingsAssetWarmupAllTabsReady(const bool *pReadyTabs, int TabCount);
int SettingsAssetWarmupNextTab(int CurrentTab, int TabCount);

#endif // GAME_CLIENT_COMPONENTS_SETTINGS_RESOURCE_JOBS_H
