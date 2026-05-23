#include "entity_bg_preview_renderer.h"

#include <base/system.h>

#include <engine/map.h>
#include <engine/shared/map.h>

#include <game/layers.h>
#include <game/map/map_preview_common.h>

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
	return false;
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
