#include "settings_warmup.h"

#include <algorithm>
#include <utility>

static constexpr double MIN_WARMUP_SECTION_BUDGET_MS = 1.0;

void CSettingsWarmupScheduler::RegisterSection(SSettingsWarmupSection Section)
{
	m_vSections.push_back(std::move(Section));
	m_bSorted = false;
}

void CSettingsWarmupScheduler::SetLastSessionPage(EClassicSettingsPage Page)
{
	m_LastSessionPage = Page;
	m_bSorted = false;
}

void CSettingsWarmupScheduler::SetEnabled(bool Enabled)
{
	m_bEnabled = Enabled;
}

bool CSettingsWarmupScheduler::WarmupFrame(double BudgetMs)
{
	if(!m_bEnabled)
		return true;

	if(!m_bSorted)
	{
		std::stable_sort(m_vSections.begin(), m_vSections.end(), [&](const SSettingsWarmupSection &Left, const SSettingsWarmupSection &Right) {
			const bool LeftLastPage = Left.m_Page == m_LastSessionPage;
			const bool RightLastPage = Right.m_Page == m_LastSessionPage;
			if(LeftLastPage != RightLastPage)
				return LeftLastPage;
			if(Left.m_Kind != Right.m_Kind)
				return Left.m_Kind == ESettingsWarmupKind::RUNTIME_FBO;
			return Left.m_Priority < Right.m_Priority;
		});
		m_bSorted = true;
	}

	double UsedBudgetMs = 0.0;
	for(SSettingsWarmupSection &Section : m_vSections)
	{
		if(Section.m_bWarmed)
			continue;
		if(UsedBudgetMs > 0.0 && UsedBudgetMs + MIN_WARMUP_SECTION_BUDGET_MS > BudgetMs)
			return false;

		const double CostMs = Section.m_WarmupFn ? Section.m_WarmupFn() : 0.0;
		Section.m_bWarmed = true;
		UsedBudgetMs += CostMs;
		if(UsedBudgetMs >= BudgetMs)
			return false;
	}
	return true;
}

void CSettingsWarmupScheduler::Reset()
{
	for(SSettingsWarmupSection &Section : m_vSections)
		Section.m_bWarmed = false;
}
