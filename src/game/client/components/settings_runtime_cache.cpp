#include "settings_runtime_cache.h"

#include <base/system.h>
#include <game/client/components/menus.h>

#include <algorithm>
#include <cstdio>
#include <engine/shared/config.h>
#include <utility>

static void AddUnique(std::vector<int> &vValues, int Value)
{
	if(std::find(vValues.begin(), vValues.end(), Value) == vValues.end())
		vValues.push_back(Value);
}

static void AddSection(std::vector<SSettingsSectionEntry> &vSections, int Page, int Tab, const char *pSection, bool HasStaticRenderer, bool HasInteractiveRenderer)
{
	for(const auto &Section : vSections)
	{
		if(Section.m_Page == Page && Section.m_Tab == Tab && Section.m_Id == pSection)
			return;
	}

	SSettingsSectionEntry Entry;
	Entry.m_Page = Page;
	Entry.m_Tab = Tab;
	Entry.m_Id = pSection;
	Entry.m_HasStaticRenderer = HasStaticRenderer;
	Entry.m_HasInteractiveRenderer = HasInteractiveRenderer;
	vSections.push_back(std::move(Entry));
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

static int SettingsWarmupPageTab(const SSettingsRuntimeCacheMetadata &Metadata, int Page)
{
	if(Page == CMenus::SETTINGS_TCLIENT)
		return Metadata.m_LastTClientTab;
	if(Page == CMenus::SETTINGS_QMCLIENT)
		return Metadata.m_LastQmTab;
	return -1;
}

static int CanonicalizeSettingsWarmupPage(int Page)
{
	return SettingsCanonicalPage(Page);
}

static void AddWarmupPageJob(std::vector<SSettingsWarmupPageJob> &vJobs, const SSettingsRuntimeCacheMetadata &Metadata, int Page, bool UseScroll)
{
	for(const SSettingsWarmupPageJob &Job : vJobs)
	{
		if(Job.m_Page == Page)
			return;
	}

	SSettingsWarmupPageJob Job;
	Job.m_Page = Page;
	Job.m_Tab = SettingsWarmupPageTab(Metadata, Page);
	Job.m_ScrollY = UseScroll && Metadata.m_LastScrollPage == Page && SettingsPageUsesRuntimeScroll(Page) ? Metadata.m_LastScrollY : 0.0f;
	vJobs.push_back(Job);
}

SSettingsPageRuntimeRegistry BuildSettingsPageRuntimeRegistry()
{
	SSettingsPageRuntimeRegistry Registry;
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_GENERAL);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_TEE);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_CONTROLS);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_GRAPHICS);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_SOUND);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_ASSETS);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_TCLIENT);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_QMCLIENT);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_APPEARANCE);
	AddUnique(Registry.m_vPages, CMenus::SETTINGS_DDNET);
	return Registry;
}

bool SettingsPageRuntimeRegistryContains(const SSettingsPageRuntimeRegistry &Registry, int Page)
{
	return std::find(Registry.m_vPages.begin(), Registry.m_vPages.end(), Page) != Registry.m_vPages.end();
}

int SettingsCanonicalPage(int Page)
{
	switch(Page)
	{
	case CMenus::SETTINGS_LANGUAGE: return CMenus::SETTINGS_GENERAL;
	case CMenus::SETTINGS_PLAYER: return CMenus::SETTINGS_TEE;
	case CMenus::SETTINGS_CONFIGS:
	case CMenus::SETTINGS_CONTRIBUTORS:
		return CMenus::SETTINGS_QMCLIENT;
	default:
		return Page;
	}
}

bool SettingsPageVisibleInRightTabBar(int Page)
{
	switch(Page)
	{
	case CMenus::SETTINGS_LANGUAGE:
	case CMenus::SETTINGS_PLAYER:
	case CMenus::SETTINGS_PROFILES:
	case CMenus::SETTINGS_CONFIGS:
	case CMenus::SETTINGS_CONTRIBUTORS:
		return false;
	default:
		return Page >= 0 && Page < CMenus::SETTINGS_LENGTH;
	}
}

SSettingsSectionRegistry BuildSettingsSectionRegistry()
{
	SSettingsSectionRegistry Registry;
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 3, "binds", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 0, "auto-reply", true, true);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 0, "pet", true, true);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 0, "theme", true, true);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 0, "misc", true, true);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TCLIENT, 0, "hud", true, true);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_VISUAL, "general", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_CONFIG, "config", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_CONTRIBUTORS, "contributors", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_CONTROLS, -1, "movement", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_CONTROLS, -1, "weapons", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_CONTROLS, -1, "voting", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_ASSETS, -1, "resource-list", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_ASSETS, -1, "preview", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_GENERAL, -1, "language-list", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TEE, -1, "skin-list", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TEE, -1, "identity", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_TEE, -1, "country-list", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_GENERAL, -1, "header", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_GENERAL, -1, "body", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_APPEARANCE, -1, "header", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_APPEARANCE, -1, "body", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_GRAPHICS, -1, "header", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_GRAPHICS, -1, "body", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_SOUND, -1, "header", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_SOUND, -1, "body", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_DDNET, -1, "header", false, false);
	AddSection(Registry.m_vSections, CMenus::SETTINGS_DDNET, -1, "body", false, false);
	return Registry;
}

bool SettingsSectionRegistryContains(const SSettingsSectionRegistry &Registry, int Page, const char *pSection)
{
	return std::find_if(Registry.m_vSections.begin(), Registry.m_vSections.end(), [Page, pSection](const SSettingsSectionEntry &Entry) {
		return Entry.m_Page == Page && Entry.m_Id == pSection;
	}) != Registry.m_vSections.end();
}

bool SettingsSectionCanRecordStaticFbo(const SSettingsSectionRegistry &Registry, int Page, int Tab, const char *pSection)
{
	const auto It = std::find_if(Registry.m_vSections.begin(), Registry.m_vSections.end(), [Page, Tab, pSection](const SSettingsSectionEntry &Entry) {
		return Entry.m_Page == Page &&
			(Entry.m_Tab < 0 || Tab < 0 || Entry.m_Tab == Tab) &&
			Entry.m_Id == pSection;
	});
	if(It == Registry.m_vSections.end())
		return false;
	return It->m_HasStaticRenderer && It->m_HasInteractiveRenderer;
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
	case CMenus::SETTINGS_ASSETS:
		if(Tab < 0)
			return 9;
		if(Tab >= NUMBER_OF_ASSETS_TABS)
			return -1;
		return 22 + Tab;
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

bool SettingsRuntimeCacheKeyMatches(const SSettingsRuntimeCacheKey &A, const SSettingsRuntimeCacheKey &B)
{
	return A.m_LanguageHash == B.m_LanguageHash &&
		A.m_FontGeneration == B.m_FontGeneration &&
		A.m_BackendGeneration == B.m_BackendGeneration &&
		A.m_WindowWidth == B.m_WindowWidth &&
		A.m_WindowHeight == B.m_WindowHeight &&
		A.m_UiScale == B.m_UiScale &&
		A.m_ConfigHash == B.m_ConfigHash;
}

bool SettingsPageUsesRuntimeScroll(int Page)
{
	return Page == CMenus::SETTINGS_TCLIENT;
}

SSettingsWarmupStartupPlan BuildSettingsWarmupStartupPlan(const SSettingsRuntimeCacheMetadata &Metadata, const SSettingsPageRuntimeRegistry &Registry)
{
	SSettingsWarmupStartupPlan Plan;
	const int LastPage = CanonicalizeSettingsWarmupPage(Metadata.m_LastPage);
	if(SettingsPageRuntimeRegistryContains(Registry, LastPage))
		AddWarmupPageJob(Plan.m_vPageJobs, Metadata, LastPage, true);
	for(int Page : Registry.m_vPages)
		AddWarmupPageJob(Plan.m_vPageJobs, Metadata, Page, Page == LastPage);
	return Plan;
}

bool SettingsWarmupPlanContainsPage(const SSettingsWarmupStartupPlan &Plan, int Page)
{
	return std::find_if(Plan.m_vPageJobs.begin(), Plan.m_vPageJobs.end(), [Page](const SSettingsWarmupPageJob &Job) {
		return Job.m_Page == Page;
	}) != Plan.m_vPageJobs.end();
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

const char *SettingsInvalidationReasonName(ESettingsInvalidationReason Reason)
{
	switch(Reason)
	{
	case ESettingsInvalidationReason::LANGUAGE_CHANGED: return "language_changed";
	case ESettingsInvalidationReason::FONT_CHANGED: return "font_changed";
	case ESettingsInvalidationReason::BACKEND_CHANGED: return "backend_changed";
	case ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED: return "window_or_scale_changed";
	case ESettingsInvalidationReason::CONFIG_HASH_CHANGED: return "config_hash_changed";
	case ESettingsInvalidationReason::SECTION_SIZE_CHANGED: return "section_size_changed";
	case ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED: return "resource_directory_changed";
	}
	return "unknown";
}

bool SettingsRuntimeCacheAllowsVisibleCompactText(const char *pRenderName)
{
	(void)pRenderName;
	return false;
}

void LogSettingsResourcePerf(int Page, const char *pJob, int Count, int Budget, int Remaining, ESettingsWarmupMissReason Reason, double DurationMs)
{
	if(g_Config.m_QmPerfDebug == 0)
		return;
	const std::string PageName = SettingsPageCacheKey(Page, -1);
	dbg_msg("perf/settings-resource", "page=%s job=%s count=%d budget=%d remaining=%d reason=%s cost_ms=%.3f",
		PageName.c_str(), pJob != nullptr ? pJob : "unknown", Count, Budget, Remaining, SettingsWarmupMissReasonName(Reason), DurationMs);
}

bool SettingsInvalidationClearsTextPool(ESettingsInvalidationReason Reason)
{
	switch(Reason)
	{
	case ESettingsInvalidationReason::LANGUAGE_CHANGED:
	case ESettingsInvalidationReason::FONT_CHANGED:
	case ESettingsInvalidationReason::BACKEND_CHANGED:
	case ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED:
	case ESettingsInvalidationReason::CONFIG_HASH_CHANGED:
		return true;
	case ESettingsInvalidationReason::SECTION_SIZE_CHANGED:
	case ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED:
		return false;
	}
	return true;
}

bool SettingsInvalidationClearsSectionFbo(ESettingsInvalidationReason Reason)
{
	switch(Reason)
	{
	case ESettingsInvalidationReason::LANGUAGE_CHANGED:
	case ESettingsInvalidationReason::FONT_CHANGED:
	case ESettingsInvalidationReason::BACKEND_CHANGED:
	case ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED:
	case ESettingsInvalidationReason::CONFIG_HASH_CHANGED:
	case ESettingsInvalidationReason::SECTION_SIZE_CHANGED:
	case ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED:
		return true;
	}
	return true;
}

bool SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason Reason)
{
	switch(Reason)
	{
	case ESettingsInvalidationReason::LANGUAGE_CHANGED:
	case ESettingsInvalidationReason::FONT_CHANGED:
	case ESettingsInvalidationReason::BACKEND_CHANGED:
	case ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED:
	case ESettingsInvalidationReason::CONFIG_HASH_CHANGED:
		return true;
	case ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED:
	case ESettingsInvalidationReason::SECTION_SIZE_CHANGED:
		return false;
	}
	return true;
}

bool SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason Reason, int Page, int AssetsPage)
{
	if(Reason == ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED)
		return Page == AssetsPage;
	return SettingsInvalidationClearsPageFbo(Reason);
}

bool SettingsInvalidationClearsResourcePlan(ESettingsInvalidationReason Reason)
{
	return Reason == ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED;
}

bool SettingsWarmupEnabled(int PrewarmConfig, int FboConfig)
{
	return PrewarmConfig != 0 && FboConfig != 0;
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
