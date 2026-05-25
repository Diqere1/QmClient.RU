#include <game/client/components/settings_resource_jobs.h>

#include <algorithm>

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

bool SettingsSkinListPlanGenerationMatches(const SSettingsSkinListPlanResult &Result, int CurrentGeneration)
{
	return Result.m_Generation == CurrentGeneration;
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

bool SettingsPageCacheCanUseRecordedResources(bool CacheMatches, bool RenderTargetValid, bool ResourcesReadyAtRecord)
{
	return CacheMatches && RenderTargetValid && ResourcesReadyAtRecord;
}

bool SettingsPageCanUsePageFbo(int Page, int AssetsPage)
{
	return Page != AssetsPage;
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
