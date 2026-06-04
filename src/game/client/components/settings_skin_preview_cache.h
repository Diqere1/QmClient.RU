#ifndef GAME_CLIENT_COMPONENTS_SETTINGS_SKIN_PREVIEW_CACHE_H
#define GAME_CLIENT_COMPONENTS_SETTINGS_SKIN_PREVIEW_CACHE_H

#include <engine/graphics.h>
#include <engine/image.h>
#include <engine/storage.h>
#include <game/client/components/settings_runtime_cache.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_VERSION = 7;
inline constexpr int SETTINGS_TEE_PREVIEW_CACHE_SUPERSAMPLE = 6;
inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_MANIFEST_VERSION = 3;
inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_KEY_LIMIT = 512;
inline constexpr size_t SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_ENTRIES = 512;
inline constexpr size_t SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_BYTES = 512u * 64u * 64u * 4u;

struct SSettingsTeePreviewCacheBuildPlacement
{
	float m_Size;
	float m_OffsetX;
	float m_OffsetY;
};

inline int ComputeSettingsTeePreviewCacheTargetLength(float PreviewLength)
{
	return std::max(1, (int)std::ceil(PreviewLength * SETTINGS_TEE_PREVIEW_CACHE_SUPERSAMPLE));
}

inline SSettingsTeePreviewCacheBuildPlacement ComputeSettingsTeePreviewCacheBuildPlacement(float PreviewSize, float OffsetX, float OffsetY)
{
	const float Supersample = (float)SETTINGS_TEE_PREVIEW_CACHE_SUPERSAMPLE;
	return {
		PreviewSize * Supersample,
		OffsetX * Supersample,
		OffsetY * Supersample,
	};
}

inline bool SettingsSkinPreviewSuppressStatusIcon(bool SourceLoaded, bool CachedPreviewReady, bool TransientSourceState)
{
	return SourceLoaded || (TransientSourceState && CachedPreviewReady);
}

inline bool SettingsSkinPreviewShowProgress(bool CachedPreviewReady, bool TransientSourceState)
{
	return TransientSourceState && !CachedPreviewReady;
}

enum ESettingsSkinPreviewCacheLayer
{
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_OUTLINE_ORIGINAL = 0,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_OUTLINE_COLORABLE,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_ORIGINAL,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_COLORABLE,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_OUTLINE_ORIGINAL,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_OUTLINE_COLORABLE,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_ORIGINAL,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_COLORABLE,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_COLORABLE,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_ORIGINAL,
	SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_COLORABLE,
	NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS,
};

inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_FINAL_PREVIEW_LAYER_COUNT = 1;

struct SSettingsSkinPreviewCacheKey
{
	std::string m_SkinName;
	int m_Version = 0;
	int m_SizeBucket = 0;
	std::string m_ContentHash;
	int m_Emote = 0;
	int m_FatSkins = 0;
	int m_TinyTee = 0;
	int m_TinyTeeSize = 100;
	int m_UseCustomColor = 0;
	int m_ColorBody = 0;
	int m_ColorFeet = 0;

	bool operator==(const SSettingsSkinPreviewCacheKey &Other) const = default;
};

struct SSettingsSkinPreviewCacheKeyHash
{
	size_t operator()(const SSettingsSkinPreviewCacheKey &Key) const;
};

enum class ESettingsSkinPreviewCacheDisabledReason
{
	NONE = 0,
	WHITE_FEET,
	SIXUP,
	MISSING_HASH,
	BACKEND_UNSUPPORTED,
};

enum class ESettingsSkinPreviewHitSource
{
	MEMORY_HIT = 0,
	DISK_RESTORE,
	SOURCE_GENERATION,
};

enum class ESettingsSkinPreviewArtifactType
{
	FINAL_PREVIEW = 0,
	PARAMETRIC_LAYERS,
};

struct SSettingsSkinPreviewTelemetryKey
{
	SSettingsSkinPreviewCacheKey m_ArtifactKey;
};

struct SSettingsSkinPreviewTelemetryEvent
{
	SSettingsSkinPreviewTelemetryKey m_Key;
	ESettingsSkinPreviewCacheDisabledReason m_DisabledReason = ESettingsSkinPreviewCacheDisabledReason::NONE;
	ESettingsWarmupMissReason m_BudgetReason = ESettingsWarmupMissReason::NONE;
};

struct SSettingsSkinPreviewCacheFileEntry
{
	std::string m_Path;
	int64_t m_ModifiedTime = 0;
};

struct SSettingsSkinPreviewCacheManifestLayer
{
	std::string m_Path;
	int m_Width = 0;
	int m_Height = 0;
	uint64_t m_VisiblePixels = 0;
	uint64_t m_EncodedBytes = 0;
};

struct SSettingsSkinPreviewCacheManifest
{
	int m_Version = SETTINGS_SKIN_PREVIEW_CACHE_MANIFEST_VERSION;
	SSettingsSkinPreviewCacheKey m_Key;
	ESettingsSkinPreviewArtifactType m_ArtifactType = ESettingsSkinPreviewArtifactType::FINAL_PREVIEW;
	int m_LayerCount = SETTINGS_SKIN_PREVIEW_CACHE_FINAL_PREVIEW_LAYER_COUNT;
	std::array<SSettingsSkinPreviewCacheManifestLayer, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> m_aLayers;
};

struct SSettingsSkinPreviewCacheAtomicPublishStep
{
	std::string m_TemporaryPath;
	std::string m_FinalPath;
};

struct SSettingsSkinPreviewCacheAtomicPublishPlan
{
	std::vector<SSettingsSkinPreviewCacheAtomicPublishStep> m_vSteps;
};

struct SSettingsSkinPreviewCacheTextures
{
	IGraphics::CTextureHandle m_FinalPreviewTexture;
	std::array<IGraphics::CTextureHandle, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> m_aTextures;
	size_t m_ApproxBytes = 0;

	bool IsComplete() const;
};

struct SSettingsSkinPreviewMemoryCacheEntry
{
	size_t m_ApproxBytes = 0;
	uint64_t m_LastUseSequence = 0;
};

struct SSettingsSkinPreviewSourceSprite
{
	int m_GridX = 0;
	int m_GridY = 0;
	int m_X = 0;
	int m_Y = 0;
	int m_W = 0;
	int m_H = 0;
};

std::string BuildSettingsSkinPreviewCachePath(const char *pSkinName, int Version, const char *pHash);
std::string BuildSettingsSkinPreviewCachePath(const SSettingsSkinPreviewCacheKey &Key);
std::string BuildSettingsSkinPreviewCachePath(const SSettingsSkinPreviewCacheKey &Key, ESettingsSkinPreviewCacheLayer Layer);
std::string BuildSettingsSkinPreviewCacheManifestPath(const SSettingsSkinPreviewCacheKey &Key);
std::string BuildSettingsSkinPreviewCacheTemporaryPath(const std::string &Path);
SSettingsSkinPreviewCacheManifest BuildSettingsSkinPreviewCacheManifest(const SSettingsSkinPreviewCacheKey &Key);
std::string SerializeSettingsSkinPreviewCacheManifest(const SSettingsSkinPreviewCacheManifest &Manifest);
std::optional<SSettingsSkinPreviewCacheManifest> ParseSettingsSkinPreviewCacheManifest(const std::string &Text);
bool SettingsSkinPreviewCacheManifestAllowsLoad(const std::optional<SSettingsSkinPreviewCacheManifest> &Manifest, const SSettingsSkinPreviewCacheKey &Key);
bool SettingsSkinPreviewCacheManifestAllowsLoad(const SSettingsSkinPreviewCacheManifest &Manifest, const SSettingsSkinPreviewCacheKey &Key);
SSettingsSkinPreviewCacheKey CanonicalizeSettingsSkinPreviewArtifactKey(const SSettingsSkinPreviewCacheKey &Key);
SSettingsSkinPreviewCacheAtomicPublishPlan BuildSettingsSkinPreviewCacheAtomicPublishPlan(const SSettingsSkinPreviewCacheKey &Key);
const char *SettingsSkinPreviewCacheLayerName(ESettingsSkinPreviewCacheLayer Layer);
uint64_t SettingsSkinPreviewCacheVisiblePixelCount(const CImageInfo &Image);
bool SettingsSkinPreviewCacheImageHasVisiblePixels(const CImageInfo &Image);
bool SettingsSkinPreviewCacheImagesHaveVisiblePixels(const std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages);
ESettingsSkinPreviewCacheDisabledReason SettingsSkinPreviewCacheDisabledReason(bool HasContentHash, bool RenderOnly, bool BackendUnsupported, bool WhiteFeet, bool Sixup);
ESettingsSkinPreviewCacheDisabledReason SettingsSkinPreviewCacheDisabledReason(bool RenderOnly, bool BackendUnsupported, bool WhiteFeet, bool Sixup);
bool SettingsSkinPreviewShouldBecomeStable(ESettingsSkinPreviewHitSource Source);
const char *SettingsSkinPreviewHitSourceName(ESettingsSkinPreviewHitSource Source);
bool SettingsSkinPreviewObligationSharesPin(const SSettingsSkinPreviewCacheKey &Left, const SSettingsSkinPreviewCacheKey &Right);
bool SettingsSkinPreviewShouldCreateObligation(ESettingsSkinPreviewCacheDisabledReason Reason);
size_t SettingsSkinPreviewBytesEstimate(int Width, int Height, int LayerCount);
bool SettingsSkinPreviewEvictPreviewFirst(bool HasStablePreview, bool SourceStillReclaimable);
bool SettingsSkinPreviewCanPruneDiskAlongsideMemory(bool StablePreview, bool MemoryEvicted);
bool SettingsSkinPreviewCacheAllowsLookup(bool HasContentHash, bool WhiteFeet, bool Sixup);
bool SettingsSkinPreviewCacheAllowsGeneration(bool SkinLoaded, bool HasContentHash, bool WhiteFeet, bool Sixup);
bool SettingsSkinPreviewCacheShouldRunMaintenance(bool RuntimeAllowed, bool RangeStable, bool InputActive, bool ScrollAnimating, int StableFrames, int RequiredStableFrames, bool HeavyRenderTargetWorkAllowed = true);
bool SettingsSkinPreviewCacheShouldPumpMaintenance(bool JobActive, bool MaintenanceAllowed);
bool SettingsSkinPreviewCacheShouldRequeueInterruptedJob(bool JobActive, const std::string &SkinName);
bool SettingsSkinPreviewCacheShouldDisableBackendAfterReadbackFailure(const char *pReason);
bool SettingsSkinPreviewCacheArtifactsReadyForPublish(const SSettingsSkinPreviewCacheKey &Key, const std::array<std::string, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aTemporaryLayerPaths, const SSettingsSkinPreviewCacheManifest &Manifest);
bool SettingsSkinPreviewCacheShouldRememberAfterPublish(bool MemoryBudgetAllows, bool DiskPublishSucceeded);
bool BuildSettingsSkinPreviewCacheLayerImageFromSource(const CImageInfo &Source, const SSettingsSkinPreviewSourceSprite &Sprite, CImageInfo &OutImage);
bool SettingsSkinPreviewCacheManifestUsesFinalPreview(const SSettingsSkinPreviewCacheManifest &Manifest);
std::vector<std::string> ComputeSettingsSkinPreviewCachePruneList(std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries, int MaxGroups);
std::vector<int> ComputeSettingsSkinPreviewCacheMemoryEvictionList(const std::vector<SSettingsSkinPreviewMemoryCacheEntry> &vEntries, size_t MaxEntries, size_t MaxBytes);
class CSettingsSkinPreviewCache;
CSettingsSkinPreviewCache *GetSharedSettingsSkinPreviewCache(IStorage *pStorage, IGraphics *pGraphics);

class CSettingsSkinPreviewCache
{
public:
	void Init(IStorage *pStorage, IGraphics *pGraphics);
	void Shutdown();
	void OnSkinDirectoryChanged();
	bool EnsureCacheFolders();
	void PruneDiskCache();
	std::string PathForKey(const SSettingsSkinPreviewCacheKey &Key) const;
	std::string PathForKey(const SSettingsSkinPreviewCacheKey &Key, ESettingsSkinPreviewCacheLayer Layer) const;
	std::string ManifestPathForKey(const SSettingsSkinPreviewCacheKey &Key) const;
	void RemoveDiskCache(const SSettingsSkinPreviewCacheKey &Key);
	bool WriteManifestAtomically(const SSettingsSkinPreviewCacheManifest &Manifest);
	bool PublishDiskCacheArtifactsAtomically(const SSettingsSkinPreviewCacheKey &Key, const std::array<std::string, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aTemporaryLayerPaths, const SSettingsSkinPreviewCacheManifest &Manifest);
	bool PublishSourceSkinLayerImages(const SSettingsSkinPreviewCacheKey &Key, const std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages);
	bool PublishSourceSkinFinalPreviewImage(const SSettingsSkinPreviewCacheKey &Key, const CImageInfo &Image);
	std::optional<SSettingsSkinPreviewCacheTextures> FindTextures(const SSettingsSkinPreviewCacheKey &Key);
	std::optional<SSettingsSkinPreviewCacheKey> FindDiskCacheKeyForSkin(const SSettingsSkinPreviewCacheKey &LookupKey) const;
	bool DiskCacheExists(const SSettingsSkinPreviewCacheKey &Key) const;
	bool DiskCacheArtifactsValid(const SSettingsSkinPreviewCacheKey &Key) const;
	bool LoadLayerImagesFromDisk(const SSettingsSkinPreviewCacheKey &Key, std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages, SSettingsSkinPreviewCacheManifest *pManifest = nullptr);
	bool LoadFinalPreviewImageFromDisk(const SSettingsSkinPreviewCacheKey &Key, CImageInfo &Image, SSettingsSkinPreviewCacheManifest *pManifest = nullptr);
	std::optional<SSettingsSkinPreviewCacheTextures> LoadTexturesFromDisk(const SSettingsSkinPreviewCacheKey &Key);
	void RememberTextures(const SSettingsSkinPreviewCacheKey &Key, SSettingsSkinPreviewCacheTextures Textures);
	void RememberStablePreview(const SSettingsSkinPreviewCacheKey &Key);
	void ForgetTexture(const SSettingsSkinPreviewCacheKey &Key);
	void ClearMemoryCache();
	size_t MemoryCacheSize() const;
	size_t MemoryCacheBytes() const;

private:
	struct SMemoryCacheEntry
	{
		SSettingsSkinPreviewCacheTextures m_Textures;
		uint64_t m_LastUseSequence = 0;
	};

	void ClearTextures();
	void EvictMemoryCacheIfNeeded();

		IStorage *m_pStorage = nullptr;
		IGraphics *m_pGraphics = nullptr;
		std::unordered_map<SSettingsSkinPreviewCacheKey, SMemoryCacheEntry, SSettingsSkinPreviewCacheKeyHash> m_vTextures;
		std::unordered_set<SSettingsSkinPreviewCacheKey, SSettingsSkinPreviewCacheKeyHash> m_vStableTextures;
		uint64_t m_NextUseSequence = 1;
	size_t m_MemoryCacheBytes = 0;
};

#endif
