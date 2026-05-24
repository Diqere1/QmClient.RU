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
