#include <game/client/components/settings_warmup.h>
#include <game/client/components/menus.h>

#include <gtest/gtest.h>

TEST(SettingsWarmup, StageReadinessUsesInclusiveThreshold)
{
	EXPECT_FALSE(IsSettingsWarmupStageReady(0, 1));
	EXPECT_TRUE(IsSettingsWarmupStageReady(1, 1));
	EXPECT_TRUE(IsSettingsWarmupStageReady(-1, 5));
}

TEST(SettingsWarmup, StageAdvanceStopsAfterLastStage)
{
	EXPECT_EQ(AdvanceSettingsWarmupStage(0, 3), 1);
	EXPECT_EQ(AdvanceSettingsWarmupStage(2, 3), 3);
	EXPECT_EQ(AdvanceSettingsWarmupStage(3, 3), -1);
	EXPECT_EQ(AdvanceSettingsWarmupStage(-1, 3), -1);
	EXPECT_EQ(AdvanceSettingsWarmupStage(0, -1), -1);
}

TEST(SettingsWarmup, PrioritizesLastSessionPage)
{
	CSettingsWarmupScheduler Scheduler;
	int WarmedControls = 0;
	int WarmedTClient = 0;

	Scheduler.RegisterSection({EClassicSettingsPage::CONTROLS, "Controls:Mouse", 0, [&]() {
			++WarmedControls;
			return 1.0;
		}});
	Scheduler.RegisterSection({EClassicSettingsPage::TCLIENT, "TClient:Pet", 0, [&]() {
			++WarmedTClient;
			return 1.0;
		}});
	Scheduler.SetLastSessionPage(EClassicSettingsPage::TCLIENT);

	EXPECT_FALSE(Scheduler.WarmupFrame(1.5));
	EXPECT_EQ(WarmedControls, 0);
	EXPECT_EQ(WarmedTClient, 1);
}

TEST(SettingsWarmup, StopsAtFrameBudget)
{
	CSettingsWarmupScheduler Scheduler;
	int WarmedCount = 0;

	Scheduler.RegisterSection({EClassicSettingsPage::CONTROLS, "Controls:Mouse", 0, [&]() {
			++WarmedCount;
			return 2.0;
		}});
	Scheduler.RegisterSection({EClassicSettingsPage::CONTROLS, "Controls:Movement", 1, [&]() {
			++WarmedCount;
			return 2.0;
		}});

	EXPECT_FALSE(Scheduler.WarmupFrame(2.5));
	EXPECT_EQ(WarmedCount, 1);
	EXPECT_TRUE(Scheduler.WarmupFrame(2.5));
	EXPECT_EQ(WarmedCount, 2);
}

TEST(SettingsWarmup, DisabledSchedulerDoesNotRunSections)
{
	CSettingsWarmupScheduler Scheduler;
	int WarmedCount = 0;

	Scheduler.RegisterSection({EClassicSettingsPage::CONTROLS, "Controls:Mouse", 0, [&]() {
			++WarmedCount;
			return 1.0;
		}});
	Scheduler.SetEnabled(false);

	EXPECT_TRUE(Scheduler.WarmupFrame(10.0));
	EXPECT_EQ(WarmedCount, 0);
}

TEST(SettingsWarmup, ManySettingsSectionsRespectBudget)
{
	CSettingsWarmupScheduler Scheduler;
	int WarmedCount = 0;

	for(int i = 0; i < 20; ++i)
	{
		Scheduler.RegisterSection({EClassicSettingsPage::CONTROLS, "Controls:Bulk", i, [&]() {
			++WarmedCount;
			return 1.0;
		}});
	}
	Scheduler.SetLastSessionPage(EClassicSettingsPage::CONTROLS);

	EXPECT_FALSE(Scheduler.WarmupFrame(3.0));
	EXPECT_EQ(WarmedCount, 3);
}

TEST(SettingsWarmup, SectionCacheMetadataRequiresMatchingRuntimeKey)
{
	SSettingsSectionCacheRuntimeKey RuntimeKey;
	RuntimeKey.m_ViewportWidth = 900;
	RuntimeKey.m_ViewportHeight = 620;
	RuntimeKey.m_UiScale = 100;
	RuntimeKey.m_ConfigHash = 1234;
	RuntimeKey.m_LanguageHash = 5678;
	RuntimeKey.m_FontHash = 9012;
	RuntimeKey.m_BackendHash = 3456;
	RuntimeKey.m_WindowHash = 7890;

	SSettingsSectionCacheMetadata Metadata;
	Metadata.m_LastPage = EClassicSettingsPage::TCLIENT;
	Metadata.m_LastTab = 0;
	Metadata.m_LastScrollY = 140.0f;
	Metadata.m_SectionNameHash = 42;
	Metadata.m_SectionHeight = 180.0f;
	Metadata.m_RuntimeKey = RuntimeKey;

	EXPECT_TRUE(Metadata.Matches(RuntimeKey));

	RuntimeKey.m_ViewportHeight += 1;
	EXPECT_FALSE(Metadata.Matches(RuntimeKey));
}

TEST(SettingsWarmup, PageRuntimeCacheShortCircuitsOnlyFirstMatchingDraw)
{
	SSettingsSectionCacheRuntimeKey RuntimeKey;
	RuntimeKey.m_ViewportWidth = 900;
	RuntimeKey.m_ViewportHeight = 620;
	RuntimeKey.m_UiScale = 100;
	RuntimeKey.m_ConfigHash = 1234;

	SSettingsPageRuntimeCacheState Cache;
	Cache.m_Page = 8;
	Cache.m_Tab = 2;
	Cache.m_Width = 900;
	Cache.m_Height = 620;
	Cache.m_RuntimeKey = RuntimeKey;
	Cache.m_Valid = true;

	EXPECT_TRUE(SettingsPageRuntimeCacheShouldShortCircuit(Cache, 8, 2, 900, 620, RuntimeKey));
	EXPECT_TRUE(Cache.m_DrawnOnce);
	EXPECT_FALSE(SettingsPageRuntimeCacheShouldShortCircuit(Cache, 8, 2, 900, 620, RuntimeKey));
}

TEST(SettingsWarmup, PageRuntimeCacheRejectsMismatchedRuntimeKey)
{
	SSettingsSectionCacheRuntimeKey RuntimeKey;
	RuntimeKey.m_ViewportWidth = 900;
	RuntimeKey.m_ViewportHeight = 620;
	RuntimeKey.m_ConfigHash = 1234;

	SSettingsPageRuntimeCacheState Cache;
	Cache.m_Page = 9;
	Cache.m_Tab = 1;
	Cache.m_Width = 900;
	Cache.m_Height = 620;
	Cache.m_RuntimeKey = RuntimeKey;
	Cache.m_Valid = true;

	RuntimeKey.m_ConfigHash = 5678;
	EXPECT_FALSE(SettingsPageRuntimeCacheShouldShortCircuit(Cache, 9, 1, 900, 620, RuntimeKey));
	EXPECT_FALSE(Cache.m_DrawnOnce);
}

TEST(SettingsWarmup, RuntimeFboWarmupRunsBeforeTextOnlyFallback)
{
	CSettingsWarmupScheduler Scheduler;
	std::vector<const char *> vOrder;

	Scheduler.RegisterSection({EClassicSettingsPage::TCLIENT, "TClient:Title", 1, [&]() {
			vOrder.push_back("text");
			return 0.1;
		}});
	Scheduler.RegisterSection({EClassicSettingsPage::TCLIENT, "TClient:FboFirstScreen", 0, [&]() {
			vOrder.push_back("fbo");
			return 1.0;
		}, false, ESettingsWarmupKind::RUNTIME_FBO});
	Scheduler.SetLastSessionPage(EClassicSettingsPage::TCLIENT);

	EXPECT_FALSE(Scheduler.WarmupFrame(1.1));
	ASSERT_EQ(vOrder.size(), 1);
	EXPECT_STREQ(vOrder[0], "fbo");
}

TEST(SettingsWarmup, LoadingRuntimeCacheWarmupIncludesCorePagesAndTClientTabs)
{
	EXPECT_EQ(SettingsLoadingRuntimeCacheWarmupSteps(6), 12);
	EXPECT_EQ(SettingsLoadingRuntimeCacheWarmupSteps(0), 6);
}

TEST(SettingsRuntimeCache, RegistersAllSettingsPages)
{
	const SSettingsPageRuntimeRegistry Registry = BuildSettingsPageRuntimeRegistry();

	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_LANGUAGE));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_PLAYER));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_TEE));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_GENERAL));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_CONTROLS));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_GRAPHICS));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_SOUND));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_DDNET));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_TCLIENT));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_QMCLIENT));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_APPEARANCE));
	EXPECT_TRUE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_ASSETS));
	EXPECT_FALSE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_PROFILES));
	EXPECT_FALSE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_CONFIGS));
	EXPECT_FALSE(SettingsPageRuntimeRegistryContains(Registry, CMenus::SETTINGS_CONTRIBUTORS));
}

TEST(SettingsRuntimeCache, BuildsStableKeysForPageSectionTextAndResource)
{
	EXPECT_EQ(SettingsPageCacheKey(CMenus::SETTINGS_TCLIENT, 2), "settings:tclient:tab:2");
	EXPECT_EQ(SettingsSectionCacheKey(CMenus::SETTINGS_TCLIENT, 2, "binds"), "settings:tclient:tab:2:section:binds");
	EXPECT_EQ(SettingsTextCacheKey(CMenus::SETTINGS_LANGUAGE, -1, "credits"), "settings:language:text:credits");
	EXPECT_EQ(SettingsResourceCacheKey(CMenus::SETTINGS_ASSETS, "entity_bg"), "settings:assets:resource:entity_bg");
}

TEST(SettingsRuntimeCache, BudgetStopsEveryMainThreadCost)
{
	SSettingsWarmupFrameBudget Budget;
	Budget.m_MaxTextContainers = 1;
	Budget.m_MaxRenderTargetRecords = 1;
	Budget.m_MaxGpuUploads = 1;
	Budget.m_MaxJobResultMerges = 1;

	EXPECT_TRUE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::TEXT_CONTAINER));
	EXPECT_FALSE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::TEXT_CONTAINER));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::TEXT_BUDGET);

	Budget.m_StopReason = ESettingsWarmupStopReason::NONE;
	EXPECT_TRUE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::RENDER_TARGET_RECORD));
	EXPECT_FALSE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::RENDER_TARGET_RECORD));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::FBO_BUDGET);

	Budget.m_StopReason = ESettingsWarmupStopReason::NONE;
	EXPECT_TRUE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::GPU_UPLOAD));
	EXPECT_FALSE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::GPU_UPLOAD));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);

	Budget.m_StopReason = ESettingsWarmupStopReason::NONE;
	EXPECT_TRUE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::JOB_RESULT_MERGE));
	EXPECT_FALSE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::JOB_RESULT_MERGE));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::MERGE_BUDGET);
}

TEST(SettingsRuntimeCache, PageCacheSlotsExistForEveryRegisteredPage)
{
	const SSettingsPageRuntimeRegistry Registry = BuildSettingsPageRuntimeRegistry();
	for(int Page : Registry.m_vPages)
	{
		const int Tab = Page == CMenus::SETTINGS_QMCLIENT ? 0 : -1;
		EXPECT_GE(SettingsPageRuntimeCacheSlot(Page, Tab), 0) << Page;
	}
	EXPECT_NE(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_TCLIENT, 0), SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_TCLIENT, 1));
}
