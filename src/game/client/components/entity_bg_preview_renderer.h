#ifndef GAME_CLIENT_COMPONENTS_ENTITY_BG_PREVIEW_RENDERER_H
#define GAME_CLIENT_COMPONENTS_ENTITY_BG_PREVIEW_RENDERER_H

#include <base/system.h>
#include <base/vmath.h>

#include <engine/image.h>

class IGraphics;
class ITextRender;
class IStorage;
class CLayers;
class IMap;
class CMapBasedEnvelopePointAccess;

namespace EntityBgPreview
{
	struct SPreparedMapPreview
	{
		char m_aMapPath[IO_MAX_PATH_LENGTH] = "";
		vec2 m_Center = vec2(0.0f, 0.0f);
		float m_Zoom = 1.0f;
		bool m_Valid = false;
	};

	bool PrepareMapPreview(
		IStorage *pStorage,
		const char *pMapPath,
		SPreparedMapPreview &OutputPreview);

	bool MapPreviewUsesExternalResources(const char *pMapPath);

	bool RenderPreparedMapPreview(
		IGraphics *pGraphics,
		ITextRender *pTextRender,
		IStorage *pStorage,
		const SPreparedMapPreview &PreparedPreview,
		CImageInfo &OutputImage);

	bool RenderMapPreview(
		IGraphics *pGraphics,
		ITextRender *pTextRender,
		IStorage *pStorage,
		const char *pMapPath,
		CImageInfo &OutputImage);
}

#endif
