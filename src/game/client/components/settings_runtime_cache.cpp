#include "settings_runtime_cache.h"

#include <game/client/components/menus.h>

#include <algorithm>
#include <cstdio>
#include <utility>

static void AddUnique(std::vector<int> &vValues, int Value)
{
	if(std::find(vValues.begin(), vValues.end(), Value) == vValues.end())
		vValues.push_back(Value);
}

static std::string BuildRuntimeCacheKey(const char *pPrefix, const char *pPageName, int Tab, const char *pSuffix, const char *pId)
{
	std::string Key = pPrefix;
	Key += pPageName;
	if(Tab >= 0)
	{
		char aTab[32];
		std::snprintf(aTab, sizeof(aTab), ":tab:%d", Tab);
		Key += aTab;
	}
	Key += pSuffix;
	Key += pId != nullptr ? pId : "";
	return Key;
}

SSettingsPageRuntimeRegistry BuildSettingsPageRuntimeRegistry()
{
	SSettingsPageRuntimeRegistry Registry;
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_LANGUAGE);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_PLAYER);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_TEE);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_GENERAL);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_CONTROLS);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_GRAPHICS);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_SOUND);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_DDNET);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_TCLIENT);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_QMCLIENT);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_APPEARANCE);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_ASSETS);
	return Registry;
}

bool SettingsPageRuntimeRegistryContains(const SSettingsPageRuntimeRegistry &Registry, int Page)
{
	return std::find(Registry.m_vPages.begin(), Registry.m_vPages.end(), Page) != Registry.m_vPages.end();
}

int SettingsPageRuntimeCacheSlot(int Page, int Tab)
{
	switch(Page)
	{
	case CMenus::SETTINGS_LANGUAGE: return 0;
	case CMenus::SETTINGS_GENERAL: return 1;
	case CMenus::SETTINGS_PLAYER: return 2;
	case CMenus::SETTINGS_TEE: return 3;
	case CMenus::SETTINGS_APPEARANCE: return 4;
	case CMenus::SETTINGS_CONTROLS: return 5;
	case CMenus::SETTINGS_GRAPHICS: return 6;
	case CMenus::SETTINGS_SOUND: return 7;
	case CMenus::SETTINGS_DDNET: return 8;
	case CMenus::SETTINGS_ASSETS: return 9;
	case CMenus::SETTINGS_TCLIENT:
		if(Tab < 0)
			return 10;
		if(Tab >= 6)
			return -1;
		return 10 + Tab;
	case CMenus::SETTINGS_QMCLIENT:
		if(Tab < 0)
			return 16;
		if(Tab >= CMenus::NUMBER_OF_QMCLIENT_SETTINGS_TABS)
			return -1;
		return 16 + Tab;
	default:
		return -1;
	}
}

bool SettingsWarmupConsumeBudget(SSettingsWarmupFrameBudget &Budget, ESettingsWarmupCost Cost)
{
	int *pBudgetCounter = nullptr;
	ESettingsWarmupStopReason StopReason = ESettingsWarmupStopReason::NONE;
	switch(Cost)
	{
	case ESettingsWarmupCost::TEXT_CONTAINER:
		pBudgetCounter = &Budget.m_MaxTextContainers;
		StopReason = ESettingsWarmupStopReason::TEXT_BUDGET;
		break;
	case ESettingsWarmupCost::RENDER_TARGET_RECORD:
		pBudgetCounter = &Budget.m_MaxRenderTargetRecords;
		StopReason = ESettingsWarmupStopReason::FBO_BUDGET;
		break;
	case ESettingsWarmupCost::GPU_UPLOAD:
		pBudgetCounter = &Budget.m_MaxGpuUploads;
		StopReason = ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET;
		break;
	case ESettingsWarmupCost::JOB_RESULT_MERGE:
		pBudgetCounter = &Budget.m_MaxJobResultMerges;
		StopReason = ESettingsWarmupStopReason::MERGE_BUDGET;
		break;
	}

	if(pBudgetCounter == nullptr)
		return false;
	if(*pBudgetCounter <= 0)
	{
		Budget.m_StopReason = StopReason;
		return false;
	}

	--(*pBudgetCounter);
	return true;
}

const char *SettingsWarmupMissReasonName(ESettingsWarmupMissReason Reason)
{
	switch(Reason)
	{
	case ESettingsWarmupMissReason::NONE: return "none";
	case ESettingsWarmupMissReason::PAGE_FBO_UNSUPPORTED: return "page_fbo_unsupported";
	case ESettingsWarmupMissReason::PAGE_FBO_NOT_READY: return "page_fbo_not_ready";
	case ESettingsWarmupMissReason::SECTION_FBO_NOT_READY: return "section_fbo_not_ready";
	case ESettingsWarmupMissReason::RESOURCE_PLAN_PENDING: return "resource_plan_pending";
	case ESettingsWarmupMissReason::JOB_RESULT_PENDING: return "job_result_pending";
	case ESettingsWarmupMissReason::GPU_UPLOAD_BUDGET: return "gpu_upload_budget";
	case ESettingsWarmupMissReason::TEXT_BUDGET: return "text_budget";
	case ESettingsWarmupMissReason::ACTIVE_ITEM: return "active_item";
	case ESettingsWarmupMissReason::INVALID_RUNTIME_KEY: return "invalid_runtime_key";
	}
	return "unknown";
}

const char *SettingsTClientPerfStageName(ETClientSettingsPerfStage Stage)
{
	switch(Stage)
	{
	case ETClientSettingsPerfStage::TAB_SHELL: return "tclient_tab_shell";
	case ETClientSettingsPerfStage::SECTION_LAYOUT: return "tclient_section_layout";
	case ETClientSettingsPerfStage::TEXT_CACHE: return "tclient_text_cache";
	case ETClientSettingsPerfStage::RESOURCE_PRETRIGGER: return "tclient_resource_pretrigger";
	case ETClientSettingsPerfStage::STATIC_LAYER: return "tclient_static_layer";
	case ETClientSettingsPerfStage::INTERACTIVE_LAYER: return "tclient_interactive_layer";
	}
	return "tclient_unknown";
}

static std::string SettingsRuntimePageName(int Page)
{
	switch(Page)
	{
	case CMenus::SETTINGS_LANGUAGE: return "language";
	case CMenus::SETTINGS_GENERAL: return "general";
	case CMenus::SETTINGS_PLAYER: return "player";
	case CMenus::SETTINGS_TEE: return "tee";
	case CMenus::SETTINGS_APPEARANCE: return "appearance";
	case CMenus::SETTINGS_CONTROLS: return "controls";
	case CMenus::SETTINGS_GRAPHICS: return "graphics";
	case CMenus::SETTINGS_SOUND: return "sound";
	case CMenus::SETTINGS_DDNET: return "ddnet";
	case CMenus::SETTINGS_ASSETS: return "assets";
	case CMenus::SETTINGS_TCLIENT: return "tclient";
	case CMenus::SETTINGS_QMCLIENT: return "qmclient";
	default: return "unknown";
	}
}

std::string SettingsPageCacheKey(int Page, int Tab)
{
	const std::string PageName = SettingsRuntimePageName(Page);
	if(Tab >= 0)
		return BuildRuntimeCacheKey("settings:", PageName.c_str(), Tab, "", "");
	return std::string("settings:") + PageName;
}

std::string SettingsSectionCacheKey(int Page, int Tab, const char *pSection)
{
	return BuildRuntimeCacheKey("settings:", SettingsRuntimePageName(Page).c_str(), Tab, ":section:", pSection);
}

std::string SettingsTextCacheKey(int Page, int Tab, const char *pTextId)
{
	return BuildRuntimeCacheKey("settings:", SettingsRuntimePageName(Page).c_str(), Tab, ":text:", pTextId);
}

std::string SettingsResourceCacheKey(int Page, const char *pResourceId)
{
	return BuildRuntimeCacheKey("settings:", SettingsRuntimePageName(Page).c_str(), -1, ":resource:", pResourceId);
}
