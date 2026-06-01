#include <game/client/components/settings_skin_preview_cache.h>

#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <sstream>

TEST(SettingsSkinPreviewCache, PathUsesSanitizedNameVersionAndHash)
{
	const auto Path = BuildSettingsSkinPreviewCachePath("beast skin", 1, "7f3c1d8a2b44");
	EXPECT_EQ(Path, "qmclient/skins/preview_cache/beast_skin--v1--7f3c1d8a2b44.webp");
}

TEST(SettingsSkinPreviewCache, KeyIgnoresBodyAndFeetColor)
{
	SSettingsSkinPreviewCacheKey A{"beast", 1, 64, "hash-a", 0};
	SSettingsSkinPreviewCacheKey B{"beast", 1, 64, "hash-a", 0};
	EXPECT_EQ(A, B);
}

TEST(SettingsSkinPreviewCache, DifferentVersionChangesPath)
{
	EXPECT_NE(
		BuildSettingsSkinPreviewCachePath("beast", 1, "hash"),
		BuildSettingsSkinPreviewCachePath("beast", 2, "hash"));
}

TEST(SettingsSkinPreviewCache, VersionTracksHighQualityCacheRevision)
{
	EXPECT_EQ(SETTINGS_SKIN_PREVIEW_CACHE_VERSION, 3);
}

TEST(SettingsSkinPreviewCache, PruneRemovesOldestOverflowEntries)
{
	std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries = {
		{"a.webp", 10},
		{"b.webp", 20},
		{"c.webp", 30},
	};
	const auto vDelete = ComputeSettingsSkinPreviewCachePruneList(vEntries, 2);
	ASSERT_EQ(vDelete.size(), 1u);
	EXPECT_EQ(vDelete[0], "a.webp");
}

TEST(SettingsSkinPreviewCache, PruneRemovesLayerGroupsTogether)
{
	std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries = {
		{"qmclient/skins/preview_cache/old--v1--hash--s50--e0--body-outline-original.webp", 10},
		{"qmclient/skins/preview_cache/old--v1--hash--s50--e0--body-original.webp", 11},
		{"qmclient/skins/preview_cache/new--v1--hash--s50--e0--body-outline-original.webp", 20},
		{"qmclient/skins/preview_cache/new--v1--hash--s50--e0--body-original.webp", 21},
	};
	const auto vDelete = ComputeSettingsSkinPreviewCachePruneList(vEntries, 1);
	ASSERT_EQ(vDelete.size(), 2u);
	EXPECT_EQ(vDelete[0], "qmclient/skins/preview_cache/old--v1--hash--s50--e0--body-outline-original.webp");
	EXPECT_EQ(vDelete[1], "qmclient/skins/preview_cache/old--v1--hash--s50--e0--body-original.webp");
}

TEST(SettingsSkinPreviewCache, SameSkinAndHashReuseCacheAcrossColors)
{
	const auto PathA = BuildSettingsSkinPreviewCachePath("beast", SETTINGS_SKIN_PREVIEW_CACHE_VERSION, "abc123");
	const auto PathB = BuildSettingsSkinPreviewCachePath("beast", SETTINGS_SKIN_PREVIEW_CACHE_VERSION, "abc123");
	EXPECT_EQ(PathA, PathB);
}

TEST(SettingsSkinPreviewCache, KeyPathIncludesSizeBucket)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	EXPECT_EQ(BuildSettingsSkinPreviewCachePath(Key), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f0.webp");
}

TEST(SettingsSkinPreviewCache, LookupDoesNotRequireLoadedSourceOnceHashExists)
{
	EXPECT_TRUE(SettingsSkinPreviewCacheAllowsLookup(true, false, false));
	EXPECT_FALSE(SettingsSkinPreviewCacheAllowsLookup(false, false, false));
	EXPECT_FALSE(SettingsSkinPreviewCacheAllowsLookup(true, true, false));
	EXPECT_FALSE(SettingsSkinPreviewCacheAllowsLookup(true, false, true));
}

TEST(SettingsSkinPreviewCache, CachedPreviewOnlySuppressesTransientLoadingStatus)
{
	EXPECT_TRUE(SettingsSkinPreviewSuppressStatusIcon(true, false, false));
	EXPECT_TRUE(SettingsSkinPreviewSuppressStatusIcon(false, true, true));
	EXPECT_FALSE(SettingsSkinPreviewSuppressStatusIcon(false, true, false));
	EXPECT_FALSE(SettingsSkinPreviewSuppressStatusIcon(false, false, true));

	EXPECT_FALSE(SettingsSkinPreviewShowProgress(true, true));
	EXPECT_TRUE(SettingsSkinPreviewShowProgress(false, true));
	EXPECT_FALSE(SettingsSkinPreviewShowProgress(true, false));
	EXPECT_FALSE(SettingsSkinPreviewShowProgress(false, false));
}

TEST(SettingsSkinPreviewCache, ClearMemoryCacheDropsRememberedEntries)
{
	CSettingsSkinPreviewCache Cache;
	SSettingsSkinPreviewCacheTextures Textures;
	const SSettingsSkinPreviewCacheKey KeyA{"beast", 3, 64, "hash-a", 0, 0};
	const SSettingsSkinPreviewCacheKey KeyB{"beast", 3, 72, "hash-b", 0, 0};

	Cache.RememberTextures(KeyA, Textures);
	Cache.RememberTextures(KeyB, Textures);
	EXPECT_EQ(Cache.MemoryCacheSize(), 2u);

	Cache.ClearMemoryCache();
	EXPECT_EQ(Cache.MemoryCacheSize(), 0u);
}

TEST(SettingsSkinPreviewCache, MemoryEvictionDropsLeastRecentlyUsedEntriesFirst)
{
	std::vector<SSettingsSkinPreviewMemoryCacheEntry> vEntries = {
		{40, 1},
		{30, 3},
		{20, 2},
	};
	const auto vEvict = ComputeSettingsSkinPreviewCacheMemoryEvictionList(vEntries, 2, 128);
	ASSERT_EQ(vEvict.size(), 1u);
	EXPECT_EQ(vEvict[0], 0);
}

TEST(SettingsSkinPreviewCache, MemoryEvictionAlsoHonorsByteBudget)
{
	std::vector<SSettingsSkinPreviewMemoryCacheEntry> vEntries = {
		{70, 5},
		{20, 1},
		{20, 2},
	};
	const auto vEvict = ComputeSettingsSkinPreviewCacheMemoryEvictionList(vEntries, 4, 80);
	ASSERT_EQ(vEvict.size(), 2u);
	EXPECT_EQ(vEvict[0], 1);
	EXPECT_EQ(vEvict[1], 2);
}

TEST(SettingsSkinPreviewCache, GenerationStillRequiresLoadedSource)
{
	EXPECT_TRUE(SettingsSkinPreviewCacheAllowsGeneration(true, true, false, false));
	EXPECT_FALSE(SettingsSkinPreviewCacheAllowsGeneration(false, true, false, false));
}

TEST(SettingsSkinPreviewCache, DifferentSizeBucketChangesKeyPath)
{
	const SSettingsSkinPreviewCacheKey Small{"beast", 1, 48, "hash", 0};
	const SSettingsSkinPreviewCacheKey Large{"beast", 1, 64, "hash", 0};
	EXPECT_NE(BuildSettingsSkinPreviewCachePath(Small), BuildSettingsSkinPreviewCachePath(Large));
}

TEST(SettingsSkinPreviewCache, DifferentEmoteChangesKeyPath)
{
	const SSettingsSkinPreviewCacheKey Normal{"beast", 1, 64, "hash", 0};
	const SSettingsSkinPreviewCacheKey Angry{"beast", 1, 64, "hash", 4};
	EXPECT_NE(BuildSettingsSkinPreviewCachePath(Normal), BuildSettingsSkinPreviewCachePath(Angry));
}

TEST(SettingsSkinPreviewCache, DifferentFatSkinModeChangesKeyPath)
{
	const SSettingsSkinPreviewCacheKey Normal{"beast", 1, 64, "hash", 0, 0};
	const SSettingsSkinPreviewCacheKey Fat{"beast", 1, 64, "hash", 0, 1};
	EXPECT_NE(BuildSettingsSkinPreviewCachePath(Normal), BuildSettingsSkinPreviewCachePath(Fat));
}

TEST(SettingsSkinPreviewCache, ManifestPathUsesKeyPath)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0, 1};
	EXPECT_EQ(BuildSettingsSkinPreviewCacheManifestPath(Key), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f1.manifest");
}

TEST(SettingsSkinPreviewCache, LayerPathIncludesDrawOrderLayerName)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	EXPECT_EQ((BuildSettingsSkinPreviewCachePath(Key, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_COLORABLE)), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f0--body-outline-colorable.webp");
	EXPECT_EQ((BuildSettingsSkinPreviewCachePath(Key, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_COLORABLE)), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f0--body-colorable.webp");
	EXPECT_EQ((BuildSettingsSkinPreviewCachePath(Key, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_COLORABLE)), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f0--back-feet-colorable.webp");
	EXPECT_EQ((BuildSettingsSkinPreviewCachePath(Key, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_COLORABLE)), "qmclient/skins/preview_cache/beast--v1--hash--s64--e0--f0--front-feet-colorable.webp");
}

TEST(SettingsSkinPreviewCache, LayerNamesAreStable)
{
	EXPECT_STREQ(SettingsSkinPreviewCacheLayerName(SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_OUTLINE_ORIGINAL), "back-feet-outline-original");
	EXPECT_STREQ(SettingsSkinPreviewCacheLayerName(SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_ORIGINAL), "body-outline-original");
	EXPECT_STREQ(SettingsSkinPreviewCacheLayerName(SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_COLORABLE), "body-colorable");
	EXPECT_STREQ(SettingsSkinPreviewCacheLayerName(SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_COLORABLE), "front-feet-colorable");
}

TEST(SettingsSkinPreviewCache, PathSanitizesInvalidFilenameCharacters)
{
	const auto Path = BuildSettingsSkinPreviewCachePath("a:b/c?d*e", 1, "hash");
	EXPECT_EQ(Path, "qmclient/skins/preview_cache/a_b_c_d_e--v1--hash.webp");
}

TEST(SettingsSkinPreviewCache, ImageVisibilityUsesAlphaForRgbaCacheLayers)
{
	std::array<uint8_t, 8> aTransparentPixels = {255, 255, 255, 0, 128, 64, 32, 0};
	CImageInfo Transparent;
	Transparent.m_Width = 2;
	Transparent.m_Height = 1;
	Transparent.m_Format = CImageInfo::FORMAT_RGBA;
	Transparent.m_pData = aTransparentPixels.data();
	EXPECT_FALSE(SettingsSkinPreviewCacheImageHasVisiblePixels(Transparent));

	std::array<uint8_t, 8> aVisiblePixels = {0, 0, 0, 0, 128, 64, 32, 1};
	CImageInfo Visible;
	Visible.m_Width = 2;
	Visible.m_Height = 1;
	Visible.m_Format = CImageInfo::FORMAT_RGBA;
	Visible.m_pData = aVisiblePixels.data();
	EXPECT_TRUE(SettingsSkinPreviewCacheImageHasVisiblePixels(Visible));
	EXPECT_EQ(SettingsSkinPreviewCacheVisiblePixelCount(Visible), 1u);
}

TEST(SettingsSkinPreviewCache, ImageSetRejectsCompletelyTransparentLayerGroup)
{
	std::array<uint8_t, 4> aTransparentPixel = {0, 0, 0, 0};
	std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> aImages;
	for(CImageInfo &Image : aImages)
	{
		Image.m_Width = 1;
		Image.m_Height = 1;
		Image.m_Format = CImageInfo::FORMAT_RGBA;
		Image.m_pData = aTransparentPixel.data();
	}
	EXPECT_FALSE(SettingsSkinPreviewCacheImagesHaveVisiblePixels(aImages));

	std::array<uint8_t, 4> aVisiblePixel = {255, 255, 255, 1};
	aImages[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_pData = aVisiblePixel.data();
	EXPECT_TRUE(SettingsSkinPreviewCacheImagesHaveVisiblePixels(aImages));
}

TEST(SettingsSkinPreviewCache, MissingManifestCannotLoadLayerGroup)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	const auto Manifest = ParseSettingsSkinPreviewCacheManifest("");
	EXPECT_FALSE(Manifest.has_value());
	EXPECT_FALSE(SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key));
}

TEST(SettingsSkinPreviewCache, HalfLayerGroupManifestCannotLoad)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	auto Manifest = BuildSettingsSkinPreviewCacheManifest(Key);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		Manifest.m_aLayers[LayerIndex].m_Path = BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Manifest.m_aLayers[LayerIndex].m_Width = 64;
		Manifest.m_aLayers[LayerIndex].m_Height = 64;
		Manifest.m_aLayers[LayerIndex].m_VisiblePixels = LayerIndex == SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL ? 1 : 0;
		Manifest.m_aLayers[LayerIndex].m_EncodedBytes = 128;
	}
	Manifest.m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_COLORABLE].m_EncodedBytes = 0;

	EXPECT_FALSE(SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key));
}

TEST(SettingsSkinPreviewCache, ManifestRejectsDifferentFatKey)
{
	const SSettingsSkinPreviewCacheKey Normal{"beast", 1, 64, "hash", 0, 0};
	const SSettingsSkinPreviewCacheKey Fat{"beast", 1, 64, "hash", 0, 1};
	auto Manifest = BuildSettingsSkinPreviewCacheManifest(Normal);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		Manifest.m_aLayers[LayerIndex].m_Path = BuildSettingsSkinPreviewCachePath(Normal, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Manifest.m_aLayers[LayerIndex].m_Width = 64;
		Manifest.m_aLayers[LayerIndex].m_Height = 64;
		Manifest.m_aLayers[LayerIndex].m_VisiblePixels = LayerIndex == SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL ? 1 : 0;
		Manifest.m_aLayers[LayerIndex].m_EncodedBytes = 128;
	}

	EXPECT_FALSE(SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Fat));
}

TEST(SettingsSkinPreviewCache, FullyTransparentManifestCannotLoad)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	auto Manifest = BuildSettingsSkinPreviewCacheManifest(Key);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		Manifest.m_aLayers[LayerIndex].m_Path = BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Manifest.m_aLayers[LayerIndex].m_Width = 64;
		Manifest.m_aLayers[LayerIndex].m_Height = 64;
		Manifest.m_aLayers[LayerIndex].m_EncodedBytes = 128;
	}

	EXPECT_FALSE(SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key));
}

TEST(SettingsSkinPreviewCache, ManifestRoundTripPreservesLayerMetadata)
{
	const SSettingsSkinPreviewCacheKey Key{"beast skin", 1, 64, "hash", 0, 1};
	auto Manifest = BuildSettingsSkinPreviewCacheManifest(Key);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		Manifest.m_aLayers[LayerIndex].m_Path = BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Manifest.m_aLayers[LayerIndex].m_Width = 64 + LayerIndex;
		Manifest.m_aLayers[LayerIndex].m_Height = 32 + LayerIndex;
		Manifest.m_aLayers[LayerIndex].m_VisiblePixels = LayerIndex + 1;
		Manifest.m_aLayers[LayerIndex].m_EncodedBytes = 128 + LayerIndex;
	}

	const auto Parsed = ParseSettingsSkinPreviewCacheManifest(SerializeSettingsSkinPreviewCacheManifest(Manifest));
	ASSERT_TRUE(Parsed.has_value());
	EXPECT_TRUE(SettingsSkinPreviewCacheManifestAllowsLoad(Parsed, Key));
	EXPECT_EQ(Parsed->m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_Path, BuildSettingsSkinPreviewCachePath(Key, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL));
	EXPECT_EQ(Parsed->m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_Width, 64 + SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL);
	EXPECT_EQ(Parsed->m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_Height, 32 + SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL);
	EXPECT_EQ(Parsed->m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_VisiblePixels, SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL + 1);
	EXPECT_EQ(Parsed->m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_EncodedBytes, 128 + SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL);
}

TEST(SettingsSkinPreviewCache, ManifestRejectsMissingLayerDimensions)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	auto Manifest = BuildSettingsSkinPreviewCacheManifest(Key);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		Manifest.m_aLayers[LayerIndex].m_Path = BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Manifest.m_aLayers[LayerIndex].m_Width = 64;
		Manifest.m_aLayers[LayerIndex].m_Height = 64;
		Manifest.m_aLayers[LayerIndex].m_VisiblePixels = LayerIndex == SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL ? 1 : 0;
		Manifest.m_aLayers[LayerIndex].m_EncodedBytes = 128;
	}
	Manifest.m_aLayers[SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL].m_Width = 0;
	EXPECT_FALSE(SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key));
}

TEST(SettingsSkinPreviewCache, AtomicPublishPlanPublishesManifestLast)
{
	const SSettingsSkinPreviewCacheKey Key{"beast", 1, 64, "hash", 0};
	const auto Plan = BuildSettingsSkinPreviewCacheAtomicPublishPlan(Key);
	ASSERT_EQ(Plan.m_vSteps.size(), (size_t)NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS + 1);
	EXPECT_EQ(Plan.m_vSteps.back().m_TemporaryPath, BuildSettingsSkinPreviewCacheTemporaryPath(BuildSettingsSkinPreviewCacheManifestPath(Key)));
	EXPECT_EQ(Plan.m_vSteps.back().m_FinalPath, BuildSettingsSkinPreviewCacheManifestPath(Key));
	for(size_t Index = 0; Index + 1 < Plan.m_vSteps.size(); ++Index)
	{
		EXPECT_NE(Plan.m_vSteps[Index].m_FinalPath, BuildSettingsSkinPreviewCacheManifestPath(Key));
		EXPECT_NE(Plan.m_vSteps[Index].m_TemporaryPath, Plan.m_vSteps[Index].m_FinalPath);
	}
}

TEST(SettingsSkinPreviewCache, GeneratorPublishesLayerGroupWithManifest)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::START_BUILD_JOB"), std::string::npos);
	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::WAIT_BUILD_JOB"), std::string::npos);
	EXPECT_NE(Source.find("CSettingsSkinPreviewCacheBuildJob"), std::string::npos);
	EXPECT_NE(Source.find("BuildSettingsSkinPreviewCacheTemporaryPath"), std::string::npos);
	EXPECT_NE(Source.find("BuildSettingsSkinPreviewCacheManifest("), std::string::npos);
	EXPECT_NE(Source.find("ESettingsWarmupCost::PREVIEW_CACHE_IO"), std::string::npos);
	EXPECT_NE(Source.find("m_Manifest.m_aLayers[LayerIndex].m_Width"), std::string::npos);
	EXPECT_NE(Source.find("m_Manifest.m_aLayers[LayerIndex].m_Height"), std::string::npos);
	EXPECT_NE(Source.find("PublishDiskCacheArtifactsAtomically(m_TeeSkinPreviewCacheJob.m_Key"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, VisibleRowsOnlyQueueDiskCacheLoad)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	const size_t QueuePreviewCacheCandidate = Source.find("auto QueuePreviewCacheCandidate = [&]");
	ASSERT_NE(QueuePreviewCacheCandidate, std::string::npos);
	const size_t QueueBodyEnd = Source.find("\n\t};", QueuePreviewCacheCandidate);
	ASSERT_NE(QueueBodyEnd, std::string::npos);
	const std::string QueueBody = Source.substr(QueuePreviewCacheCandidate, QueueBodyEnd - QueuePreviewCacheCandidate);
	EXPECT_EQ(QueueBody.find("LoadTexturesFromDisk"), std::string::npos);
	EXPECT_EQ(QueueBody.find("LoadLayerImagesFromDisk"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, DiskCacheLoadsUseBackgroundJob)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	EXPECT_NE(Source.find("CSettingsSkinPreviewCacheDiskLoadJob"), std::string::npos);
	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::START_LOAD_DISK_JOB"), std::string::npos);
	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::WAIT_LOAD_DISK_JOB"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, DiskLoadValidatesManifestVisiblePixels)
{
	std::ifstream File("src/game/client/components/settings_skin_preview_cache.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	const size_t LoadLayerImagesFromDisk = Source.find("CSettingsSkinPreviewCache::LoadLayerImagesFromDisk");
	ASSERT_NE(LoadLayerImagesFromDisk, std::string::npos);
	const size_t LoadLayerImagesFromDiskEnd = Source.find("\n}", LoadLayerImagesFromDisk);
	ASSERT_NE(LoadLayerImagesFromDiskEnd, std::string::npos);
	const std::string Body = Source.substr(LoadLayerImagesFromDisk, LoadLayerImagesFromDiskEnd - LoadLayerImagesFromDisk);
	EXPECT_NE(Body.find("SettingsSkinPreviewCacheVisiblePixelCount(aImages[LayerIndex])"), std::string::npos);
	EXPECT_NE(Body.find("aImages[LayerIndex].m_Width"), std::string::npos);
	EXPECT_NE(Body.find("Manifest->m_aLayers[LayerIndex].m_Width"), std::string::npos);
	EXPECT_NE(Body.find("aImages[LayerIndex].m_Height"), std::string::npos);
	EXPECT_NE(Body.find("Manifest->m_aLayers[LayerIndex].m_Height"), std::string::npos);
	EXPECT_NE(Body.find("Manifest->m_aLayers[LayerIndex].m_VisiblePixels"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, PruneRemovesManifestWithLayerGroup)
{
	std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries = {
		{"qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0.manifest", 12},
		{"qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0--body-outline-original.webp", 10},
		{"qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0--body-original.webp", 11},
		{"qmclient/skins/preview_cache/new--v1--hash--s50--e0--f0.manifest", 22},
		{"qmclient/skins/preview_cache/new--v1--hash--s50--e0--f0--body-original.webp", 21},
	};
	const auto vDelete = ComputeSettingsSkinPreviewCachePruneList(vEntries, 1);
	ASSERT_EQ(vDelete.size(), 3u);
	EXPECT_EQ(vDelete[0], "qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0.manifest");
	EXPECT_EQ(vDelete[1], "qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0--body-outline-original.webp");
	EXPECT_EQ(vDelete[2], "qmclient/skins/preview_cache/old--v1--hash--s50--e0--f0--body-original.webp");
}

TEST(SettingsSkinPreviewCache, FirstFrameGenerationDoesNotReplaceLivePreview)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_EQ(Source.find("CachedTextures = GeneratePreviewCache"), std::string::npos);
	EXPECT_NE(Source.find("PumpPreviewCacheJob();"), std::string::npos);
	EXPECT_NE(Source.find("const bool CachedPreviewReady = CachedTextures.has_value();"), std::string::npos);
	EXPECT_NE(Source.find("if(CachedPreviewReady)"), std::string::npos);
	EXPECT_NE(Source.find("else\n\t\t\t\tRenderTools()->RenderTee"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, CpuPreviewGenerationNoLongerTouchesUiClipOrRenderTargets)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("class CSettingsSkinPreviewCacheBuildJob"), std::string::npos);
	EXPECT_EQ(Source.find("auto BeginPreviewCacheReadback = [&]"), std::string::npos);
	EXPECT_EQ(Source.find("Graphics()->BeginRenderTarget("), std::string::npos);
	EXPECT_EQ(Source.find("Graphics()->BeginRenderTargetReadback("), std::string::npos);
	EXPECT_EQ(Source.find("class CScopedSettingsSkinPreviewClip"), std::string::npos);
	EXPECT_EQ(Source.find("class CScopedSettingsSkinPreviewScreen"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, TeeSettingsSkinListClearsSixupBeforeApplyingSkin)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	const size_t OwnSkinCopy = Source.find("CTeeRenderInfo Info = OwnSkinInfo;");
	ASSERT_NE(OwnSkinCopy, std::string::npos);
	const size_t ResetSixup = Source.find("Sixup.Reset();", OwnSkinCopy);
	const size_t ApplySkin = Source.find("Info.Apply(pSkin);", OwnSkinCopy);
	ASSERT_NE(ResetSixup, std::string::npos);
	ASSERT_NE(ApplySkin, std::string::npos);
	EXPECT_LT(ResetSixup, ApplySkin);
}

TEST(SettingsSkinPreviewCache, GenerationFailureDoesNotSuppressFutureAttempts)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_EQ(Source.find("s_FailedPreviewCachePaths"), std::string::npos);
	EXPECT_EQ(Source.find("FailedPreviewCachePaths"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, PreviewGenerationNoLongerDependsOnRenderTargetSupport)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("const bool PreviewCacheUseAllowed = !Ui()->RenderOnly();"), std::string::npos);
	EXPECT_NE(Source.find("const bool PreviewCacheGenerateAllowed = PreviewCacheUseAllowed;"), std::string::npos);
	const size_t QueuePreviewCacheCandidate = Source.find("auto QueuePreviewCacheCandidate = [&]");
	ASSERT_NE(QueuePreviewCacheCandidate, std::string::npos);
	const size_t QueueBodyEnd = Source.find("\n\t};", QueuePreviewCacheCandidate);
	ASSERT_NE(QueueBodyEnd, std::string::npos);
	const std::string QueueBody = Source.substr(QueuePreviewCacheCandidate, QueueBodyEnd - QueuePreviewCacheCandidate);
	EXPECT_NE(QueueBody.find("if(!PreviewCacheUseAllowed)"), std::string::npos);
	EXPECT_NE(QueueBody.find("return;"), std::string::npos);
	EXPECT_EQ(Source.find("Graphics()->IsRenderTargetSupported()"), std::string::npos);
	EXPECT_EQ(Source.find("RenderTargetSupportReason()"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, QueueDiagnosticsDoNotLogEveryCandidate)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_EQ(Source.find("preview cache queued skin"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, PendingSkinsDoNotSpamNotAllowedLogs)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("pSkinContainer->State() == CSkins::CSkinContainer::EState::LOADED)"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, VisibleRowsDoNotSynchronouslyLoadDiskCache)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	const size_t VisibleRequest = Source.find("SkinListEntry.RequestLoad(ESettingsResourcePriority::VISIBLE);");
	const size_t ClipEnable = Source.find("Ui()->ClipEnable(&TeeClip);", VisibleRequest);
	ASSERT_NE(VisibleRequest, std::string::npos);
	ASSERT_NE(ClipEnable, std::string::npos);
	const std::string VisiblePreviewBlock = Source.substr(VisibleRequest, ClipEnable - VisibleRequest);
	EXPECT_EQ(VisiblePreviewBlock.find("LoadTexturesFromDisk"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, MaintenanceOnlyRunsWhenSkinListIsIdleButWarmupIsSeparate)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("const bool SkinListRangeStable"), std::string::npos);
	EXPECT_NE(Source.find("s_SkinListStableFrames"), std::string::npos);
	EXPECT_NE(Source.find("const bool SkinListInputActive"), std::string::npos);
	EXPECT_NE(Source.find("s_ListBox.ScrollbarAnimating()"), std::string::npos);
	EXPECT_NE(Source.find("SettingsSkinPreviewCacheShouldRunMaintenance("), std::string::npos);
	EXPECT_NE(Source.find("SettingsSkinBackgroundWarmupShouldRun("), std::string::npos);
	EXPECT_NE(Source.find("PrefetchWarmupAllowed && Index <"), std::string::npos);
	EXPECT_NE(Source.find("BackgroundWarmupAllowed && WarmupIndex <"), std::string::npos);
	EXPECT_EQ(Source.find("const bool IdleWarmupAllowed = PreviewCacheMaintenanceAllowed;"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, BackgroundWarmupKeepsRunningWhenLoadedWindowIsFull)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("const bool SkinLoadWindowFull"), std::string::npos);
	EXPECT_NE(Source.find("const bool BackgroundWarmupAllowed = SettingsSkinBackgroundWarmupShouldRun(true, VisibleBacklog, SkinListInputActive, PreviewCacheMaintenanceAllowed);"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, QueuedGenerationCandidatesDoNotPinLoadedSourceSkin)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("ReleasePreviewCacheCandidatePin"), std::string::npos);
	EXPECT_NE(Source.find("Candidate.m_SourceSkinPinned"), std::string::npos);
	EXPECT_EQ(Source.find("Existing->m_SourceSkinPinned = GameClient()->m_Skins.AcquireSettingsPreviewCachePin"), std::string::npos);
	EXPECT_EQ(Source.find("Candidate.m_SourceSkinPinned = GameClient()->m_Skins.AcquireSettingsPreviewCachePin"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, BackgroundLoadedSkinsSignalPreviewGenerationImmediately)
{
	std::ifstream SkinsFile("src/game/client/components/skins.cpp");
	ASSERT_TRUE(SkinsFile.good());
	std::stringstream SkinsBuffer;
	SkinsBuffer << SkinsFile.rdbuf();
	const std::string SkinsSource = SkinsBuffer.str();
	EXPECT_NE(SkinsSource.find("m_SettingsPreviewWarmupReadyQueued.insert(pSkinContainer->Name())"), std::string::npos);
	EXPECT_NE(SkinsSource.find("m_vSettingsPreviewWarmupReadySkins.push_back(pSkinContainer->Name())"), std::string::npos);

	std::ifstream MenusFile("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(MenusFile.good());
	std::stringstream MenusBuffer;
	MenusBuffer << MenusFile.rdbuf();
	const std::string MenusSource = MenusBuffer.str();
	EXPECT_NE(MenusSource.find("ConsumeSettingsPreviewWarmupReadySkins()"), std::string::npos);
	EXPECT_NE(MenusSource.find("QueueLoadedPreviewCacheGeneration(pLoadedContainer, ESettingsResourcePriority::BACKGROUND"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, PreviewGenerationUsesCpuBuildJobInsteadOfGpuReadback)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("class CSettingsSkinPreviewCacheBuildJob"), std::string::npos);
	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::START_BUILD_JOB"), std::string::npos);
	EXPECT_NE(Source.find("ESettingsSkinPreviewCacheJobStage::WAIT_BUILD_JOB"), std::string::npos);
	EXPECT_EQ(Source.find("ESettingsSkinPreviewCacheJobStage::BEGIN_BLACK_READBACK"), std::string::npos);
	EXPECT_EQ(Source.find("ESettingsSkinPreviewCacheJobStage::WAIT_BLACK_READBACK"), std::string::npos);
	EXPECT_EQ(Source.find("ESettingsSkinPreviewCacheJobStage::BEGIN_WHITE_READBACK"), std::string::npos);
	EXPECT_EQ(Source.find("ESettingsSkinPreviewCacheJobStage::WAIT_WHITE_READBACK"), std::string::npos);
	EXPECT_EQ(Source.find("Graphics()->BeginRenderTarget("), std::string::npos);
	EXPECT_EQ(Source.find("Graphics()->BeginRenderTargetReadback("), std::string::npos);
}

TEST(SettingsSkinPreviewCache, PreviewBuildUsesSixTimesSupersampleTarget)
{
	EXPECT_EQ(SETTINGS_TEE_PREVIEW_CACHE_SUPERSAMPLE, 6);
	EXPECT_EQ(ComputeSettingsTeePreviewCacheTargetLength(54.0f), 324);
	EXPECT_EQ(ComputeSettingsTeePreviewCacheTargetLength(44.0f), 264);
	EXPECT_EQ(ComputeSettingsTeePreviewCacheTargetLength(0.0f), 1);
	const auto Placement = ComputeSettingsTeePreviewCacheBuildPlacement(50.0f, 3.0f, -2.0f);
	EXPECT_FLOAT_EQ(Placement.m_Size, 300.0f);
	EXPECT_FLOAT_EQ(Placement.m_OffsetX, 18.0f);
	EXPECT_FLOAT_EQ(Placement.m_OffsetY, -12.0f);
}

TEST(SettingsSkinPreviewCache, ActivePreviewPinsReleaseOnShutdownAndDirectoryInvalidation)
{
	std::ifstream File("src/game/client/components/menus.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();
	EXPECT_NE(Source.find("void CMenus::ResetActiveTeeSkinPreviewCacheJob()"), std::string::npos);
	EXPECT_NE(Source.find("ResetActiveTeeSkinPreviewCacheJob();"), std::string::npos);
	EXPECT_NE(Source.find("ReleaseSettingsPreviewCachePin"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, LeavingSettingsTeeResetsPreviewWork)
{
	std::ifstream MenusFile("src/game/client/components/menus.cpp");
	ASSERT_TRUE(MenusFile.good());
	std::stringstream MenusBuffer;
	MenusBuffer << MenusFile.rdbuf();
	const std::string MenusSource = MenusBuffer.str();
	EXPECT_NE(MenusSource.find("if(g_Config.m_UiSettingsPage == SETTINGS_TEE)"), std::string::npos);

	std::ifstream SettingsFile("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(SettingsFile.good());
	std::stringstream SettingsBuffer;
	SettingsBuffer << SettingsFile.rdbuf();
	const std::string SettingsSource = SettingsBuffer.str();
	EXPECT_NE(SettingsSource.find("if(s_PrevSettingsPage == SETTINGS_TEE || g_Config.m_UiSettingsPage == SETTINGS_TEE)"), std::string::npos);
	EXPECT_NE(SettingsSource.find("m_TeeSkinPreviewCacheRetryAt.clear();"), std::string::npos);
	EXPECT_NE(SettingsSource.find("m_TeeSkinPreviewCache.ClearMemoryCache();"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, MaintenanceDecisionRequiresRuntimeAndIdleFrames)
{
	EXPECT_FALSE(SettingsSkinPreviewCacheShouldRunMaintenance(false, true, false, false, 8, 8));
	EXPECT_FALSE(SettingsSkinPreviewCacheShouldRunMaintenance(true, true, true, false, 8, 8));
	EXPECT_FALSE(SettingsSkinPreviewCacheShouldRunMaintenance(true, true, false, true, 8, 8));
	EXPECT_FALSE(SettingsSkinPreviewCacheShouldRunMaintenance(true, false, false, false, 8, 8));
	EXPECT_FALSE(SettingsSkinPreviewCacheShouldRunMaintenance(true, true, false, false, 7, 8));
	EXPECT_TRUE(SettingsSkinPreviewCacheShouldRunMaintenance(true, true, false, false, 8, 8));
}
