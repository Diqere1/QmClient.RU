#ifndef GAME_CLIENT_COMPONENTS_SETTINGS_SKIN_PREVIEW_CACHE_H
#define GAME_CLIENT_COMPONENTS_SETTINGS_SKIN_PREVIEW_CACHE_H

#include <engine/graphics.h>
#include <engine/image.h>
#include <engine/storage.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_VERSION = 3;
inline constexpr int SETTINGS_TEE_PREVIEW_CACHE_SUPERSAMPLE = 6;
inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_MANIFEST_VERSION = 2;
inline constexpr int SETTINGS_SKIN_PREVIEW_CACHE_KEY_LIMIT = 512;
inline constexpr size_t SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_ENTRIES = 24;
inline constexpr size_t SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_BYTES = 96u * 1024u * 1024u;

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

struct SSettingsSkinPreviewCacheKey
{
	std::string m_SkinName;
	int m_Version = 0;
	int m_SizeBucket = 0;
	std::string m_ContentHash;
	int m_Emote = 0;
	int m_FatSkins = 0;

	bool operator==(const SSettingsSkinPreviewCacheKey &Other) const = default;
};

struct SSettingsSkinPreviewCacheKeyHash
{
	size_t operator()(const SSettingsSkinPreviewCacheKey &Key) const;
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
	int m_LayerCount = NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS;
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
	std::array<IGraphics::CTextureHandle, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> m_aTextures;
	size_t m_ApproxBytes = 0;

	bool IsComplete() const;
};

struct SSettingsSkinPreviewMemoryCacheEntry
{
	size_t m_ApproxBytes = 0;
	uint64_t m_LastUseSequence = 0;
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
SSettingsSkinPreviewCacheAtomicPublishPlan BuildSettingsSkinPreviewCacheAtomicPublishPlan(const SSettingsSkinPreviewCacheKey &Key);
const char *SettingsSkinPreviewCacheLayerName(ESettingsSkinPreviewCacheLayer Layer);
uint64_t SettingsSkinPreviewCacheVisiblePixelCount(const CImageInfo &Image);
bool SettingsSkinPreviewCacheImageHasVisiblePixels(const CImageInfo &Image);
bool SettingsSkinPreviewCacheImagesHaveVisiblePixels(const std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages);
bool SettingsSkinPreviewCacheAllowsLookup(bool HasContentHash, bool WhiteFeet, bool Sixup);
bool SettingsSkinPreviewCacheAllowsGeneration(bool SkinLoaded, bool HasContentHash, bool WhiteFeet, bool Sixup);
bool SettingsSkinPreviewCacheShouldRunMaintenance(bool RuntimeAllowed, bool RangeStable, bool InputActive, bool ScrollAnimating, int StableFrames, int RequiredStableFrames);
std::vector<std::string> ComputeSettingsSkinPreviewCachePruneList(std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries, int MaxGroups);
std::vector<int> ComputeSettingsSkinPreviewCacheMemoryEvictionList(const std::vector<SSettingsSkinPreviewMemoryCacheEntry> &vEntries, size_t MaxEntries, size_t MaxBytes);

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
	std::optional<SSettingsSkinPreviewCacheTextures> FindTextures(const SSettingsSkinPreviewCacheKey &Key);
	bool DiskCacheExists(const SSettingsSkinPreviewCacheKey &Key) const;
	bool LoadLayerImagesFromDisk(const SSettingsSkinPreviewCacheKey &Key, std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages, SSettingsSkinPreviewCacheManifest *pManifest = nullptr);
	std::optional<SSettingsSkinPreviewCacheTextures> LoadTexturesFromDisk(const SSettingsSkinPreviewCacheKey &Key);
	void RememberTextures(const SSettingsSkinPreviewCacheKey &Key, SSettingsSkinPreviewCacheTextures Textures);
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
	uint64_t m_NextUseSequence = 1;
	size_t m_MemoryCacheBytes = 0;
};

#endif
