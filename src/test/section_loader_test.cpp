#include <base/system.h>

#include <game/client/components/section_loader.h>

#include <gtest/gtest.h>

// Helper: modify the CUIRect inline without calling HSplitTop (not linked in test runner)
static float ConsumeHeight(CUIRect &Rect, float Height)
{
	Rect.h -= Height;
	Rect.y += Height;
	return Height;
}

// Helper: create a simple section with a fixed height
static SSettingsSection MakeTestSection(const char *pName, float Height)
{
	SSettingsSection S;
	S.m_pName = pName;
	S.m_MeasureFn = [Height](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, Height);
	};
	S.m_RenderCompactFn = [Height](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, Height);
	};
	S.m_RenderFullFn = [Height](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, Height);
	};
	return S;
}

TEST(SectionLoader, StateTransitions)
{
	CSectionLoader Loader;
	Loader.Register({
		MakeTestSection("Section A", 50.0f),
		MakeTestSection("Section B", 60.0f),
	});

	CUIRect MainView{0, 0, 400, 600};
	Loader.Begin(MainView, 100.0f);

	int Iterations = 0;
	while(Loader.Process() && Iterations < 10)
		++Iterations;

	EXPECT_TRUE(Loader.IsComplete());
}

TEST(SectionLoader, FrameBudgetTruncation)
{
	CSectionLoader Loader;
	std::vector<SSettingsSection> vSections;
	for(int i = 0; i < 30; ++i)
	{
		char aName[32];
		str_format(aName, sizeof(aName), "Section %d", i);
		vSections.push_back(MakeTestSection(aName, 30.0f));
	}
	Loader.Register(std::move(vSections));

	CUIRect MainView{0, 0, 400, 800};
	Loader.Begin(MainView, 0.1);

	int FrameCount = 0;
	while(Loader.Process() && FrameCount < 100)
		++FrameCount;

	// With 0.1ms budget and 30 sections, it MUST take more than 1 frame
	EXPECT_GT(FrameCount, 1);
}

TEST(SectionLoader, DirtyFlagSkipsRender)
{
	CSectionLoader Loader;
	bool FullRenderCalled = false;
	int ConfigValue = 42;

	SSettingsSection S;
	S.m_pName = "DirtyTest";
	S.m_DependencyConfigInts.push_back(&ConfigValue);
	S.m_MeasureFn = [](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, 10.0f);
	};
	S.m_RenderCompactFn = [](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, 10.0f);
	};
	S.m_RenderFullFn = [&FullRenderCalled](CUIRect &Rect) -> float {
		FullRenderCalled = true;
		return ConsumeHeight(Rect, 10.0f);
	};

	Loader.Register({S});
	CUIRect MainView{0, 0, 400, 600};

	// First frame: m_bDirty=true → FullRender must be called
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}
	EXPECT_TRUE(FullRenderCalled);

	// Second frame: config unchanged → FullRender must NOT be called
	FullRenderCalled = false;
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}
	EXPECT_FALSE(FullRenderCalled);

	// Modify config → dirty flag triggers FullRender again
	ConfigValue = 99;
	Loader.SetDirtyByConfig(&ConfigValue);
	Loader.Begin(MainView, 100.0f);
	FullRenderCalled = false;
	while(Loader.Process()) {}
	EXPECT_TRUE(FullRenderCalled);
}

TEST(SectionLoader, MeasureFullHeightConsistency)
{
	CSectionLoader Loader;
	float MeasureHeight = 0.0f;
	float FullHeight = 0.0f;

	SSettingsSection S;
	S.m_pName = "HeightTest";
	S.m_MeasureFn = [&MeasureHeight](CUIRect &Rect) -> float {
		MeasureHeight = 45.0f;
		return ConsumeHeight(Rect, 45.0f);
	};
	S.m_RenderCompactFn = [](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, 45.0f);
	};
	S.m_RenderFullFn = [&FullHeight](CUIRect &Rect) -> float {
		FullHeight = 45.0f;
		return ConsumeHeight(Rect, 45.0f);
	};

	Loader.Register({S});
	CUIRect MainView{0, 0, 400, 800};
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}

	EXPECT_FLOAT_EQ(MeasureHeight, FullHeight);
}

TEST(SectionLoader, WarmupWithoutCache)
{
	CSectionLoader Loader;
	Loader.Register({MakeTestSection("A", 50.0f)});

	// nullptr cache → warmup should immediately report done
	EXPECT_TRUE(Loader.Warmup(nullptr, 3.0f));
	EXPECT_TRUE(Loader.IsWarmupComplete());

	// Invalid cache → same
	SSessionUiCache InvalidCache;
	InvalidCache.m_bValid = false;
	EXPECT_TRUE(Loader.Warmup(&InvalidCache, 3.0f));
	EXPECT_TRUE(Loader.IsWarmupComplete());
}

TEST(SectionLoader, WarmupWithCache)
{
	CSectionLoader Loader;
	Loader.Register({
		MakeTestSection("A", 50.0f),
		MakeTestSection("B", 50.0f),
		MakeTestSection("C", 50.0f),
	});

	CUIRect MainView{0, 0, 400, 600};
	Loader.Begin(MainView, 5.0f);

	SSessionUiCache Cache;
	Cache.m_LastTClientTab = 0;
	Cache.m_LastScrollY = 0.0f;
	Cache.m_bValid = true;

	bool Done = false;
	for(int i = 0; i < 20 && !Done; ++i)
		Done = Loader.Warmup(&Cache, 100.0f);

	EXPECT_TRUE(Done);
	EXPECT_TRUE(Loader.IsWarmupComplete());
}

TEST(SectionLoader, InvalidateAfterLanguageSwitch)
{
	CSectionLoader Loader;
	int RenderCount = 0;

	SSettingsSection S;
	S.m_pName = "CacheTest";
	S.m_MeasureFn = [](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, 10.0f);
	};
	S.m_RenderCompactFn = [](CUIRect &Rect) -> float {
		return ConsumeHeight(Rect, 10.0f);
	};
	S.m_RenderFullFn = [&RenderCount](CUIRect &Rect) -> float {
		++RenderCount;
		return ConsumeHeight(Rect, 10.0f);
	};

	Loader.Register({S});
	CUIRect MainView{0, 0, 400, 600};

	// First frame: renders
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}
	EXPECT_EQ(RenderCount, 1);

	// Second frame: skips (clean)
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}
	EXPECT_EQ(RenderCount, 1);

	// Simulate language change
	Loader.InvalidateCache();
	Loader.Begin(MainView, 100.0f);
	while(Loader.Process()) {}
	EXPECT_EQ(RenderCount, 2);
}
