#include "section_loader.h"

#include <base/perf_timer.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <cstdlib>

CSectionLoader::CSectionLoader() = default;

void CSectionLoader::Register(std::vector<SSettingsSection> vSections)
{
	m_vSections = std::move(vSections);
}

void CSectionLoader::Begin(CUIRect MainView, float TimeBudgetMs)
{
	m_MainView = MainView;
	m_BudgetPerFrameMs = (double)TimeBudgetMs;
	m_CurrentIndex = 0;

	m_bLightweight = false;
	m_bComplete = false;
	m_TotalFrameTimeMs = 0.0;
}

void CSectionLoader::BeginLightweight(int InitialFrames, float TimeBudgetMs)
{
	m_bLightweight = true;
	m_LightweightFramesInitial = InitialFrames;
	m_LightweightFramesRemaining = InitialFrames;
	m_BudgetPerFrameMs = (double)TimeBudgetMs;
	m_bComplete = false;
	m_TotalFrameTimeMs = 0.0;
}

int CSectionLoader::GetFramesRemaining() const
{
	if(m_bLightweight)
		return m_LightweightFramesRemaining;
	// In full mode, estimate based on how many sections aren't FULL
	int Remaining = 0;
	for(const auto &S : m_vSections)
		if(S.m_State != ESettingsSectionState::FULL)
			++Remaining;
	return Remaining;
}

bool CSectionLoader::Process()
{
	if(m_bLightweight)
	{
		CPerfTimer FrameTimer;
		if(m_LightweightFramesRemaining > 0)
			--m_LightweightFramesRemaining;
		if(m_LightweightFramesRemaining <= 0)
			m_bComplete = true;
		return !m_bComplete;
	}

	if(!m_bInitialized)
	{
		for(auto &Section : m_vSections)
		{
			Section.m_State = ESettingsSectionState::UNINITIALIZED;
			Section.m_CachedHeight = 0.0f;
			Section.m_bDirty = true;
		}
		m_bInitialized = true;
		m_CurrentIndex = 0;
	}

	CPerfTimer FrameTimer;
	int UnlockedThisFrame = 0;
	const int MaxUnlockPerFrame = 2;

	while(m_CurrentIndex < (int)m_vSections.size())
	{
		if(FrameTimer.ElapsedMs() >= m_BudgetPerFrameMs)
			break;

		SSettingsSection &Section = m_vSections[m_CurrentIndex];

		switch(Section.m_State)
		{
		case ESettingsSectionState::UNINITIALIZED:
		{
			if(Section.m_MeasureFn)
			{
				CUIRect MeasureRect = m_MainView;
				Section.m_CachedHeight = Section.m_MeasureFn(MeasureRect);
			}
			else
			{
				Section.m_CachedHeight = 0.0f;
			}
			Section.m_State = ESettingsSectionState::MEASURING;
			++m_CurrentIndex;
			break;
		}
		case ESettingsSectionState::MEASURING:
		{
			const CUIRect SectionRect{m_MainView.x, m_MainView.y, m_MainView.w, Section.m_CachedHeight};
			const int Priority = ComputeViewportPriority(SectionRect);
			if(Priority <= 1)
			{
				Section.m_State = ESettingsSectionState::COMPACT;
				if(Section.m_RenderCompactFn)
					Section.m_RenderCompactFn(m_MainView);
			}
			++m_CurrentIndex;
			break;
		}
		case ESettingsSectionState::COMPACT:
		{
			const CUIRect SectionRect{m_MainView.x, m_MainView.y, m_MainView.w, Section.m_CachedHeight};
			const int Priority = ComputeViewportPriority(SectionRect);
			if(UnlockedThisFrame < MaxUnlockPerFrame && Priority <= 1)
			{
				Section.m_State = ESettingsSectionState::FULL;
				if(Section.m_RenderFullFn)
					Section.m_RenderFullFn(m_MainView);
				Section.m_LastConfigHash = ComputeConfigHash(Section);
				Section.m_bDirty = false;
				++UnlockedThisFrame;
				++m_CurrentIndex;
				break;
			}
			// Still not promoted; re-render compact
			if(Section.m_RenderCompactFn)
				Section.m_RenderCompactFn(m_MainView);
			++m_CurrentIndex;
			break;
		}
		case ESettingsSectionState::FULL:
		{
			if(!Section.m_bDirty)
			{
				++m_CurrentIndex;
				break;
			}
			if(Section.m_RenderFullFn)
				Section.m_RenderFullFn(m_MainView);
			Section.m_LastConfigHash = ComputeConfigHash(Section);
			Section.m_bDirty = false;
			++m_CurrentIndex;
			break;
		}
		}
	}

	if(m_CurrentIndex >= (int)m_vSections.size())
	{
		m_bComplete = true;
		for(const auto &Sect : m_vSections)
		{
			if(Sect.m_State != ESettingsSectionState::FULL)
			{
				m_bComplete = false;
				m_CurrentIndex = 0;
				break;
			}
		}
	}

	m_TotalFrameTimeMs += FrameTimer.ElapsedMs();

	// Profiling: log when budget is exceeded and perf-debug is enabled
	if(g_Config.m_QmPerfDebug && m_TotalFrameTimeMs > 1.0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf),
			"section_loader: sections=%d budget=%.1fms actual=%.1fms complete=%d",
			(int)m_vSections.size(), m_BudgetPerFrameMs, m_TotalFrameTimeMs,
			m_bComplete ? 1 : 0);
		dbg_msg("perf/section_loader", "%s", aBuf);
	}

	return !m_bComplete;
}

bool CSectionLoader::IsComplete() const
{
	return m_bComplete;
}

void CSectionLoader::Reset()
{
	m_bInitialized = false;
	m_bComplete = false;
	m_CurrentIndex = 0;
	m_TotalFrameTimeMs = 0.0;
}

// -- Pre-warming --

bool CSectionLoader::Warmup(const SSessionUiCache *pCache, float TimeBudgetMs)
{
	if(!pCache || !pCache->m_bValid)
	{
		m_bWarmupActive = false;
		return true;
	}

	if(!m_bWarmupActive)
	{
		m_bWarmupActive = true;
		m_WarmupIndex = 0;
		m_WarmupBudgetMs = TimeBudgetMs;
		m_pWarmupCache = pCache;
		for(auto &Section : m_vSections)
		{
			Section.m_State = ESettingsSectionState::UNINITIALIZED;
			Section.m_CachedHeight = 0.0f;
		}
	}

	CPerfTimer WarmupTimer;
	while(m_WarmupIndex < (int)m_vSections.size())
	{
		if(WarmupTimer.ElapsedMs() >= (double)m_WarmupBudgetMs)
			break;

		SSettingsSection &Section = m_vSections[m_WarmupIndex];

		const CUIRect SectionRect{m_MainView.x, m_MainView.y, m_MainView.w, Section.m_CachedHeight};
		const int Priority = ComputeViewportPriority(SectionRect);

		if(Priority > 1)
		{
			// Far from viewport: measure only
			if(Section.m_State == ESettingsSectionState::UNINITIALIZED)
			{
				if(Section.m_MeasureFn)
				{
					CUIRect MeasureRect = m_MainView;
					Section.m_CachedHeight = Section.m_MeasureFn(MeasureRect);
				}
				Section.m_State = ESettingsSectionState::MEASURING;
			}
			++m_WarmupIndex;
			continue;
		}

		// In or near viewport: render compact to trigger glyph rasterization
		const CPerfTimer SectTimer;
		if(Section.m_RenderCompactFn)
			Section.m_RenderCompactFn(m_MainView);
		Section.m_State = ESettingsSectionState::COMPACT;

		const double Elapsed = SectTimer.ElapsedMs();
		++m_WarmupIndex;

		if(Elapsed > 1.0)
			break; // expensive section; leave rest for next frame
	}

	if(m_WarmupIndex >= (int)m_vSections.size())
	{
		m_bWarmupActive = false;
		return true;
	}
	return false;
}

bool CSectionLoader::IsWarmupComplete() const
{
	return !m_bWarmupActive;
}

// -- Cache invalidation --

void CSectionLoader::InvalidateCache()
{
	for(auto &Section : m_vSections)
		Section.m_bDirty = true;
}

void CSectionLoader::SetDirtyByConfig(const void *pConfigVar)
{
	for(auto &Section : m_vSections)
	{
		for(const int *pInt : Section.m_DependencyConfigInts)
		{
			if(static_cast<const void *>(pInt) == pConfigVar)
			{
				Section.m_bDirty = true;
				return;
			}
		}
		for(const unsigned *pCol : Section.m_DependencyConfigCols)
		{
			if(static_cast<const void *>(pCol) == pConfigVar)
			{
				Section.m_bDirty = true;
				return;
			}
		}
	}
}

// -- Session cache I/O --

bool CSectionLoader::LoadSessionCache(SSessionUiCache &Cache, const char *pFilename, IStorage *pStorage)
{
	const IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return false;

	char aBuf[256];
	const unsigned Read = io_read(File, aBuf, sizeof(aBuf) - 1);
	io_close(File);
	if(Read == 0)
		return false;

	aBuf[Read] = '\0';

	// Parse simple key=value lines
	const char *p = aBuf;
	while(*p)
	{
		// Skip whitespace
		while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			++p;
		if(*p == '\0')
			break;

		// Read key
		const char *pKeyEnd = p;
		while(*pKeyEnd && *pKeyEnd != '=' && *pKeyEnd != '\r' && *pKeyEnd != '\n')
			++pKeyEnd;
		const int KeyLen = (int)(pKeyEnd - p);
		if(KeyLen <= 0 || *pKeyEnd != '=')
		{
			// Advance past this line
			while(*p && *p != '\n')
				++p;
			if(*p == '\n')
				++p;
			continue;
		}

		const char *pVal = pKeyEnd + 1;
		const char *pValEnd = pVal;
		while(*pValEnd && *pValEnd != '\r' && *pValEnd != '\n')
			++pValEnd;
		const int ValLen = (int)(pValEnd - pVal);

		if(KeyLen == 12 && strncmp(p, "tab_tclient=", 12) == 0)
			Cache.m_LastTClientTab = atoi(pVal);
		else if(KeyLen == 6 && strncmp(p, "tab_qm=", 6) == 0)
			Cache.m_LastQmTab = atoi(pVal);
		else if(KeyLen == 8 && strncmp(p, "scroll_y=", 8) == 0)
			Cache.m_LastScrollY = (float)atof(pVal);

		p = pValEnd;
		if(*p == '\n')
			++p;
	}

	Cache.m_bValid = (Cache.m_LastTClientTab >= 0 || Cache.m_LastQmTab >= 0);
	return Cache.m_bValid;
}

void CSectionLoader::SaveSessionCache(const SSessionUiCache &Cache, const char *pFilename, IStorage *pStorage)
{
	if(!Cache.m_bValid)
		return;

	pStorage->CreateFolder("qmclient", IStorage::TYPE_SAVE);
	const IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	char aLine[128];
	int Len;

	Len = str_format(aLine, sizeof(aLine), "tab_tclient=%d\n", Cache.m_LastTClientTab);
	io_write(File, aLine, (unsigned)Len);

	Len = str_format(aLine, sizeof(aLine), "tab_qm=%d\n", Cache.m_LastQmTab);
	io_write(File, aLine, (unsigned)Len);

	Len = str_format(aLine, sizeof(aLine), "scroll_y=%f\n", Cache.m_LastScrollY);
	io_write(File, aLine, (unsigned)Len);

	io_close(File);
}

// -- Profiling --

const char *CSectionLoader::GetPerfReport() const
{
	return ""; // Reporting is inline via dbg_msg in Process()
}

// -- Private helpers --

int CSectionLoader::ComputeViewportPriority(const CUIRect &SectionRect) const
{
	const float ViewportTop = m_MainView.y - m_ScrollY;
	const float ViewportBottom = ViewportTop + m_MainView.h;
	const float PrefetchMargin = 200.0f;

	if(SectionRect.y + SectionRect.h >= ViewportTop - PrefetchMargin &&
		SectionRect.y <= ViewportBottom + PrefetchMargin)
	{
		if(SectionRect.y + SectionRect.h >= ViewportTop &&
			SectionRect.y <= ViewportBottom)
			return 0; // In viewport
		return 1; // Near viewport
	}
	return 2; // Far from viewport
}

uint64_t CSectionLoader::ComputeConfigHash(const SSettingsSection &Section)
{
	// FNV-1a 64-bit
	uint64_t Hash = 14695981039346656037ull;
	for(const int *pVal : Section.m_DependencyConfigInts)
	{
		const uint8_t *pBytes = reinterpret_cast<const uint8_t *>(pVal);
		for(size_t i = 0; i < sizeof(int); ++i)
		{
			Hash ^= pBytes[i];
			Hash *= 1099511628211ull;
		}
	}
	for(const unsigned *pVal : Section.m_DependencyConfigCols)
	{
		const uint8_t *pBytes = reinterpret_cast<const uint8_t *>(pVal);
		for(size_t i = 0; i < sizeof(unsigned); ++i)
		{
			Hash ^= pBytes[i];
			Hash *= 1099511628211ull;
		}
	}
	return Hash;
}
