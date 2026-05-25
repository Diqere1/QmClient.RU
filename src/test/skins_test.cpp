#include <game/client/components/skins.h>

#include <gtest/gtest.h>

TEST(Skins, UsageTrackingSkipsAlwaysLoadedStates)
{
	using EState = CSkins::CSkinContainer::EState;

	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::PENDING, true));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::LOADING, true));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::LOADED, true));

	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::PENDING, false));
	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::LOADING, false));
	EXPECT_TRUE(CSkins::CSkinContainer::TracksUsage(EState::LOADED, false));
	EXPECT_FALSE(CSkins::CSkinContainer::TracksUsage(EState::UNLOADED, false));
}

TEST(Skins, AlwaysLoadedStateTransitionsNeverTouchUsageList)
{
	using EState = CSkins::CSkinContainer::EState;

	for(const EState State : {EState::PENDING, EState::LOADING, EState::LOADED})
	{
		const auto Clean = CSkins::CSkinContainer::UsageTrackingUpdate(State, true, false);
		EXPECT_FALSE(Clean.m_ShouldTouch);
		EXPECT_FALSE(Clean.m_ShouldErase);

		const auto Polluted = CSkins::CSkinContainer::UsageTrackingUpdate(State, true, true);
		EXPECT_FALSE(Polluted.m_ShouldTouch);
		EXPECT_TRUE(Polluted.m_ShouldErase);
	}
}

TEST(Skins, UsageListEntriesThatCannotBeUnloadedAreDiscarded)
{
	using EState = CSkins::CSkinContainer::EState;

	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(false, EState::LOADED, false));
	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADED, true));
	EXPECT_TRUE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::NOT_FOUND, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::PENDING, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADING, false));
	EXPECT_FALSE(CSkins::CSkinContainer::ShouldDiscardUsageEntryBeforeUnload(true, EState::LOADED, false));
}

TEST(Skins, RegularStateTransitionsEnterAndLeaveUsageList)
{
	using EState = CSkins::CSkinContainer::EState;

	for(const EState State : {EState::PENDING, EState::LOADING, EState::LOADED})
	{
		const auto MissingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, false);
		EXPECT_TRUE(MissingEntry.m_ShouldTouch);
		EXPECT_FALSE(MissingEntry.m_ShouldErase);

		const auto ExistingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, true);
		EXPECT_FALSE(ExistingEntry.m_ShouldTouch);
		EXPECT_FALSE(ExistingEntry.m_ShouldErase);
	}

	for(const EState State : {EState::UNLOADED, EState::ERROR, EState::NOT_FOUND})
	{
		const auto ExistingEntry = CSkins::CSkinContainer::UsageTrackingUpdate(State, false, true);
		EXPECT_FALSE(ExistingEntry.m_ShouldTouch);
		EXPECT_TRUE(ExistingEntry.m_ShouldErase);
	}
}
