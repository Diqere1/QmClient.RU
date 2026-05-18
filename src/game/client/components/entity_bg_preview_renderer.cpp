#include "entity_bg_preview_renderer.h"

#include <base/logger.h>
#include <base/system.h>

#include <engine/gfx/image_loader.h>
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/shared/map.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/layers.h>
#include <game/map/map_preview_common.h>
#include <game/map/map_renderer.h>

#include <memory>
#include <optional>

namespace
{
	static bool LoadPreparedMap(
		IGraphics *pGraphics,
		ITextRender *pTextRender,
		IStorage *pStorage,
		const char *pMapPath,
		CMap &PreviewMap,
		CLayers &Layers,
		MapPreview::CPreviewMapImagesBase &MapImages,
		MapPreview::CPreviewEnvelopeEvalBase &EnvelopeEval,
		CRenderMap &RenderCore,
		CMapRenderer &RenderMap)
	{
		if(pGraphics == nullptr || pTextRender == nullptr || pStorage == nullptr || pMapPath == nullptr || pMapPath[0] == '\0')
			return false;

		IEngineMap *pMap = &PreviewMap;
		if(!pMap->Load(pMapPath))
			return false;

		Layers.Init(pMap, false);
		MapImages.Init(pGraphics, pMap, &Layers);
		EnvelopeEval.Init(pMap);
		RenderCore.Init(pGraphics, pTextRender);
		RenderMap.OnInit(pGraphics, pTextRender, &RenderCore);
		RenderMap.Load(RENDERTYPE_FULL_DESIGN, &Layers, &MapImages, &EnvelopeEval, std::nullopt);
		return true;
	}
} // namespace

bool EntityBgPreview::PrepareMapPreview(
	IStorage *pStorage,
	const char *pMapPath,
	SPreparedMapPreview &OutputPreview)
{
	OutputPreview = SPreparedMapPreview();
	if(pStorage == nullptr || pMapPath == nullptr || pMapPath[0] == '\0')
		return false;

	CMap PreviewMap;
	IEngineMap *pMap = &PreviewMap;
	if(!pMap->Load(pMapPath))
		return false;

	CLayers Layers;
	Layers.Init(pMap, false);

	MapPreview::CPreviewEnvelopeEvalBase EnvelopeEval;
	EnvelopeEval.Init(pMap);

	constexpr int PreviewWidth = 1920;
	constexpr int PreviewHeight = 1080;
	const float PreviewAspect = (float)PreviewWidth / (float)PreviewHeight;
	const auto [Center, Zoom] = MapPreview::CalcCamera(Layers, PreviewAspect, &EnvelopeEval);

	str_copy(OutputPreview.m_aMapPath, pMapPath, sizeof(OutputPreview.m_aMapPath));
	OutputPreview.m_Center = Center;
	OutputPreview.m_Zoom = Zoom;
	OutputPreview.m_Valid = true;
	return true;
}

bool EntityBgPreview::MapPreviewUsesExternalResources(const char *pMapPath)
{
	if(pMapPath == nullptr || pMapPath[0] == '\0')
		return false;

	CMap PreviewMap;
	IEngineMap *pMap = &PreviewMap;
	if(!pMap->Load(pMapPath))
		return false;

	return MapPreview::MapUsesExternalResources(pMap);
}

bool EntityBgPreview::RenderPreparedMapPreview(
	IGraphics *pGraphics,
	ITextRender *pTextRender,
	IStorage *pStorage,
	const SPreparedMapPreview &PreparedPreview,
	CImageInfo &OutputImage)
{
	if(!PreparedPreview.m_Valid || PreparedPreview.m_aMapPath[0] == '\0' || pGraphics == nullptr || pTextRender == nullptr || pStorage == nullptr)
		return false;

	OutputImage.Free();
	if(!pGraphics->SupportsOffscreenMapPreview())
		return false;

	CMap PreviewMap;
	CLayers Layers;
	MapPreview::CPreviewMapImagesBase MapImages;
	MapPreview::CPreviewEnvelopeEvalBase EnvelopeEval;
	CRenderMap RenderCore;
	CMapRenderer RenderMap;
	if(!LoadPreparedMap(pGraphics, pTextRender, pStorage, PreparedPreview.m_aMapPath, PreviewMap, Layers, MapImages, EnvelopeEval, RenderCore, RenderMap))
		return false;

	CRenderLayerParams Params{};
	Params.m_RenderType = RENDERTYPE_FULL_DESIGN;
	Params.m_EntityOverlayVal = 0;
	Params.m_Center = PreparedPreview.m_Center;
	Params.m_Zoom = PreparedPreview.m_Zoom;
	Params.m_RenderText = false;
	Params.m_RenderInvalidTiles = false;
	Params.m_TileAndQuadBuffering = true;
	Params.m_RenderTileBorder = false;

	constexpr int PreviewWidth = 1920;
	constexpr int PreviewHeight = 1080;
	const bool Rendered = pGraphics->RenderOffscreenMapPreview(PreviewWidth, PreviewHeight, [&] {
		pGraphics->MapScreenToInterface(PreparedPreview.m_Center.x, PreparedPreview.m_Center.y, PreparedPreview.m_Zoom);
		pGraphics->Clear(0.0f, 0.0f, 0.0f, true);
		RenderMap.Render(Params); }, OutputImage);
	if(!Rendered)
		return false;

	MapPreview::CropClearColorBorder(OutputImage);
	return OutputImage.m_pData != nullptr;
}

bool EntityBgPreview::RenderMapPreview(
	IGraphics *pGraphics,
	ITextRender *pTextRender,
	IStorage *pStorage,
	const char *pMapPath,
	CImageInfo &OutputImage)
{
	SPreparedMapPreview PreparedPreview;
	if(!PrepareMapPreview(pStorage, pMapPath, PreparedPreview))
		return false;
	return RenderPreparedMapPreview(pGraphics, pTextRender, pStorage, PreparedPreview, OutputImage);
}
