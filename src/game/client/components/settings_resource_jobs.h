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
};

SSettingsSkinListPlan BuildSettingsSkinListPlan(std::vector<SSettingsSkinListEntry> vEntries);
std::vector<int> BuildSettingsCountryFlagWarmupPlan(const std::vector<int> &vCountryCodes);
bool SettingsResourceConsumeMergeEntry(SSettingsResourceMergeBudget &Budget);
bool SettingsResourceConsumeGpuUpload(SSettingsResourceMergeBudget &Budget);
bool SettingsSkinListPlanGenerationMatches(const SSettingsSkinListPlanResult &Result, int CurrentGeneration);

#endif // GAME_CLIENT_COMPONENTS_SETTINGS_RESOURCE_JOBS_H
