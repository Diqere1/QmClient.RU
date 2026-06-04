#include <game/client/components/settings_skin_preview_cache.h>

#include <test/test.h>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

TEST(SettingsSkinPreviewCache, PathUsesSanitizedNameVersionAndHash)
{
	const auto Path = BuildSettingsSkinPreviewCachePath("beast skin", 1, "7f3c1d8a2b44");
	EXPECT_EQ(Path, "qmclient/skins/preview_cache/beast_skin--v1--7f3c1d8a2b44.webp");
}

TEST(SettingsSkinPreviewCache, ParametricArtifactCanonicalizesPerColorVariants)
{
	SSettingsSkinPreviewCacheKey A{"beast", 1, 64, "hash-a", 0};
	A.m_UseCustomColor = 1;
	A.m_ColorBody = 0x123456;
	A.m_ColorFeet = 0xabcdef;

	SSettingsSkinPreviewCacheKey B = A;
	B.m_ColorBody = 0x654321;
	B.m_ColorFeet = 0x00ff00;

	const SSettingsSkinPreviewCacheKey CanonicalA = CanonicalizeSettingsSkinPreviewArtifactKey(A);
	const SSettingsSkinPreviewCacheKey CanonicalB = CanonicalizeSettingsSkinPreviewArtifactKey(B);
	EXPECT_EQ(CanonicalA, CanonicalB);
	EXPECT_EQ(CanonicalA.m_ColorBody, 0);
	EXPECT_EQ(CanonicalA.m_ColorFeet, 0);
}

TEST(SettingsSkinPreviewCache, KeyHashTracksBehaviorAcrossKeyFields)
{
	const SSettingsSkinPreviewCacheKey Base{"beast", 3, 64, "hash-a", 0, 0};
	const SSettingsSkinPreviewCacheKey SameAsBase{"beast", 3, 64, "hash-a", 0, 0};
	const SSettingsSkinPreviewCacheKey DifferentVersion{"beast", 4, 64, "hash-a", 0, 0};
	const SSettingsSkinPreviewCacheKey DifferentHash{"beast", 3, 64, "hash-b", 0, 0};

	const SSettingsSkinPreviewCacheKeyHash Hasher;
	EXPECT_EQ(Hasher(Base), Hasher(SameAsBase));
	EXPECT_NE(Hasher(Base), Hasher(DifferentVersion));
	EXPECT_NE(Hasher(Base), Hasher(DifferentHash));
}

TEST(SettingsSkinPreviewCache, DisabledReasonDistinguishesMissingHashWhiteFeetAndSixup)
{
	EXPECT_EQ(SettingsSkinPreviewCacheDisabledReason(false, false, false, false, false), ESettingsSkinPreviewCacheDisabledReason::MISSING_HASH);
	EXPECT_EQ(SettingsSkinPreviewCacheDisabledReason(false, false, true, false), ESettingsSkinPreviewCacheDisabledReason::WHITE_FEET);
	EXPECT_EQ(SettingsSkinPreviewCacheDisabledReason(false, false, false, true), ESettingsSkinPreviewCacheDisabledReason::SIXUP);
}

TEST(SettingsSkinPreviewCache, StableHitSourceNamesRemainAvailableForPerfLogs)
{
	EXPECT_TRUE(SettingsSkinPreviewShouldBecomeStable(ESettingsSkinPreviewHitSource::DISK_RESTORE));
	EXPECT_TRUE(SettingsSkinPreviewShouldBecomeStable(ESettingsSkinPreviewHitSource::SOURCE_GENERATION));
	EXPECT_STREQ(SettingsSkinPreviewHitSourceName(ESettingsSkinPreviewHitSource::MEMORY_HIT), "memory-hit");
	EXPECT_STREQ(SettingsSkinPreviewHitSourceName(ESettingsSkinPreviewHitSource::DISK_RESTORE), "disk-restore");
}

TEST(SettingsSkinPreviewCache, PreviewBytesEstimateUsesUploadedTextureArea)
{
	EXPECT_EQ(SettingsSkinPreviewBytesEstimate(64, 64, 4), static_cast<size_t>(64 * 64 * 4 * 4));
	EXPECT_FALSE(SettingsSkinPreviewEvictPreviewFirst(true, true));
	EXPECT_FALSE(SettingsSkinPreviewCanPruneDiskAlongsideMemory(true, true));
}

TEST(SettingsSkinPreviewCache, VersionTracksCurrentArtifactRevision)
{
	EXPECT_EQ(SETTINGS_SKIN_PREVIEW_CACHE_VERSION, 7);
}

TEST(SettingsSkinPreviewCache, TeePageNoLongerConsumesPreviewCache)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	const size_t RenderPos = Source.find("void CMenus::RenderSettingsTee(CUIRect MainView)");
	ASSERT_NE(RenderPos, std::string::npos);
	const size_t RenderEnd = Source.find("void CMenus::RenderSettingsAppearance", RenderPos);
	ASSERT_NE(RenderEnd, std::string::npos);
	const std::string RenderBody = Source.substr(RenderPos, RenderEnd - RenderPos);

	EXPECT_EQ(RenderBody.find("GetSkinPreviewCache("), std::string::npos);
	EXPECT_EQ(RenderBody.find("TryBuildSkinPreviewCacheKey("), std::string::npos);
	EXPECT_EQ(RenderBody.find("RenderSettingsSkinPreviewCacheLayers("), std::string::npos);
	EXPECT_EQ(RenderBody.find("ConsumeSettingsPreviewWarmupReadySkins()"), std::string::npos);
	EXPECT_NE(RenderBody.find("RenderTools()->RenderTee"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, MenusSettingsNoLongerIncludesPreviewCacheHelpers)
{
	std::ifstream File("src/game/client/components/menus_settings.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	EXPECT_EQ(Source.find("#include <game/client/components/settings_skin_preview_cache.h>"), std::string::npos);
	EXPECT_EQ(Source.find("void CMenus::ResetActiveTeeSkinPreviewWork"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, SkinsNoLongerPublishesPreviewCacheArtifacts)
{
	std::ifstream File("src/game/client/components/skins.cpp");
	ASSERT_TRUE(File.good());
	std::stringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Source = Buffer.str();

	EXPECT_EQ(Source.find("#include <game/client/components/settings_skin_preview_cache.h>"), std::string::npos);
	EXPECT_EQ(Source.find("TryPublishSettingsSkinPreviewCacheFromSourceData"), std::string::npos);
	EXPECT_EQ(Source.find("QueueSettingsSkinPreviewSourcePublishJob"), std::string::npos);
	EXPECT_EQ(Source.find("CSettingsSkinPreviewSourcePublishJob"), std::string::npos);
}

TEST(SettingsSkinPreviewCache, SkinsNoLongerTracksPreviewCachePinsOrHashes)
{
	std::ifstream HeaderFile("src/game/client/components/skins.h");
	ASSERT_TRUE(HeaderFile.good());
	std::stringstream HeaderBuffer;
	HeaderBuffer << HeaderFile.rdbuf();
	const std::string HeaderSource = HeaderBuffer.str();

	EXPECT_EQ(HeaderSource.find("SettingsPreviewCacheContentHash"), std::string::npos);
	EXPECT_EQ(HeaderSource.find("AcquireSettingsPreviewCachePin"), std::string::npos);
	EXPECT_EQ(HeaderSource.find("ReleaseSettingsPreviewCachePin"), std::string::npos);
	EXPECT_EQ(HeaderSource.find("m_SettingsPreviewCachePinCount"), std::string::npos);

	std::ifstream SourceFile("src/game/client/components/skins.cpp");
	ASSERT_TRUE(SourceFile.good());
	std::stringstream SourceBuffer;
	SourceBuffer << SourceFile.rdbuf();
	const std::string Source = SourceBuffer.str();

	EXPECT_EQ(Source.find("IsSettingsPreviewCachePinned()"), std::string::npos);
	EXPECT_EQ(Source.find("LogSettingsSkinPreviewHashReadyEvent"), std::string::npos);
}
