#include <game/client/components/settings_resource_jobs.h>
#include <game/client/components/menus.h>

#include <algorithm>
#include <cmath>

float SettingsSkinPreviewSize(float RowHeight, float PreviewWidth, float RequestedSize)
{
	const float MaxSize = std::max(0.0f, std::min(RowHeight, PreviewWidth) - 10.0f);
	return std::clamp(RequestedSize, 0.0f, MaxSize);
}

SSettingsSkinListPlan BuildSettingsSkinListPlan(std::vector<SSettingsSkinListEntry> vEntries)
{
	std::stable_sort(vEntries.begin(), vEntries.end(), [](const SSettingsSkinListEntry &A, const SSettingsSkinListEntry &B) {
		if(A.m_Selected != B.m_Selected)
			return A.m_Selected && !B.m_Selected;
		if(A.m_Favorite != B.m_Favorite)
			return A.m_Favorite && !B.m_Favorite;
		return A.m_Name < B.m_Name;
	});

	SSettingsSkinListPlan Plan;
	Plan.m_vNames.reserve(vEntries.size());
	for(const SSettingsSkinListEntry &Entry : vEntries)
		Plan.m_vNames.push_back(Entry.m_Name);
	return Plan;
}

std::vector<int> BuildSettingsCountryFlagWarmupPlan(const std::vector<int> &vCountryCodes)
{
	std::vector<int> vPlan;
	vPlan.reserve(vCountryCodes.size());
	for(int CountryCode : vCountryCodes)
	{
		if(std::find(vPlan.begin(), vPlan.end(), CountryCode) == vPlan.end())
			vPlan.push_back(CountryCode);
	}
	return vPlan;
}

bool SettingsResourceConsumeMergeEntry(SSettingsResourceMergeBudget &Budget)
{
	if(Budget.m_MaxListEntries <= 0)
	{
		Budget.m_StopReason = ESettingsWarmupStopReason::MERGE_BUDGET;
		return false;
	}

	--Budget.m_MaxListEntries;
	return true;
}

bool SettingsResourceConsumeGpuUpload(SSettingsResourceMergeBudget &Budget)
{
	if(Budget.m_MaxGpuUploads <= 0)
	{
		Budget.m_StopReason = ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET;
		return false;
	}

	--Budget.m_MaxGpuUploads;
	return true;
}

bool SettingsResourceConsumeMergeEntry(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget)
{
	if(pFrameBudget != nullptr && !Budget.m_FrameMergeBudgetConsumed)
	{
		if(!SettingsWarmupConsumeBudget(*pFrameBudget, ESettingsWarmupCost::JOB_RESULT_MERGE))
		{
			Budget.m_StopReason = pFrameBudget->m_StopReason;
			return false;
		}
		Budget.m_FrameMergeBudgetConsumed = true;
	}
	if(!SettingsResourceConsumeMergeEntry(Budget))
		return false;
	return true;
}

bool SettingsResourceConsumeGpuUpload(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget)
{
	if(!SettingsResourceConsumeGpuUpload(Budget))
		return false;
	if(pFrameBudget == nullptr)
		return true;
	if(SettingsWarmupConsumeBudget(*pFrameBudget, ESettingsWarmupCost::GPU_UPLOAD))
		return true;
	Budget.m_StopReason = pFrameBudget->m_StopReason;
	++Budget.m_MaxGpuUploads;
	return false;
}

bool SettingsResourceConsumeGpuUploads(SSettingsResourceMergeBudget &Budget, SSettingsWarmupFrameBudget *pFrameBudget, int Count)
{
	if(Count <= 0)
		return true;
	for(int Upload = 0; Upload < Count; ++Upload)
	{
		if(!SettingsResourceConsumeGpuUpload(Budget, pFrameBudget))
			return false;
	}
	return true;
}

bool SettingsSkinListPlanGenerationMatches(const SSettingsSkinListPlanResult &Result, int CurrentGeneration)
{
	return Result.m_Generation == CurrentGeneration;
}

bool SettingsAssetListJobGenerationMatches(int JobGeneration, int CurrentGeneration)
{
	return JobGeneration == CurrentGeneration;
}

bool SettingsSkinListShouldPublishMergedList(size_t Cursor, size_t Total)
{
	return Cursor >= Total;
}

bool SettingsSkinListShouldReplacePublishedEntries(int PublishedEntries, int PendingEntries, bool DirectoryScanPending, bool MergeComplete)
{
	(void)PublishedEntries;
	(void)PendingEntries;
	(void)DirectoryScanPending;
	return MergeComplete;
}

bool SettingsSkinListHasPendingMergeWork(bool HasPendingPlan, size_t PendingNames, size_t PendingEntries, size_t Cursor)
{
	if(!HasPendingPlan)
		return false;
	return PendingNames == 0 || Cursor < PendingNames || PendingEntries < PendingNames;
}

int SettingsSkinListFirstPageWarmupEntries(float ListHeight, float RowHeight, int ItemsPerRow, int ExtraRows)
{
	if(ListHeight <= 0.0f || RowHeight <= 0.0f || ItemsPerRow <= 0)
		return 0;

	const int VisibleRows = std::max(1, (int)std::ceil(ListHeight / RowHeight));
	const int WarmRows = std::max(1, VisibleRows + std::max(0, ExtraRows));
	return WarmRows * ItemsPerRow;
}

int SettingsSkinListPrefetchCount(int FirstVisibleIndex, int LastVisibleIndex, int ItemsPerRow, int PrefetchRows, int TotalEntries)
{
	if(FirstVisibleIndex < 0 || LastVisibleIndex < FirstVisibleIndex || ItemsPerRow <= 0 || PrefetchRows <= 0 || TotalEntries <= 0)
		return 0;

	const int PrefetchItems = ItemsPerRow * PrefetchRows;
	const int PrefetchStart = LastVisibleIndex + 1;
	if(PrefetchStart >= TotalEntries)
		return 0;
	const int Remaining = TotalEntries - PrefetchStart;
	return std::min(PrefetchItems, Remaining);
}

int SettingsSkinListBackgroundWarmupCount(int TotalEntries, int MaxEntriesPerFrame)
{
	if(TotalEntries <= 0 || MaxEntriesPerFrame <= 0)
		return 0;
	return std::min(TotalEntries, MaxEntriesPerFrame);
}

bool SettingsSkinBackgroundWarmupShouldRun(bool PageVisible, bool VisibleBacklog, bool InputActive, bool PreviewCacheMaintenanceAllowed)
{
	(void)PreviewCacheMaintenanceAllowed;
	return PageVisible && !VisibleBacklog && !InputActive;
}

bool SettingsSkinBackgroundWarmupWindowFull(size_t Loaded, size_t Loading, size_t Pending, int LoadedMax)
{
	return LoadedMax > 0 && Loaded + Loading + Pending >= (size_t)LoadedMax;
}

bool SettingsSkinListHasProgressiveWarmEntries(int PublishedEntries, int RequestedEntries, int PlannedEntries)
{
	if(PublishedEntries <= 0)
		return false;
	if(RequestedEntries <= 0)
		return true;
	if(PlannedEntries > 0 && PublishedEntries >= PlannedEntries)
		return true;
	return PublishedEntries >= RequestedEntries;
}
bool SettingsSkinListSelectionStillValid(int SelectedIndex, int EntryCount)
{
	return SelectedIndex < 0 || (SelectedIndex >= 0 && SelectedIndex < EntryCount);
}

bool SettingsSkinListScrollResetNeeded(int PreviousCount, int CurrentCount, bool ListActive, bool ScrollbarActive)
{
	if(CurrentCount >= PreviousCount)
		return false;
	if(ScrollbarActive)
		return false;
	return !ListActive;
}


bool SettingsSkinListShouldRequestImmediateLoad(bool Visible, bool Prefetched)
{
	return Visible || Prefetched;
}

int SettingsSkinFinalizeMaxPerFrame(bool TeeSettingsActive)
{
	return TeeSettingsActive ? 24 : 12;
}

int SettingsSkinGpuUploadUnits(bool TeeSettingsActive)
{
	return TeeSettingsActive ? 28 : 14;
}

void SettingsApplyActiveTeeSkinFrameBudget(SSettingsWarmupFrameBudget &Budget, bool TeeSettingsActive)
{
	if(!TeeSettingsActive)
		return;

	Budget.m_MaxGpuUploads = SettingsSkinGpuUploadUnits(true);
	Budget.m_MaxGpuReadbacks = 1;
	Budget.m_MaxPreviewCacheIo = 1;
	Budget.m_MaxJobResultMerges = 2;
}

bool SettingsSkinFinalizeShouldDeferBackgroundSweep(bool ProcessedHighPrioritySkin, int ProcessedThisFrame, int MaxPerFrame)
{
	return ProcessedHighPrioritySkin && ProcessedThisFrame > 0 && ProcessedThisFrame < MaxPerFrame;
}

bool SettingsAssetListShouldShowBlockingLoading(bool Loading, int VisibleEntries)
{
	return Loading && VisibleEntries <= 0;
}

bool SettingsAssetListCanStartPreviewDecode(bool Loading, bool Merging, bool Loaded)
{
	return !Loading && !Merging && Loaded;
}

bool SettingsAssetPreviewShouldDeferFinalize(int FinalizedThisFrame, double ElapsedMs, int MaxFinalizesPerFrame, double MaxFinalizeMsPerFrame)
{
	return FinalizedThisFrame >= MaxFinalizesPerFrame ||
	       (FinalizedThisFrame > 0 && ElapsedMs >= MaxFinalizeMsPerFrame);
}

bool SettingsAssetPreviewShouldPrioritizeVisibleRange(int Index, int FirstVisibleIndex, int LastVisibleIndex)
{
	return FirstVisibleIndex >= 0 && LastVisibleIndex >= FirstVisibleIndex && Index >= FirstVisibleIndex && Index <= LastVisibleIndex;
}

bool SettingsAssetPreviewShouldUploadHighPriorityFirst(bool CurrentHighPriority, bool CandidateHighPriority)
{
	return !CurrentHighPriority && CandidateHighPriority;
}

bool SettingsWorkshopThumbShouldStartHighPriority(int VisibleDownloadableIndex, int FirstVisibleDownloadableIndex, int LastVisibleDownloadableIndex)
{
	return SettingsAssetPreviewShouldPrioritizeVisibleRange(VisibleDownloadableIndex, FirstVisibleDownloadableIndex, LastVisibleDownloadableIndex);
}

bool SettingsResourceCanUseHighPriorityBudget(int StartedThisFrame, int NormalBudget, int HighPriorityBudget, bool HighPriority)
{
	if(StartedThisFrame < NormalBudget)
		return true;
	return HighPriority && StartedThisFrame < HighPriorityBudget;
}

int SettingsResourceFrameStageBudget(const SSettingsResourceFrameContext &Context, ESettingsResourcePriority Priority, int NormalBudget, int ScrollActiveVisibleBudget)
{
	if(!Context.m_ScrollActive)
		return std::max(0, NormalBudget);
	if(Priority != ESettingsResourcePriority::VISIBLE)
		return 0;
	return std::max(0, ScrollActiveVisibleBudget);
}

int SettingsScrollInteractionCooldown(bool ActiveThisFrame, int CurrentCooldownFrames, int CooldownFrames)
{
	if(ActiveThisFrame)
		return std::max(0, CooldownFrames);
	if(CurrentCooldownFrames <= 0)
		return 0;
	return CurrentCooldownFrames - 1;
}

int SettingsScrollInteractionRecovery(bool ScrollActiveThisFrame, int PreviousCooldownFrames, int CurrentCooldownFrames, int CurrentRecoveryFrames, int RecoveryFrames)
{
	if(ScrollActiveThisFrame || CurrentCooldownFrames > 0)
		return 0;
	if(PreviousCooldownFrames > 0)
		return std::max(0, RecoveryFrames);
	if(CurrentRecoveryFrames <= 0)
		return 0;
	return CurrentRecoveryFrames - 1;
}

int SettingsResourceSharedHeavyBudget(const SSettingsResourceFrameContext &Context, int NormalBudget, int RecoveryBudget)
{
	if(Context.m_ScrollActive)
		return 0;
	if(Context.m_PostScrollRecoveryFrames > 0)
		return std::max(0, RecoveryBudget);
	return std::max(0, NormalBudget);
}

bool SettingsResourceConsumeSharedHeavyBudget(int &RemainingBudget)
{
	if(RemainingBudget <= 0)
		return false;
	--RemainingBudget;
	return true;
}

bool SettingsResourceUploadWithinByteBudget(int UploadedThisFrame, size_t UploadedBytesThisFrame, size_t ItemBytes, size_t MaxBytesPerFrame)
{
	if(MaxBytesPerFrame == 0)
		return false;
	if(ItemBytes > MaxBytesPerFrame)
		return UploadedThisFrame == 0;
	return UploadedThisFrame == 0 || UploadedBytesThisFrame + ItemBytes <= MaxBytesPerFrame;
}

std::string SettingsAssetPreviewHandleKey(const SSettingsAssetPreviewHandle &Handle)
{
	return std::to_string(Handle.m_Tab) + ":" + std::to_string(Handle.m_Epoch) + ":" + std::to_string(Handle.m_Index) + ":" + Handle.m_Name;
}

bool SettingsAssetPreviewHandleMatches(const SSettingsAssetPreviewHandle &Handle, int CurrentTab, unsigned CurrentEpoch, size_t CurrentIndex, const char *pName)
{
	return pName != nullptr &&
	       Handle.m_Tab == CurrentTab &&
	       Handle.m_Epoch == CurrentEpoch &&
	       Handle.m_Index == CurrentIndex &&
	       Handle.m_Name == pName;
}

bool SettingsPageCacheCanUseRecordedResources(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord, bool DependenciesReadyAtRecord)
{
	return CacheMatches && RenderTargetValid && ResourcesReadyAtRecord && DependenciesReadyAtRecord;
}

ESettingsWarmupMissReason SettingsPageRecordedCacheMissReason(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord, bool DependenciesReadyAtRecord)
{
	if(SettingsPageCacheCanUseRecordedResources(CacheMatches, RenderTargetValid, ResourcesReadyAtRecord, DependenciesReadyAtRecord))
		return ESettingsWarmupMissReason::NONE;
	if(CacheMatches && RenderTargetValid)
	{
		if(!DependenciesReadyAtRecord)
			return ESettingsWarmupMissReason::DEPENDENCY_NOT_READY;
		if(!ResourcesReadyAtRecord)
			return ESettingsWarmupMissReason::RESOURCE_PLAN_PENDING;
	}
	return ESettingsWarmupMissReason::PAGE_FBO_NOT_READY;
}

bool SettingsPageCanUsePageFbo(int Page, int AssetsPage, int DynamicPreviewPage, int Tab)
{
	const bool IsTClientSettingsPage = Page == CMenus::SETTINGS_TCLIENT && Tab == 0;
	return Page >= 0 && Page != AssetsPage && Page != DynamicPreviewPage && !IsTClientSettingsPage;
}

const char *SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason StopReason)
{
	switch(StopReason)
	{
	case ESettingsWarmupStopReason::NONE: return "none";
	case ESettingsWarmupStopReason::TEXT_BUDGET: return "text_budget";
	case ESettingsWarmupStopReason::FBO_BUDGET: return "fbo_budget";
	case ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET: return "gpu_upload_budget";
	case ESettingsWarmupStopReason::GPU_READBACK_BUDGET: return "gpu_readback_budget";
	case ESettingsWarmupStopReason::PREVIEW_CACHE_IO_BUDGET: return "preview_cache_io_budget";
	case ESettingsWarmupStopReason::MERGE_BUDGET: return "merge_budget";
	case ESettingsWarmupStopReason::ACTIVE_ITEM: return "active_item";
	}
	return "unknown";
}

bool SettingsAssetWarmupAllTabsReady(const bool *pReadyTabs, int TabCount)
{
	if(pReadyTabs == nullptr || TabCount <= 0)
		return true;
	for(int Tab = 0; Tab < TabCount; ++Tab)
	{
		if(!pReadyTabs[Tab])
			return false;
	}
	return true;
}

int SettingsAssetWarmupNextTab(int CurrentTab, int TabCount)
{
	if(TabCount <= 0)
		return -1;
	if(CurrentTab < 0 || CurrentTab >= TabCount)
		return 0;
	return (CurrentTab + 1) % TabCount;
}
