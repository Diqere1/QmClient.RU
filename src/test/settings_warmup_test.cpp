#include <game/client/components/settings_warmup.h>
#include <game/client/components/menus.h>
#include <game/client/components/settings_resource_jobs.h>

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

TEST(SettingsWarmup, PageRuntimeCacheTracksWhetherResourcesWereReadyAtRecord)
{
	SSettingsPageRuntimeCacheState Cache;
	EXPECT_TRUE(Cache.m_ResourcesReadyAtRecord);

	Cache.m_ResourcesReadyAtRecord = false;
	EXPECT_FALSE(Cache.m_ResourcesReadyAtRecord);
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

TEST(SettingsWarmup, LoadingRuntimeCacheWarmupIncludesRegisteredPagesAndTClientTabs)
{
	EXPECT_EQ(CMenus::SettingsRuntimeCacheWarmupSteps(), (int)BuildSettingsPageRuntimeRegistry().m_vPages.size() +
		6 + (CMenus::NUMBER_OF_QMCLIENT_SETTINGS_TABS - 1));
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

TEST(SettingsRuntimeCache, DefaultGpuBudgetAllowsOneSkinUploadBatch)
{
	SSettingsWarmupFrameBudget Budget;
	for(int Upload = 0; Upload < 14; ++Upload)
		EXPECT_TRUE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::GPU_UPLOAD));
	EXPECT_FALSE(SettingsWarmupConsumeBudget(Budget, ESettingsWarmupCost::GPU_UPLOAD));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
}

TEST(SettingsRuntimeCache, BudgetStopReasonsMapToProductionMissReasons)
{
	EXPECT_STREQ(SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason::TEXT_BUDGET), "text_budget");
	EXPECT_STREQ(SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason::FBO_BUDGET), "fbo_budget");
	EXPECT_STREQ(SettingsWarmupBudgetStopMissReasonName(ESettingsWarmupStopReason::NONE), "none");
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
	EXPECT_NE(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_ASSETS, ASSETS_TAB_ENTITIES), SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_ASSETS, ASSETS_TAB_ENTITY_BG));
}

TEST(SettingsRuntimeCache, TClientPerfStageNamesAreStable)
{
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::TAB_SHELL), "tclient_tab_shell");
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::SECTION_LAYOUT), "tclient_section_layout");
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::TEXT_CACHE), "tclient_text_cache");
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::RESOURCE_PRETRIGGER), "tclient_resource_pretrigger");
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::STATIC_LAYER), "tclient_static_layer");
	EXPECT_STREQ(SettingsTClientPerfStageName(ETClientSettingsPerfStage::INTERACTIVE_LAYER), "tclient_interactive_layer");
}

TEST(SettingsRuntimeCache, PerfReasonNamesAreStable)
{
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::NONE), "none");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::PAGE_FBO_UNSUPPORTED), "page_fbo_unsupported");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::PAGE_FBO_NOT_READY), "page_fbo_not_ready");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::SECTION_FBO_NOT_READY), "section_fbo_not_ready");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::RESOURCE_PLAN_PENDING), "resource_plan_pending");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::JOB_RESULT_PENDING), "job_result_pending");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::GPU_UPLOAD_BUDGET), "gpu_upload_budget");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::TEXT_BUDGET), "text_budget");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::ACTIVE_ITEM), "active_item");
	EXPECT_STREQ(SettingsWarmupMissReasonName(ESettingsWarmupMissReason::INVALID_RUNTIME_KEY), "invalid_runtime_key");
}

TEST(SettingsRuntimeCache, InvalidationReasonNamesAreStable)
{
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::LANGUAGE_CHANGED), "language_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::FONT_CHANGED), "font_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::BACKEND_CHANGED), "backend_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED), "window_or_scale_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::CONFIG_HASH_CHANGED), "config_hash_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::SECTION_SIZE_CHANGED), "section_size_changed");
	EXPECT_STREQ(SettingsInvalidationReasonName(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED), "resource_directory_changed");
}

TEST(SettingsRuntimeCache, CompactVisibleTextIsRejected)
{
	EXPECT_TRUE(SettingsRuntimeCacheAllowsVisibleCompactText("TClientPetSection"));
	EXPECT_FALSE(SettingsRuntimeCacheAllowsVisibleCompactText(nullptr));
	EXPECT_FALSE(SettingsRuntimeCacheAllowsVisibleCompactText("TClientDeferredSummary"));
	EXPECT_FALSE(SettingsRuntimeCacheAllowsVisibleCompactText("TClientCompactSummary"));
	EXPECT_FALSE(SettingsRuntimeCacheAllowsVisibleCompactText("TClientSummaryBlock"));
}

TEST(SettingsRuntimeCache, RuntimeKeyInvalidatesOnLanguageFontBackendWindowScaleAndConfig)
{
	SSettingsRuntimeCacheKey A;
	A.m_LanguageHash = 1;
	A.m_FontGeneration = 2;
	A.m_BackendGeneration = 3;
	A.m_WindowWidth = 1280;
	A.m_WindowHeight = 720;
	A.m_UiScale = 100;
	A.m_ConfigHash = 4;

	SSettingsRuntimeCacheKey B = A;
	EXPECT_TRUE(SettingsRuntimeCacheKeyMatches(A, B));

	B.m_LanguageHash = 9;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
	B = A;
	B.m_FontGeneration = 9;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
	B = A;
	B.m_BackendGeneration = 9;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
	B = A;
	B.m_WindowWidth = 1920;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
	B = A;
	B.m_UiScale = 125;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
	B = A;
	B.m_ConfigHash = 9;
	EXPECT_FALSE(SettingsRuntimeCacheKeyMatches(A, B));
}

TEST(SettingsRuntimeCache, ScrollWarmupOnlyUsesMatchingScrollPage)
{
	SSettingsRuntimeCacheMetadata Metadata;
	Metadata.m_LastPage = CMenus::SETTINGS_QMCLIENT;
	Metadata.m_LastQmTab = CMenus::QMCLIENT_SETTINGS_TAB_CONFIG;
	Metadata.m_LastScrollPage = CMenus::SETTINGS_TCLIENT;
	Metadata.m_LastScrollY = 128.0f;
	Metadata.m_Valid = true;

	const SSettingsPageRuntimeRegistry Registry = BuildSettingsPageRuntimeRegistry();
	const SSettingsWarmupStartupPlan Plan = BuildSettingsWarmupStartupPlan(Metadata, Registry);

	ASSERT_FALSE(Plan.m_vPageJobs.empty());
	EXPECT_EQ(Plan.m_vPageJobs.front().m_Page, CMenus::SETTINGS_QMCLIENT);
	EXPECT_FLOAT_EQ(Plan.m_vPageJobs.front().m_ScrollY, 0.0f);

	auto It = std::find_if(Plan.m_vPageJobs.begin(), Plan.m_vPageJobs.end(), [](const SSettingsWarmupPageJob &Job) {
		return Job.m_Page == CMenus::SETTINGS_TCLIENT;
	});
	ASSERT_NE(It, Plan.m_vPageJobs.end());
	EXPECT_FLOAT_EQ(It->m_ScrollY, 0.0f);

	Metadata.m_LastPage = CMenus::SETTINGS_TCLIENT;
	const SSettingsWarmupStartupPlan TClientPlan = BuildSettingsWarmupStartupPlan(Metadata, Registry);
	ASSERT_FALSE(TClientPlan.m_vPageJobs.empty());
	EXPECT_EQ(TClientPlan.m_vPageJobs.front().m_Page, CMenus::SETTINGS_TCLIENT);
	EXPECT_FLOAT_EQ(TClientPlan.m_vPageJobs.front().m_ScrollY, 128.0f);
}

TEST(SettingsRuntimeCache, StartupPlanCoversLastPageAndAllRegisteredPages)
{
	SSettingsRuntimeCacheMetadata Metadata;
	Metadata.m_LastPage = CMenus::SETTINGS_ASSETS;
	Metadata.m_LastTClientTab = 2;
	Metadata.m_LastQmTab = 1;
	Metadata.m_LastScrollPage = CMenus::SETTINGS_TCLIENT;
	Metadata.m_LastScrollY = 128.0f;

	const SSettingsWarmupStartupPlan Plan = BuildSettingsWarmupStartupPlan(Metadata, BuildSettingsPageRuntimeRegistry());

	ASSERT_FALSE(Plan.m_vPageJobs.empty());
	EXPECT_EQ(Plan.m_vPageJobs[0].m_Page, CMenus::SETTINGS_ASSETS);
	for(int Page : BuildSettingsPageRuntimeRegistry().m_vPages)
		EXPECT_TRUE(SettingsWarmupPlanContainsPage(Plan, Page));
}

TEST(SettingsRuntimeCache, PageCacheSlotsRejectInvalidPersistedTabs)
{
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_TCLIENT, -1), 10);
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_TCLIENT, 99), -1);
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_QMCLIENT, -1), 16);
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_QMCLIENT, CMenus::NUMBER_OF_QMCLIENT_SETTINGS_TABS), -1);
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_ASSETS, -1), 9);
	EXPECT_EQ(SettingsPageRuntimeCacheSlot(CMenus::SETTINGS_ASSETS, NUMBER_OF_ASSETS_TABS), -1);
}

TEST(SettingsRuntimeCache, SectionRegistryCoversComplexPages)
{
	SSettingsSectionRegistry Registry = BuildSettingsSectionRegistry();

	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_TCLIENT, "binds"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_QMCLIENT, "general"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_CONTROLS, "movement"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_ASSETS, "resource-list"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_LANGUAGE, "language-list"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_PLAYER, "skin-list"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_GENERAL, "body"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_APPEARANCE, "body"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_GRAPHICS, "body"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_SOUND, "body"));
	EXPECT_TRUE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_DDNET, "body"));
	EXPECT_FALSE(SettingsSectionRegistryContains(Registry, CMenus::SETTINGS_GENERAL, "missing"));
}

TEST(SettingsRuntimeCache, TextCacheKeysAreStableAcrossFrames)
{
	EXPECT_EQ(SettingsTextCacheKey(CMenus::SETTINGS_TCLIENT, 1, "auto-reply-title"), SettingsTextCacheKey(CMenus::SETTINGS_TCLIENT, 1, "auto-reply-title"));
	EXPECT_NE(SettingsTextCacheKey(CMenus::SETTINGS_TCLIENT, 1, "auto-reply-title"), SettingsTextCacheKey(CMenus::SETTINGS_TCLIENT, 2, "auto-reply-title"));
}

TEST(SettingsRuntimeCache, SectionRegistryRequiresBothLayersForStaticFbo)
{
	const SSettingsSectionRegistry Registry = BuildSettingsSectionRegistry();
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 3, "binds"));
	EXPECT_TRUE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 0, "auto-reply"));
	EXPECT_TRUE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 0, "pet"));
	EXPECT_TRUE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 0, "theme"));
	EXPECT_TRUE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 0, "misc"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 1, "auto-reply"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_VISUAL, "general"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_CONFIG, "config"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_QMCLIENT, CMenus::QMCLIENT_SETTINGS_TAB_CONTRIBUTORS, "contributors"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_CONTROLS, -1, "movement"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_CONTROLS, -1, "weapons"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_CONTROLS, -1, "voting"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_ASSETS, -1, "resource-list"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_ASSETS, -1, "preview"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_LANGUAGE, -1, "language-list"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_LANGUAGE, -1, "credits"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_PLAYER, -1, "skin-list"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_PLAYER, -1, "identity"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TEE, -1, "skin-list"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TEE, -1, "identity"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_GENERAL, -1, "body"));
	EXPECT_FALSE(SettingsSectionCanRecordStaticFbo(Registry, CMenus::SETTINGS_TCLIENT, 0, "missing"));
}

TEST(SettingsRuntimeCache, InvalidationReasonTargetsExpectedCaches)
{
	EXPECT_TRUE(SettingsInvalidationClearsTextPool(ESettingsInvalidationReason::LANGUAGE_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsSectionFbo(ESettingsInvalidationReason::LANGUAGE_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason::LANGUAGE_CHANGED));

	EXPECT_FALSE(SettingsInvalidationClearsTextPool(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsResourcePlan(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsSectionFbo(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED));
	EXPECT_FALSE(SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED, CMenus::SETTINGS_ASSETS, CMenus::SETTINGS_ASSETS));
	EXPECT_FALSE(SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason::RESOURCE_DIRECTORY_CHANGED, CMenus::SETTINGS_TCLIENT, CMenus::SETTINGS_ASSETS));

	EXPECT_TRUE(SettingsInvalidationClearsTextPool(ESettingsInvalidationReason::FONT_CHANGED));
	EXPECT_FALSE(SettingsInvalidationClearsResourcePlan(ESettingsInvalidationReason::FONT_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsPageFbo(ESettingsInvalidationReason::WINDOW_OR_SCALE_CHANGED));
	EXPECT_FALSE(SettingsInvalidationClearsTextPool(ESettingsInvalidationReason::SECTION_SIZE_CHANGED));
	EXPECT_TRUE(SettingsInvalidationClearsSectionFbo(ESettingsInvalidationReason::SECTION_SIZE_CHANGED));
}

TEST(SettingsRuntimeCache, DisabledConfigBypassesWarmupAndFbo)
{
	EXPECT_FALSE(SettingsWarmupEnabled(0, 1));
	EXPECT_FALSE(SettingsWarmupEnabled(1, 0));
	EXPECT_FALSE(SettingsWarmupEnabled(0, 0));
	EXPECT_TRUE(SettingsWarmupEnabled(1, 1));
}

TEST(SettingsResourceJobs, SkinPlanKeepsSelectedFavoritesThenSorted)
{
	std::vector<SSettingsSkinListEntry> vEntries = {
		{"zeta", false, false},
		{"alpha", false, true},
		{"selected", true, false},
	};
	const SSettingsSkinListPlan Plan = BuildSettingsSkinListPlan(vEntries);

	ASSERT_EQ(Plan.m_vNames.size(), 3u);
	EXPECT_EQ(Plan.m_vNames[0], "selected");
	EXPECT_EQ(Plan.m_vNames[1], "alpha");
	EXPECT_EQ(Plan.m_vNames[2], "zeta");
}

TEST(SettingsResourceJobs, CountryFlagPlanDeduplicatesAndKeepsOrder)
{
	const std::vector<int> vPlan = BuildSettingsCountryFlagWarmupPlan({156, 840, 156, -1});
	ASSERT_EQ(vPlan.size(), 3u);
	EXPECT_EQ(vPlan[0], 156);
	EXPECT_EQ(vPlan[1], 840);
	EXPECT_EQ(vPlan[2], -1);
}

TEST(SettingsResourceJobs, ResourceMergeBudgetStopsBatchWork)
{
	SSettingsResourceMergeBudget Budget;
	Budget.m_MaxListEntries = 2;
	Budget.m_MaxGpuUploads = 1;

	EXPECT_TRUE(SettingsResourceConsumeMergeEntry(Budget));
	EXPECT_TRUE(SettingsResourceConsumeMergeEntry(Budget));
	EXPECT_FALSE(SettingsResourceConsumeMergeEntry(Budget));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::MERGE_BUDGET);

	Budget.m_StopReason = ESettingsWarmupStopReason::NONE;
	EXPECT_TRUE(SettingsResourceConsumeGpuUpload(Budget));
	EXPECT_FALSE(SettingsResourceConsumeGpuUpload(Budget));
	EXPECT_EQ(Budget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
}

TEST(SettingsResourceJobs, ResourceMergeBudgetHonorsUnifiedFrameBudget)
{
	SSettingsResourceMergeBudget ResourceBudget;
	ResourceBudget.m_MaxListEntries = 8;
	SSettingsWarmupFrameBudget FrameBudget;
	FrameBudget.m_MaxJobResultMerges = 1;

	EXPECT_TRUE(SettingsResourceConsumeMergeEntry(ResourceBudget, &FrameBudget));
	EXPECT_TRUE(SettingsResourceConsumeMergeEntry(ResourceBudget, &FrameBudget));
	EXPECT_EQ(ResourceBudget.m_MaxListEntries, 6);
	EXPECT_EQ(ResourceBudget.m_StopReason, ESettingsWarmupStopReason::NONE);

	SSettingsResourceMergeBudget NextBatchBudget;
	NextBatchBudget.m_MaxListEntries = 8;
	EXPECT_FALSE(SettingsResourceConsumeMergeEntry(NextBatchBudget, &FrameBudget));
	EXPECT_EQ(NextBatchBudget.m_MaxListEntries, 8);
	EXPECT_EQ(NextBatchBudget.m_StopReason, ESettingsWarmupStopReason::MERGE_BUDGET);
	EXPECT_EQ(FrameBudget.m_StopReason, ESettingsWarmupStopReason::MERGE_BUDGET);
}

TEST(SettingsResourceJobs, ResourceGpuUploadBudgetHonorsUnifiedFrameBudget)
{
	SSettingsResourceMergeBudget ResourceBudget;
	ResourceBudget.m_MaxGpuUploads = 8;
	SSettingsWarmupFrameBudget FrameBudget;
	FrameBudget.m_MaxGpuUploads = 1;

	EXPECT_TRUE(SettingsResourceConsumeGpuUpload(ResourceBudget, &FrameBudget));
	EXPECT_FALSE(SettingsResourceConsumeGpuUpload(ResourceBudget, &FrameBudget));
	EXPECT_EQ(ResourceBudget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
	EXPECT_EQ(FrameBudget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
	EXPECT_EQ(ResourceBudget.m_MaxGpuUploads, 7);
}

TEST(SettingsResourceJobs, ResourceGpuUploadBudgetCanReserveMultipleUploads)
{
	SSettingsResourceMergeBudget ResourceBudget;
	ResourceBudget.m_MaxGpuUploads = 8;
	SSettingsWarmupFrameBudget FrameBudget;
	FrameBudget.m_MaxGpuUploads = 3;

	EXPECT_TRUE(SettingsResourceConsumeGpuUploads(ResourceBudget, &FrameBudget, 3));
	EXPECT_EQ(ResourceBudget.m_MaxGpuUploads, 5);
	EXPECT_FALSE(SettingsResourceConsumeGpuUploads(ResourceBudget, &FrameBudget, 1));
	EXPECT_EQ(ResourceBudget.m_MaxGpuUploads, 5);
	EXPECT_EQ(FrameBudget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
}

TEST(SettingsResourceJobs, DefaultSettingsGpuBudgetAllowsOneSkinUploadBatch)
{
	SSettingsResourceMergeBudget ResourceBudget;
	ResourceBudget.m_MaxGpuUploads = 42;
	SSettingsWarmupFrameBudget FrameBudget;

	EXPECT_TRUE(SettingsResourceConsumeGpuUploads(ResourceBudget, &FrameBudget, 14));
	EXPECT_FALSE(SettingsResourceConsumeGpuUploads(ResourceBudget, &FrameBudget, 14));
	EXPECT_EQ(ResourceBudget.m_MaxGpuUploads, 28);
	EXPECT_EQ(FrameBudget.m_StopReason, ESettingsWarmupStopReason::GPU_UPLOAD_BUDGET);
}

TEST(SettingsResourceJobs, AssetListLoadingDoesNotBlockVisibleEntries)
{
	EXPECT_TRUE(SettingsAssetListShouldShowBlockingLoading(true, 0));
	EXPECT_FALSE(SettingsAssetListShouldShowBlockingLoading(true, 1));
	EXPECT_FALSE(SettingsAssetListShouldShowBlockingLoading(false, 0));
}

TEST(SettingsResourceJobs, AssetPreviewDecodeCanStartWhileMerging)
{
	EXPECT_FALSE(SettingsAssetListCanStartPreviewDecode(true, false, false));
	EXPECT_FALSE(SettingsAssetListCanStartPreviewDecode(false, true, false));
	EXPECT_TRUE(SettingsAssetListCanStartPreviewDecode(false, false, true));
	EXPECT_FALSE(SettingsAssetListCanStartPreviewDecode(false, false, false));
}

TEST(SettingsResourceJobs, AssetPreviewFinalizeBudgetDefersAfterLimit)
{
	EXPECT_FALSE(SettingsAssetPreviewShouldDeferFinalize(0, 10.0, 2, 4.0));
	EXPECT_FALSE(SettingsAssetPreviewShouldDeferFinalize(1, 3.5, 2, 4.0));
	EXPECT_TRUE(SettingsAssetPreviewShouldDeferFinalize(1, 4.0, 2, 4.0));
	EXPECT_TRUE(SettingsAssetPreviewShouldDeferFinalize(2, 0.0, 2, 4.0));
}

TEST(SettingsResourceJobs, AssetPreviewPrioritizesCurrentVisibleRange)
{
	EXPECT_FALSE(SettingsAssetPreviewShouldPrioritizeVisibleRange(9, 10, 20));
	EXPECT_TRUE(SettingsAssetPreviewShouldPrioritizeVisibleRange(10, 10, 20));
	EXPECT_TRUE(SettingsAssetPreviewShouldPrioritizeVisibleRange(15, 10, 20));
	EXPECT_TRUE(SettingsAssetPreviewShouldPrioritizeVisibleRange(20, 10, 20));
	EXPECT_FALSE(SettingsAssetPreviewShouldPrioritizeVisibleRange(21, 10, 20));
	EXPECT_FALSE(SettingsAssetPreviewShouldPrioritizeVisibleRange(10, -1, 20));
}

TEST(SettingsResourceJobs, WorkshopThumbDecodePrioritizesVisibleDownloadableItems)
{
	EXPECT_TRUE(SettingsWorkshopThumbShouldStartHighPriority(0, 0, 3));
	EXPECT_TRUE(SettingsWorkshopThumbShouldStartHighPriority(3, 0, 3));
	EXPECT_FALSE(SettingsWorkshopThumbShouldStartHighPriority(4, 0, 3));
	EXPECT_FALSE(SettingsWorkshopThumbShouldStartHighPriority(0, -1, 3));
}

TEST(SettingsResourceJobs, PageCacheRejectsRecordedFrameWithoutReadyResources)
{
	EXPECT_FALSE(SettingsPageCacheCanUseRecordedResources(true, true, false));
	EXPECT_FALSE(SettingsPageCacheCanUseRecordedResources(true, false, true));
	EXPECT_FALSE(SettingsPageCacheCanUseRecordedResources(false, true, true));
	EXPECT_TRUE(SettingsPageCacheCanUseRecordedResources(true, true, true));
}

TEST(SettingsResourceJobs, AssetsPageRejectsWholePageFbo)
{
	EXPECT_FALSE(SettingsPageCanUsePageFbo(CMenus::SETTINGS_ASSETS, CMenus::SETTINGS_ASSETS));
	EXPECT_TRUE(SettingsPageCanUsePageFbo(CMenus::SETTINGS_TCLIENT, CMenus::SETTINGS_ASSETS));
	EXPECT_TRUE(SettingsPageCanUsePageFbo(CMenus::SETTINGS_LANGUAGE, CMenus::SETTINGS_ASSETS));
}

TEST(SettingsResourceJobs, AssetWarmupTracksAllTabsAndCycles)
{
	bool aReadyTabs[] = {true, false, true};
	EXPECT_FALSE(SettingsAssetWarmupAllTabsReady(aReadyTabs, 3));
	aReadyTabs[1] = true;
	EXPECT_TRUE(SettingsAssetWarmupAllTabsReady(aReadyTabs, 3));
	EXPECT_TRUE(SettingsAssetWarmupAllTabsReady(nullptr, 0));

	EXPECT_EQ(SettingsAssetWarmupNextTab(-1, 3), 0);
	EXPECT_EQ(SettingsAssetWarmupNextTab(0, 3), 1);
	EXPECT_EQ(SettingsAssetWarmupNextTab(2, 3), 0);
	EXPECT_EQ(SettingsAssetWarmupNextTab(0, 0), -1);
}

TEST(SettingsResourceJobs, SkinSnapshotRejectsStaleGeneration)
{
	SSettingsSkinListPlanResult Result;
	Result.m_Generation = 7;
	EXPECT_TRUE(SettingsSkinListPlanGenerationMatches(Result, 7));
	EXPECT_FALSE(SettingsSkinListPlanGenerationMatches(Result, 8));
}

TEST(SettingsResourceJobs, AssetListRejectsStaleJobGeneration)
{
	EXPECT_TRUE(SettingsAssetListJobGenerationMatches(4, 4));
	EXPECT_FALSE(SettingsAssetListJobGenerationMatches(4, 5));
}

TEST(SettingsResourceJobs, SkinListPublishesOnlyCompleteMergedList)
{
	EXPECT_FALSE(SettingsSkinListShouldPublishMergedList(0, 3));
	EXPECT_FALSE(SettingsSkinListShouldPublishMergedList(2, 3));
	EXPECT_TRUE(SettingsSkinListShouldPublishMergedList(3, 3));
	EXPECT_TRUE(SettingsSkinListShouldPublishMergedList(4, 3));
}

TEST(SettingsResourceJobs, VisibleSkinListEntriesRequestImmediateLoad)
{
	EXPECT_TRUE(SettingsSkinListShouldRequestImmediateLoad(true));
	EXPECT_FALSE(SettingsSkinListShouldRequestImmediateLoad(false));
}

TEST(SettingsResourceJobs, CountryFlagPlanHandlesEmptyInput)
{
	const std::vector<int> vPlan = BuildSettingsCountryFlagWarmupPlan({});
	EXPECT_TRUE(vPlan.empty());
}
