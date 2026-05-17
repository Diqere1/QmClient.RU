#include <base/logger.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/engine.h>
#include <engine/gfx/image_loader.h>
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/layers.h>
#include <game/map/map_preview_common.h>
#include <game/map/map_renderer.h>

#include <memory>
#include <optional>

namespace
{
	class CMapPreviewTool
	{
		std::unique_ptr<IStorage> m_pStorage;
		IEngine *m_pEngine = nullptr;
		IEngineGraphics *m_pGraphics = nullptr;
		IEngineTextRender *m_pTextRender = nullptr;
		IEngineMap *m_pMap = nullptr;
		IConsole *m_pConsole = nullptr;
		IConfigManager *m_pConfigManager = nullptr;
		CLayers m_Layers;
		MapPreview::CPreviewMapImagesBase m_MapImages;
		CRenderMap m_RenderCore;
		CMapRenderer m_RenderMap;
		MapPreview::CPreviewEnvelopeEvalBase m_EnvelopeEval;
		char m_aSource[IO_MAX_PATH_LENGTH] = {};
		char m_aOutput[IO_MAX_PATH_LENGTH] = {};
		std::optional<CImageInfo> m_OutputImage;
		static constexpr std::chrono::nanoseconds PREVIEW_TIME = std::chrono::nanoseconds(0);

		bool SaveOutputPng(const CImageInfo &Image)
		{
			if(fs_makedir_rec_for(m_aOutput) < 0)
			{
				log_error("map_preview", "Failed to create output directory for '%s'", m_aOutput);
				return false;
			}

			IOHANDLE File = io_open(m_aOutput, IOFLAG_WRITE);
			if(!File)
			{
				log_error("map_preview", "Failed to open output '%s' for writing", m_aOutput);
				return false;
			}

			if(!CImageLoader::SavePng(File, m_aOutput, Image))
			{
				io_close(File);
				log_error("map_preview", "Failed to write png '%s'", m_aOutput);
				return false;
			}

			io_close(File);
			return true;
		}

		static void DeriveOutputPathFromSource(const char *pSource, char *pOut, int OutSize)
		{
			const char *pName = pSource;
			const char *pSlash = str_rchr(pSource, '/');
			const char *pBackslash = str_rchr(pSource, '\\');
			if(pSlash && pBackslash)
				pName = (pSlash > pBackslash ? pSlash : pBackslash) + 1;
			else if(pSlash)
				pName = pSlash + 1;
			else if(pBackslash)
				pName = pBackslash + 1;

			char aBaseName[IO_MAX_PATH_LENGTH];
			str_copy(aBaseName, pName, sizeof(aBaseName));
			const char *pExt = str_rchr(aBaseName, '.');
			if(pExt != nullptr)
				aBaseName[pExt - aBaseName] = '\0';

			str_format(pOut, OutSize, "%s.png", aBaseName);
		}

	public:
		bool Init(int argc, const char **argv)
		{
			if(argc != 3)
			{
				log_error("map_preview", "Usage: %s <map> <output.png>", argv[0]);
				return false;
			}

			str_copy(m_aSource, argv[1], sizeof(m_aSource));
			if(argv[2] == nullptr || argv[2][0] == '\0' || str_comp(argv[2], "-") == 0)
				DeriveOutputPathFromSource(argv[1], m_aOutput, sizeof(m_aOutput));
			else
				str_copy(m_aOutput, argv[2], sizeof(m_aOutput));

			IKernel *pKernel = IKernel::Create();
			m_pStorage = std::unique_ptr<IStorage>(CreateStorage(IStorage::EInitializationType::BASIC, argc, argv));
			if(!m_pStorage)
				return false;
			pKernel->RegisterInterface(m_pStorage.get(), false);

			m_pEngine = CreateTestEngine("map_preview");
			pKernel->RegisterInterface(m_pEngine);

			m_pConsole = CreateConsole(CFGFLAG_CLIENT | CFGFLAG_STORE).release();
			pKernel->RegisterInterface(m_pConsole);

			m_pConfigManager = CreateConfigManager();
			pKernel->RegisterInterface(m_pConfigManager);

			m_pGraphics = CreateEngineGraphicsThreaded();
			pKernel->RegisterInterface(m_pGraphics);
			pKernel->RegisterInterface(static_cast<IGraphics *>(m_pGraphics), false);

			m_pTextRender = CreateEngineTextRender();
			pKernel->RegisterInterface(m_pTextRender);
			pKernel->RegisterInterface(static_cast<ITextRender *>(m_pTextRender), false);

			m_pMap = CreateEngineMap();
			pKernel->RegisterInterface(m_pMap);
			pKernel->RegisterInterface(static_cast<IMap *>(m_pMap), false);

			str_copy(g_Config.m_GfxBackend, "OpenGL");
			g_Config.m_GfxGLMajor = 3;
			g_Config.m_GfxGLMinor = 3;
			g_Config.m_GfxGLPatch = 0;
			g_Config.m_GfxFullscreen = 0;
			g_Config.m_GfxBorderless = 0;
			g_Config.m_GfxScreen = 0;
			g_Config.m_GfxVsync = 0;
			g_Config.m_GfxFsaaSamples = 0;
			g_Config.m_ClVideoShowDirection = 0;

			if(m_pGraphics->Init() != 0)
				return false;
			m_pTextRender->Init();
			m_pEngine->Init();
			m_pConsole->Init();
			m_pConfigManager->Init();
			return true;
		}

		bool Export()
		{
			if(!m_pMap->Load(m_aSource))
			{
				log_error("map_preview", "Failed to load map '%s'", m_aSource);
				return false;
			}

			m_Layers.Init(m_pMap, false);
			dbg_msg("map_preview", "map stats: groups=%d layers=%d", m_Layers.NumGroups(), m_Layers.NumLayers());
			m_EnvelopeEval.Init(m_pMap, PREVIEW_TIME);
			m_MapImages.Init(m_pGraphics, m_pMap, &m_Layers);
			const auto &Stats = m_MapImages.LoadStats();
			dbg_msg("map_preview", "loaded textures=%d", Stats.m_Loaded);
			if(Stats.m_MissingExternal > 0)
			{
				log_warn(
					"map_preview",
					"Map references %d external images and %d could not be loaded. Preview may differ from the editor/game unless matching files exist under data/mapres/.",
					Stats.m_ExpectedExternal, Stats.m_MissingExternal);
			}
			m_RenderCore.Init(m_pGraphics, m_pTextRender);
			m_RenderMap.OnInit(m_pGraphics, m_pTextRender, &m_RenderCore);
			m_RenderMap.Load(RENDERTYPE_FULL_DESIGN, &m_Layers, &m_MapImages, &m_EnvelopeEval, std::nullopt);

			m_pGraphics->Resize(1920, 1080, 60);

			const float Aspect = m_pGraphics->ScreenAspect();
			const auto [Center, Zoom] = MapPreview::CalcCamera(m_Layers, Aspect, &m_EnvelopeEval);
			m_pGraphics->MapScreenToInterface(Center.x, Center.y, Zoom);

			CRenderLayerParams Params{};
			Params.m_RenderType = RENDERTYPE_FULL_DESIGN;
			Params.m_EntityOverlayVal = 0;
			Params.m_Center = Center;
			Params.m_Zoom = Zoom;
			Params.m_RenderText = false;
			Params.m_RenderInvalidTiles = false;
			Params.m_TileAndQuadBuffering = true;
			Params.m_RenderTileBorder = false;
			Params.m_DebugRenderGroupClips = false;
			Params.m_DebugRenderQuadClips = false;
			Params.m_DebugRenderClusterClips = false;
			Params.m_DebugRenderTileClips = false;

			m_pGraphics->Clear(0.0f, 0.0f, 0.0f, true);
			m_RenderMap.Render(Params);
			m_pGraphics->TakeCustomScreenshot(m_aOutput, [this](CImageInfo &&Image) {
				m_OutputImage = std::move(Image);
			});
			m_pGraphics->Swap();
			m_pGraphics->WaitForIdle();
			if(!m_OutputImage.has_value())
			{
				log_error("map_preview", "Failed to capture rendered image");
				return false;
			}
			MapPreview::CropClearColorBorder(m_OutputImage.value());
			dbg_msg("map_preview", "camera center %.1f %.1f zoom %.3f", Center.x, Center.y, Zoom);
			bool Ok = SaveOutputPng(m_OutputImage.value());
			m_OutputImage->Free();
			m_OutputImage.reset();
			return Ok;
		}
	};
} // namespace

int main(int argc, const char **argv)
{
	CCmdlineFix CmdlineFix(&argc, &argv);
	log_set_global_logger_default();

	CMapPreviewTool Tool;
	if(!Tool.Init(argc, argv))
		return -1;
	return Tool.Export() ? 0 : 1;
}
