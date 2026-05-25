#include <game/client/components/settings_resource_jobs.h>

#include <algorithm>

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

bool SettingsSkinListShouldRequestImmediateLoad(bool Visible)
{
	return Visible;
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

bool SettingsPageCacheCanUseRecordedResources(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord)
{
	return CacheMatches && RenderTargetValid && ResourcesReadyAtRecord;
}

bool SettingsPageCanUsePageFbo(int Page, int AssetsPage)
{
	return Page >= 0 && Page != AssetsPage;
}

const char *SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason StopReason)
{
	switch(StopReason)
	{
	case ESettingsWarmupStopReason::NONE: return "none";
	case ESettingsWarmupStopReason::TEXT_BUDGET: return "text_budget";
	case ESettingsWarmupStopReason::FBO_BUDGET: return "fbo_budget";
	case ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET: return "gpu_upload_budget";
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
