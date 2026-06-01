#include "settings_skin_preview_cache.h"

#include <base/log.h>
#include <base/system.h>

#include <engine/gfx/image_loader.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <unordered_map>

size_t SSettingsSkinPreviewCacheKeyHash::operator()(const SSettingsSkinPreviewCacheKey &Key) const
{
	return std::hash<std::string>{}(Key.m_SkinName) ^
	       ((size_t)Key.m_Version << 1) ^
	       ((size_t)Key.m_SizeBucket << 17) ^
	       (std::hash<std::string>{}(Key.m_ContentHash) << 1) ^
	       ((size_t)Key.m_Emote << 23) ^
	       ((size_t)Key.m_FatSkins << 29);
}

bool SSettingsSkinPreviewCacheTextures::IsComplete() const
{
	for(const IGraphics::CTextureHandle &Texture : m_aTextures)
	{
		if(!Texture.IsValid())
			return false;
	}
	return true;
}

const char *SettingsSkinPreviewCacheLayerName(ESettingsSkinPreviewCacheLayer Layer)
{
	switch(Layer)
	{
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_OUTLINE_ORIGINAL: return "back-feet-outline-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_OUTLINE_COLORABLE: return "back-feet-outline-colorable";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_ORIGINAL: return "body-outline-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_OUTLINE_COLORABLE: return "body-outline-colorable";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_OUTLINE_ORIGINAL: return "front-feet-outline-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_OUTLINE_COLORABLE: return "front-feet-outline-colorable";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_ORIGINAL: return "back-feet-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BACK_FEET_COLORABLE: return "back-feet-colorable";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_ORIGINAL: return "body-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_BODY_COLORABLE: return "body-colorable";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_ORIGINAL: return "front-feet-original";
	case SETTINGS_SKIN_PREVIEW_CACHE_LAYER_FRONT_FEET_COLORABLE: return "front-feet-colorable";
	case NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS: break;
	}
	return "unknown";
}

uint64_t SettingsSkinPreviewCacheVisiblePixelCount(const CImageInfo &Image)
{
	if(Image.m_pData == nullptr || Image.m_Width == 0 || Image.m_Height == 0)
		return 0;

	const size_t PixelCount = Image.m_Width * Image.m_Height;
	switch(Image.m_Format)
	{
	case CImageInfo::FORMAT_RGBA:
	{
		uint64_t VisiblePixels = 0;
		for(size_t Pixel = 0; Pixel < PixelCount; ++Pixel)
		{
			if(Image.m_pData[Pixel * 4 + 3] != 0)
				++VisiblePixels;
		}
		return VisiblePixels;
	}
	case CImageInfo::FORMAT_RA:
	{
		uint64_t VisiblePixels = 0;
		for(size_t Pixel = 0; Pixel < PixelCount; ++Pixel)
		{
			if(Image.m_pData[Pixel * 2 + 1] != 0)
				++VisiblePixels;
		}
		return VisiblePixels;
	}
	case CImageInfo::FORMAT_RGB:
	case CImageInfo::FORMAT_R:
		return PixelCount;
	case CImageInfo::FORMAT_UNDEFINED:
		return 0;
	}
	return 0;
}

bool SettingsSkinPreviewCacheImageHasVisiblePixels(const CImageInfo &Image)
{
	return SettingsSkinPreviewCacheVisiblePixelCount(Image) > 0;
}

bool SettingsSkinPreviewCacheImagesHaveVisiblePixels(const std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages)
{
	for(const CImageInfo &Image : aImages)
	{
		if(SettingsSkinPreviewCacheImageHasVisiblePixels(Image))
			return true;
	}
	return false;
}

bool SettingsSkinPreviewCacheAllowsLookup(bool HasContentHash, bool WhiteFeet, bool Sixup)
{
	return HasContentHash && !WhiteFeet && !Sixup;
}

bool SettingsSkinPreviewCacheAllowsGeneration(bool SkinLoaded, bool HasContentHash, bool WhiteFeet, bool Sixup)
{
	return SkinLoaded && SettingsSkinPreviewCacheAllowsLookup(HasContentHash, WhiteFeet, Sixup);
}

bool SettingsSkinPreviewCacheShouldRunMaintenance(bool RuntimeAllowed, bool RangeStable, bool InputActive, bool ScrollAnimating, int StableFrames, int RequiredStableFrames)
{
	if(!RuntimeAllowed || InputActive || ScrollAnimating)
		return false;
	if(!RangeStable)
		return false;
	return StableFrames >= RequiredStableFrames;
}

std::string BuildSettingsSkinPreviewCachePath(const char *pSkinName, int Version, const char *pHash)
{
	char aName[IO_MAX_PATH_LENGTH];
	str_copy(aName, pSkinName ? pSkinName : "skin", sizeof(aName));
	str_sanitize_filename(aName);
	for(char *pChar = aName; *pChar != '\0'; ++pChar)
	{
		if(*pChar == ' ')
			*pChar = '_';
		else if(*pChar == '/' || *pChar == '\\' || *pChar == ':' || *pChar == '?' || *pChar == '*')
			*pChar = '_';
	}

	char aBuf[IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "qmclient/skins/preview_cache/%s--v%d--%s.webp", aName, Version, pHash ? pHash : "missing");
	return aBuf;
}

std::string BuildSettingsSkinPreviewCachePath(const SSettingsSkinPreviewCacheKey &Key)
{
	std::string Path = BuildSettingsSkinPreviewCachePath(Key.m_SkinName.c_str(), Key.m_Version, Key.m_ContentHash.c_str());
	const size_t ExtensionPos = Path.rfind(".webp");
	if(ExtensionPos == std::string::npos)
		return Path;

	char aSize[32];
	str_format(aSize, sizeof(aSize), "--s%d--e%d--f%d", Key.m_SizeBucket, Key.m_Emote, Key.m_FatSkins);
	Path.insert(ExtensionPos, aSize);
	return Path;
}

std::string BuildSettingsSkinPreviewCachePath(const SSettingsSkinPreviewCacheKey &Key, ESettingsSkinPreviewCacheLayer Layer)
{
	std::string Path = BuildSettingsSkinPreviewCachePath(Key);
	const size_t ExtensionPos = Path.rfind(".webp");
	if(ExtensionPos == std::string::npos)
		return Path;

	char aLayer[64];
	str_format(aLayer, sizeof(aLayer), "--%s", SettingsSkinPreviewCacheLayerName(Layer));
	Path.insert(ExtensionPos, aLayer);
	return Path;
}

std::string BuildSettingsSkinPreviewCacheManifestPath(const SSettingsSkinPreviewCacheKey &Key)
{
	std::string Path = BuildSettingsSkinPreviewCachePath(Key);
	const size_t ExtensionPos = Path.rfind(".webp");
	if(ExtensionPos == std::string::npos)
		return Path + ".manifest";
	Path.replace(ExtensionPos, 5, ".manifest");
	return Path;
}

std::string BuildSettingsSkinPreviewCacheTemporaryPath(const std::string &Path)
{
	return Path + ".tmp";
}

SSettingsSkinPreviewCacheManifest BuildSettingsSkinPreviewCacheManifest(const SSettingsSkinPreviewCacheKey &Key)
{
	SSettingsSkinPreviewCacheManifest Manifest;
	Manifest.m_Key = Key;
	Manifest.m_Version = SETTINGS_SKIN_PREVIEW_CACHE_MANIFEST_VERSION;
	Manifest.m_LayerCount = NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS;
	return Manifest;
}

std::string SerializeSettingsSkinPreviewCacheManifest(const SSettingsSkinPreviewCacheManifest &Manifest)
{
	std::string Text;
	Text += "manifest_version=" + std::to_string(Manifest.m_Version) + "\n";
	Text += "skin=" + Manifest.m_Key.m_SkinName + "\n";
	Text += "cache_version=" + std::to_string(Manifest.m_Key.m_Version) + "\n";
	Text += "size_bucket=" + std::to_string(Manifest.m_Key.m_SizeBucket) + "\n";
	Text += "content_hash=" + Manifest.m_Key.m_ContentHash + "\n";
	Text += "emote=" + std::to_string(Manifest.m_Key.m_Emote) + "\n";
	Text += "fat_skins=" + std::to_string(Manifest.m_Key.m_FatSkins) + "\n";
	Text += "layers=" + std::to_string(Manifest.m_LayerCount) + "\n";
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const SSettingsSkinPreviewCacheManifestLayer &Layer = Manifest.m_aLayers[LayerIndex];
		Text += "layer=" + std::to_string(LayerIndex) + "\t" + SettingsSkinPreviewCacheLayerName((ESettingsSkinPreviewCacheLayer)LayerIndex) + "\t" +
			std::to_string(Layer.m_Width) + "\t" + std::to_string(Layer.m_Height) + "\t" + std::to_string(Layer.m_VisiblePixels) + "\t" +
			std::to_string(Layer.m_EncodedBytes) + "\t" + Layer.m_Path + "\n";
	}
	return Text;
}

namespace
{
	bool ParseIntValue(const std::string &Value, int &Out)
	{
		char *pEnd = nullptr;
		errno = 0;
		const long Parsed = std::strtol(Value.c_str(), &pEnd, 10);
		if(errno != 0 || pEnd == Value.c_str() || *pEnd != '\0')
			return false;
		if(Parsed < std::numeric_limits<int>::min() || Parsed > std::numeric_limits<int>::max())
			return false;
		Out = (int)Parsed;
		return true;
	}

	bool ParseUint64Value(const std::string &Value, uint64_t &Out)
	{
		char *pEnd = nullptr;
		errno = 0;
		const unsigned long long Parsed = std::strtoull(Value.c_str(), &pEnd, 10);
		if(errno != 0 || pEnd == Value.c_str() || *pEnd != '\0')
			return false;
		Out = (uint64_t)Parsed;
		return true;
	}

	bool SplitLayerManifestLine(const std::string &Line, int &LayerIndex, int &Width, int &Height, uint64_t &VisiblePixels, uint64_t &EncodedBytes, std::string &Path)
	{
		if(Line.rfind("layer=", 0) != 0)
			return false;
		const std::string Rest = Line.substr(6);
		size_t FieldStart = 0;
		std::array<std::string, 7> aFields;
		for(std::string &Field : aFields)
		{
			const size_t FieldEnd = Rest.find('\t', FieldStart);
			if(FieldEnd == std::string::npos)
			{
				Field = Rest.substr(FieldStart);
				FieldStart = Rest.size();
			}
			else
			{
				Field = Rest.substr(FieldStart, FieldEnd - FieldStart);
				FieldStart = FieldEnd + 1;
			}
		}
		if(FieldStart != Rest.size() || !ParseIntValue(aFields[0], LayerIndex))
			return false;
		if(LayerIndex < 0 || LayerIndex >= NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS)
			return false;
		if(aFields[1] != SettingsSkinPreviewCacheLayerName((ESettingsSkinPreviewCacheLayer)LayerIndex))
			return false;
		if(!ParseIntValue(aFields[2], Width) || !ParseIntValue(aFields[3], Height))
			return false;
		if(Width <= 0 || Height <= 0)
			return false;
		if(!ParseUint64Value(aFields[4], VisiblePixels) || !ParseUint64Value(aFields[5], EncodedBytes))
			return false;
		Path = aFields[6];
		return !Path.empty();
	}
}

std::optional<SSettingsSkinPreviewCacheManifest> ParseSettingsSkinPreviewCacheManifest(const std::string &Text)
{
	if(Text.empty())
		return std::nullopt;

	SSettingsSkinPreviewCacheManifest Manifest;
	std::array<bool, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> aLayerSeen = {};
	std::istringstream Stream(Text);
	std::string Line;
	while(std::getline(Stream, Line))
	{
		if(Line.empty())
			continue;
		if(Line.rfind("layer=", 0) == 0)
		{
			int LayerIndex = -1;
			int Width = 0;
			int Height = 0;
			uint64_t VisiblePixels = 0;
			uint64_t EncodedBytes = 0;
			std::string Path;
			if(!SplitLayerManifestLine(Line, LayerIndex, Width, Height, VisiblePixels, EncodedBytes, Path))
				return std::nullopt;
			if(aLayerSeen[LayerIndex])
				return std::nullopt;
			aLayerSeen[LayerIndex] = true;
			Manifest.m_aLayers[LayerIndex].m_Path = std::move(Path);
			Manifest.m_aLayers[LayerIndex].m_Width = Width;
			Manifest.m_aLayers[LayerIndex].m_Height = Height;
			Manifest.m_aLayers[LayerIndex].m_VisiblePixels = VisiblePixels;
			Manifest.m_aLayers[LayerIndex].m_EncodedBytes = EncodedBytes;
			continue;
		}

		const size_t Separator = Line.find('=');
		if(Separator == std::string::npos)
			return std::nullopt;
		const std::string Key = Line.substr(0, Separator);
		const std::string Value = Line.substr(Separator + 1);
		if(Key == "manifest_version")
		{
			if(!ParseIntValue(Value, Manifest.m_Version))
				return std::nullopt;
		}
		else if(Key == "skin")
		{
			Manifest.m_Key.m_SkinName = Value;
		}
		else if(Key == "cache_version")
		{
			if(!ParseIntValue(Value, Manifest.m_Key.m_Version))
				return std::nullopt;
		}
		else if(Key == "size_bucket")
		{
			if(!ParseIntValue(Value, Manifest.m_Key.m_SizeBucket))
				return std::nullopt;
		}
		else if(Key == "content_hash")
		{
			Manifest.m_Key.m_ContentHash = Value;
		}
		else if(Key == "emote")
		{
			if(!ParseIntValue(Value, Manifest.m_Key.m_Emote))
				return std::nullopt;
		}
		else if(Key == "fat_skins")
		{
			if(!ParseIntValue(Value, Manifest.m_Key.m_FatSkins))
				return std::nullopt;
		}
		else if(Key == "layers")
		{
			if(!ParseIntValue(Value, Manifest.m_LayerCount))
				return std::nullopt;
		}
		else
		{
			return std::nullopt;
		}
	}

	for(bool LayerSeen : aLayerSeen)
	{
		if(!LayerSeen)
			return std::nullopt;
	}
	return Manifest;
}

bool SettingsSkinPreviewCacheManifestAllowsLoad(const std::optional<SSettingsSkinPreviewCacheManifest> &Manifest, const SSettingsSkinPreviewCacheKey &Key)
{
	return Manifest.has_value() && SettingsSkinPreviewCacheManifestAllowsLoad(*Manifest, Key);
}

bool SettingsSkinPreviewCacheManifestAllowsLoad(const SSettingsSkinPreviewCacheManifest &Manifest, const SSettingsSkinPreviewCacheKey &Key)
{
	if(Manifest.m_Version != SETTINGS_SKIN_PREVIEW_CACHE_MANIFEST_VERSION || Manifest.m_LayerCount != NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS || !(Manifest.m_Key == Key))
		return false;

	uint64_t VisiblePixels = 0;
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const SSettingsSkinPreviewCacheManifestLayer &Layer = Manifest.m_aLayers[LayerIndex];
		if(Layer.m_Path != BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex))
			return false;
		if(Layer.m_Width <= 0 || Layer.m_Height <= 0)
			return false;
		if(Layer.m_EncodedBytes == 0)
			return false;
		VisiblePixels += Layer.m_VisiblePixels;
	}
	return VisiblePixels > 0;
}

SSettingsSkinPreviewCacheAtomicPublishPlan BuildSettingsSkinPreviewCacheAtomicPublishPlan(const SSettingsSkinPreviewCacheKey &Key)
{
	SSettingsSkinPreviewCacheAtomicPublishPlan Plan;
	Plan.m_vSteps.reserve(NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS + 1);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const std::string FinalPath = BuildSettingsSkinPreviewCachePath(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Plan.m_vSteps.push_back({BuildSettingsSkinPreviewCacheTemporaryPath(FinalPath), FinalPath});
	}
	const std::string ManifestPath = BuildSettingsSkinPreviewCacheManifestPath(Key);
	Plan.m_vSteps.push_back({BuildSettingsSkinPreviewCacheTemporaryPath(ManifestPath), ManifestPath});
	return Plan;
}

std::vector<std::string> ComputeSettingsSkinPreviewCachePruneList(std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries, int MaxGroups)
{
	if(MaxGroups < 0)
		MaxGroups = 0;
	if(vEntries.empty())
		return {};

	struct SPreviewCachePruneGroup
	{
		int64_t m_ModifiedTime = std::numeric_limits<int64_t>::max();
		std::vector<std::string> m_vPaths;
	};

	auto GroupPath = [](const std::string &Path) {
		const std::string ManifestSuffix = ".manifest";
		if(Path.size() >= ManifestSuffix.size() && Path.compare(Path.size() - ManifestSuffix.size(), ManifestSuffix.size(), ManifestSuffix) == 0)
			return Path.substr(0, Path.size() - ManifestSuffix.size());
		for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
		{
			std::string Suffix = std::string("--") + SettingsSkinPreviewCacheLayerName((ESettingsSkinPreviewCacheLayer)LayerIndex) + ".webp";
			if(Path.size() >= Suffix.size() && Path.compare(Path.size() - Suffix.size(), Suffix.size(), Suffix) == 0)
				return Path.substr(0, Path.size() - Suffix.size());
		}
		return Path;
	};

	std::unordered_map<std::string, SPreviewCachePruneGroup> GroupsByPath;
	for(const SSettingsSkinPreviewCacheFileEntry &Entry : vEntries)
	{
		SPreviewCachePruneGroup &Group = GroupsByPath[GroupPath(Entry.m_Path)];
		Group.m_ModifiedTime = minimum(Group.m_ModifiedTime, Entry.m_ModifiedTime);
		Group.m_vPaths.push_back(Entry.m_Path);
	}

	std::vector<SPreviewCachePruneGroup> vGroups;
	vGroups.reserve(GroupsByPath.size());
	for(auto &[Path, Group] : GroupsByPath)
		vGroups.push_back(std::move(Group));
	if((int)vGroups.size() <= MaxGroups)
		return {};

	std::sort(vGroups.begin(), vGroups.end(), [](const auto &A, const auto &B) {
		return A.m_ModifiedTime < B.m_ModifiedTime;
	});

	std::vector<std::string> vDelete;
	vDelete.reserve(vEntries.size());
	size_t RemainingGroups = vGroups.size();
	for(const SPreviewCachePruneGroup &Group : vGroups)
	{
		if(RemainingGroups <= (size_t)MaxGroups)
			break;
		for(const std::string &Path : Group.m_vPaths)
			vDelete.push_back(Path);
		--RemainingGroups;
	}
	return vDelete;
}

std::vector<int> ComputeSettingsSkinPreviewCacheMemoryEvictionList(const std::vector<SSettingsSkinPreviewMemoryCacheEntry> &vEntries, size_t MaxEntries, size_t MaxBytes)
{
	size_t TotalBytes = 0;
	for(const auto &Entry : vEntries)
		TotalBytes += Entry.m_ApproxBytes;
	if(vEntries.size() <= MaxEntries && TotalBytes <= MaxBytes)
		return {};

	std::vector<int> vOrder(vEntries.size());
	for(size_t Index = 0; Index < vEntries.size(); ++Index)
		vOrder[Index] = (int)Index;
	std::stable_sort(vOrder.begin(), vOrder.end(), [&](int Left, int Right) {
		return vEntries[Left].m_LastUseSequence < vEntries[Right].m_LastUseSequence;
	});

	std::vector<int> vEvict;
	size_t RemainingEntries = vEntries.size();
	size_t RemainingBytes = TotalBytes;
	for(int Index : vOrder)
	{
		if(RemainingEntries <= MaxEntries && RemainingBytes <= MaxBytes)
			break;
		vEvict.push_back(Index);
		RemainingEntries--;
		RemainingBytes -= vEntries[Index].m_ApproxBytes;
	}
	return vEvict;
}

namespace
{
	void UnloadPreviewCacheTextures(IGraphics *pGraphics, SSettingsSkinPreviewCacheTextures &Textures)
	{
		if(pGraphics == nullptr)
			return;
		for(IGraphics::CTextureHandle &Texture : Textures.m_aTextures)
		{
			if(Texture.IsValid())
				pGraphics->UnloadTexture(&Texture);
		}
	}

	int CollectPreviewCacheFileInfo(const CFsFileInfo *pInfo, int IsDir, int DirType, void *pUser)
	{
		(void)DirType;
		if(IsDir != 0 || pInfo == nullptr || pInfo->m_pName == nullptr)
			return 0;
		if(str_endswith(pInfo->m_pName, ".webp") == nullptr && str_endswith(pInfo->m_pName, ".manifest") == nullptr)
			return 0;
		auto *pEntries = static_cast<std::vector<SSettingsSkinPreviewCacheFileEntry> *>(pUser);
		pEntries->push_back({std::string("qmclient/skins/preview_cache/") + pInfo->m_pName, (int64_t)pInfo->m_TimeModified});
		return 0;
	}
}

void CSettingsSkinPreviewCache::Init(IStorage *pStorage, IGraphics *pGraphics)
{
	m_pStorage = pStorage;
	m_pGraphics = pGraphics;
	EnsureCacheFolders();
}

void CSettingsSkinPreviewCache::Shutdown()
{
	ClearTextures();
}

void CSettingsSkinPreviewCache::OnSkinDirectoryChanged()
{
	Shutdown();
	EnsureCacheFolders();
}

bool CSettingsSkinPreviewCache::EnsureCacheFolders()
{
	if(m_pStorage == nullptr)
		return false;
	const bool QmFolderOk = m_pStorage->FolderExists("qmclient", IStorage::TYPE_SAVE) || m_pStorage->CreateFolder("qmclient", IStorage::TYPE_SAVE);
	const bool SkinsFolderOk = m_pStorage->FolderExists("qmclient/skins", IStorage::TYPE_SAVE) || m_pStorage->CreateFolder("qmclient/skins", IStorage::TYPE_SAVE);
	const bool CacheFolderOk = m_pStorage->FolderExists("qmclient/skins/preview_cache", IStorage::TYPE_SAVE) || m_pStorage->CreateFolder("qmclient/skins/preview_cache", IStorage::TYPE_SAVE);
	return QmFolderOk && SkinsFolderOk && CacheFolderOk;
}

void CSettingsSkinPreviewCache::PruneDiskCache()
{
	if(m_pStorage == nullptr || !EnsureCacheFolders())
		return;
	std::vector<SSettingsSkinPreviewCacheFileEntry> vEntries;
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, "qmclient/skins/preview_cache", CollectPreviewCacheFileInfo, &vEntries);
	for(const std::string &Path : ComputeSettingsSkinPreviewCachePruneList(std::move(vEntries), SETTINGS_SKIN_PREVIEW_CACHE_KEY_LIMIT))
		m_pStorage->RemoveFile(Path.c_str(), IStorage::TYPE_SAVE);
}

std::string CSettingsSkinPreviewCache::PathForKey(const SSettingsSkinPreviewCacheKey &Key) const
{
	return BuildSettingsSkinPreviewCachePath(Key);
}

std::string CSettingsSkinPreviewCache::PathForKey(const SSettingsSkinPreviewCacheKey &Key, ESettingsSkinPreviewCacheLayer Layer) const
{
	return BuildSettingsSkinPreviewCachePath(Key, Layer);
}

std::string CSettingsSkinPreviewCache::ManifestPathForKey(const SSettingsSkinPreviewCacheKey &Key) const
{
	return BuildSettingsSkinPreviewCacheManifestPath(Key);
}

void CSettingsSkinPreviewCache::RemoveDiskCache(const SSettingsSkinPreviewCacheKey &Key)
{
	if(m_pStorage == nullptr)
		return;
	m_pStorage->RemoveFile(ManifestPathForKey(Key).c_str(), IStorage::TYPE_SAVE);
	m_pStorage->RemoveFile(BuildSettingsSkinPreviewCacheTemporaryPath(ManifestPathForKey(Key)).c_str(), IStorage::TYPE_SAVE);
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		m_pStorage->RemoveFile(BuildSettingsSkinPreviewCacheTemporaryPath(PathForKey(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex)).c_str(), IStorage::TYPE_SAVE);
		m_pStorage->RemoveFile(PathForKey(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex).c_str(), IStorage::TYPE_SAVE);
	}
}

bool CSettingsSkinPreviewCache::WriteManifestAtomically(const SSettingsSkinPreviewCacheManifest &Manifest)
{
	if(m_pStorage == nullptr || !SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Manifest.m_Key) || !EnsureCacheFolders())
		return false;

	const std::string ManifestPath = BuildSettingsSkinPreviewCacheManifestPath(Manifest.m_Key);
	const std::string TemporaryPath = BuildSettingsSkinPreviewCacheTemporaryPath(ManifestPath);
	const std::string Text = SerializeSettingsSkinPreviewCacheManifest(Manifest);

	IOHANDLE File = m_pStorage->OpenFile(TemporaryPath.c_str(), IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return false;
	const unsigned Written = io_write(File, Text.data(), (unsigned)Text.size());
	io_close(File);
	if(Written != Text.size())
	{
		m_pStorage->RemoveFile(TemporaryPath.c_str(), IStorage::TYPE_SAVE);
		return false;
	}

	m_pStorage->RemoveFile(ManifestPath.c_str(), IStorage::TYPE_SAVE);
	if(!m_pStorage->RenameFile(TemporaryPath.c_str(), ManifestPath.c_str(), IStorage::TYPE_SAVE))
	{
		m_pStorage->RemoveFile(TemporaryPath.c_str(), IStorage::TYPE_SAVE);
		return false;
	}
	return true;
}

bool CSettingsSkinPreviewCache::PublishDiskCacheArtifactsAtomically(const SSettingsSkinPreviewCacheKey &Key, const std::array<std::string, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aTemporaryLayerPaths, const SSettingsSkinPreviewCacheManifest &Manifest)
{
	if(m_pStorage == nullptr || !SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key) || !EnsureCacheFolders())
		return false;

	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const std::string FinalPath = PathForKey(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		if(aTemporaryLayerPaths[LayerIndex].empty() || aTemporaryLayerPaths[LayerIndex] == FinalPath)
		{
			RemoveDiskCache(Key);
			return false;
		}
		m_pStorage->RemoveFile(FinalPath.c_str(), IStorage::TYPE_SAVE);
		if(!m_pStorage->RenameFile(aTemporaryLayerPaths[LayerIndex].c_str(), FinalPath.c_str(), IStorage::TYPE_SAVE))
		{
			RemoveDiskCache(Key);
			return false;
		}
	}

	if(!WriteManifestAtomically(Manifest))
	{
		RemoveDiskCache(Key);
		return false;
	}
	return true;
}

std::optional<SSettingsSkinPreviewCacheTextures> CSettingsSkinPreviewCache::FindTextures(const SSettingsSkinPreviewCacheKey &Key)
{
	const auto It = m_vTextures.find(Key);
	if(It == m_vTextures.end() || !It->second.m_Textures.IsComplete())
		return std::nullopt;
	It->second.m_LastUseSequence = m_NextUseSequence++;
	return It->second.m_Textures;
}

bool CSettingsSkinPreviewCache::DiskCacheExists(const SSettingsSkinPreviewCacheKey &Key) const
{
	return m_pStorage != nullptr && m_pStorage->FileExists(ManifestPathForKey(Key).c_str(), IStorage::TYPE_SAVE);
}

bool CSettingsSkinPreviewCache::LoadLayerImagesFromDisk(const SSettingsSkinPreviewCacheKey &Key, std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> &aImages, SSettingsSkinPreviewCacheManifest *pManifest)
{
	if(m_pStorage == nullptr)
		return false;

	const std::string ManifestPath = ManifestPathForKey(Key);
	if(!m_pStorage->FileExists(ManifestPath.c_str(), IStorage::TYPE_SAVE))
	{
		RemoveDiskCache(Key);
		return false;
	}

	char *pManifestText = m_pStorage->ReadFileStr(ManifestPath.c_str(), IStorage::TYPE_SAVE);
	const std::optional<SSettingsSkinPreviewCacheManifest> Manifest = pManifestText ? ParseSettingsSkinPreviewCacheManifest(pManifestText) : std::nullopt;
	free(pManifestText);
	if(!SettingsSkinPreviewCacheManifestAllowsLoad(Manifest, Key))
	{
		RemoveDiskCache(Key);
		return false;
	}

	auto CleanupImages = [&]() {
		for(CImageInfo &Image : aImages)
			Image.Free();
	};
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const ESettingsSkinPreviewCacheLayer Layer = (ESettingsSkinPreviewCacheLayer)LayerIndex;
		const std::string Path = PathForKey(Key, Layer);
		if(!m_pStorage->FileExists(Path.c_str(), IStorage::TYPE_SAVE))
		{
			RemoveDiskCache(Key);
			CleanupImages();
			return false;
		}

		IOHANDLE File = m_pStorage->OpenFile(Path.c_str(), IOFLAG_READ, IStorage::TYPE_SAVE);
		const int64_t EncodedBytes = File ? io_length(File) : -1;
		if(EncodedBytes < 0 || (uint64_t)EncodedBytes != Manifest->m_aLayers[LayerIndex].m_EncodedBytes)
		{
			if(File)
				io_close(File);
			RemoveDiskCache(Key);
			CleanupImages();
			return false;
		}

		if(!CImageLoader::LoadWebP(File, Path.c_str(), aImages[LayerIndex]))
		{
			RemoveDiskCache(Key);
			CleanupImages();
			return false;
		}
		if(SettingsSkinPreviewCacheVisiblePixelCount(aImages[LayerIndex]) != Manifest->m_aLayers[LayerIndex].m_VisiblePixels)
		{
			RemoveDiskCache(Key);
			CleanupImages();
			return false;
		}
		if(aImages[LayerIndex].m_Width != Manifest->m_aLayers[LayerIndex].m_Width || aImages[LayerIndex].m_Height != Manifest->m_aLayers[LayerIndex].m_Height)
		{
			RemoveDiskCache(Key);
			CleanupImages();
			return false;
		}
	}

	if(!SettingsSkinPreviewCacheImagesHaveVisiblePixels(aImages))
	{
		RemoveDiskCache(Key);
		CleanupImages();
		return false;
	}

	if(pManifest != nullptr)
		*pManifest = *Manifest;
	return true;
}

std::optional<SSettingsSkinPreviewCacheTextures> CSettingsSkinPreviewCache::LoadTexturesFromDisk(const SSettingsSkinPreviewCacheKey &Key)
{
	if(m_pStorage == nullptr || m_pGraphics == nullptr)
		return std::nullopt;

	std::array<CImageInfo, NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS> aImages;
	auto CleanupImages = [&]() {
		for(CImageInfo &Image : aImages)
			Image.Free();
	};
	if(!LoadLayerImagesFromDisk(Key, aImages))
		return std::nullopt;

	SSettingsSkinPreviewCacheTextures Textures;
	for(int LayerIndex = 0; LayerIndex < NUM_SETTINGS_SKIN_PREVIEW_CACHE_LAYERS; ++LayerIndex)
	{
		const std::string Path = PathForKey(Key, (ESettingsSkinPreviewCacheLayer)LayerIndex);
		Textures.m_ApproxBytes += (size_t)aImages[LayerIndex].m_Width * (size_t)aImages[LayerIndex].m_Height * 4u;
		Textures.m_aTextures[LayerIndex] = m_pGraphics->LoadTextureRawMove(aImages[LayerIndex], 0, Path.c_str());
		if(!Textures.m_aTextures[LayerIndex].IsValid())
		{
			log_error("settings/skins", "failed to upload preview cache texture '%s'", Path.c_str());
			RemoveDiskCache(Key);
			UnloadPreviewCacheTextures(m_pGraphics, Textures);
			CleanupImages();
			return std::nullopt;
		}
	}

	RememberTextures(Key, Textures);
	return Textures;
}

void CSettingsSkinPreviewCache::RememberTextures(const SSettingsSkinPreviewCacheKey &Key, SSettingsSkinPreviewCacheTextures Textures)
{
	const auto It = m_vTextures.find(Key);
	if(It != m_vTextures.end())
	{
		SSettingsSkinPreviewCacheTextures OldTextures = It->second.m_Textures;
		UnloadPreviewCacheTextures(m_pGraphics, OldTextures);
		m_MemoryCacheBytes -= It->second.m_Textures.m_ApproxBytes;
	}
	SMemoryCacheEntry Entry;
	Entry.m_Textures = std::move(Textures);
	Entry.m_LastUseSequence = m_NextUseSequence++;
	m_MemoryCacheBytes += Entry.m_Textures.m_ApproxBytes;
	m_vTextures[Key] = std::move(Entry);
	EvictMemoryCacheIfNeeded();
}

void CSettingsSkinPreviewCache::ForgetTexture(const SSettingsSkinPreviewCacheKey &Key)
{
	const auto It = m_vTextures.find(Key);
	if(It == m_vTextures.end())
		return;
	if(m_pGraphics)
		UnloadPreviewCacheTextures(m_pGraphics, It->second.m_Textures);
	m_MemoryCacheBytes -= It->second.m_Textures.m_ApproxBytes;
	m_vTextures.erase(It);
}

void CSettingsSkinPreviewCache::ClearMemoryCache()
{
	ClearTextures();
}

size_t CSettingsSkinPreviewCache::MemoryCacheSize() const
{
	return m_vTextures.size();
}

size_t CSettingsSkinPreviewCache::MemoryCacheBytes() const
{
	return m_MemoryCacheBytes;
}

void CSettingsSkinPreviewCache::ClearTextures()
{
	if(m_pGraphics)
	{
		for(auto &[Key, Entry] : m_vTextures)
		{
			UnloadPreviewCacheTextures(m_pGraphics, Entry.m_Textures);
		}
	}
	m_vTextures.clear();
	m_MemoryCacheBytes = 0;
}

void CSettingsSkinPreviewCache::EvictMemoryCacheIfNeeded()
{
	if(m_vTextures.size() <= SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_ENTRIES &&
		m_MemoryCacheBytes <= SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_BYTES)
	{
		return;
	}

	std::vector<SSettingsSkinPreviewMemoryCacheEntry> vEntries;
	std::vector<SSettingsSkinPreviewCacheKey> vKeys;
	vEntries.reserve(m_vTextures.size());
	vKeys.reserve(m_vTextures.size());
	for(const auto &[Key, Entry] : m_vTextures)
	{
		vKeys.push_back(Key);
		vEntries.push_back({Entry.m_Textures.m_ApproxBytes, Entry.m_LastUseSequence});
	}

	const auto vEvict = ComputeSettingsSkinPreviewCacheMemoryEvictionList(vEntries, SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_ENTRIES, SETTINGS_SKIN_PREVIEW_MEMORY_CACHE_MAX_BYTES);
	for(int Index : vEvict)
	{
		if(Index < 0 || (size_t)Index >= vKeys.size())
			continue;
		ForgetTexture(vKeys[Index]);
	}
}
