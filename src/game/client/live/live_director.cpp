#include "live_director.h"

#include <game/teamscore.h>

void CLiveDirector::Reset()
{
	m_vTeams.clear();
	m_FallbackPlayer = -1;
	m_Mode = CLiveObserverSession::EDirectorMode::FREEVIEW;
}

void CLiveDirector::UpdateTeams(const std::array<int, MAX_CLIENTS> &aTeams, const std::array<bool, MAX_CLIENTS> &aActivePlayers)
{
	m_vTeams.clear();
	m_FallbackPlayer = -1;

	std::array<int, TEAM_SUPER> aTeamCounts{};
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		if(!aActivePlayers[ClientId])
			continue;

		if(m_FallbackPlayer < 0)
			m_FallbackPlayer = ClientId;

		const int Team = aTeams[ClientId];
		if(Team > TEAM_FLOCK && Team < TEAM_SUPER)
			aTeamCounts[Team]++;
	}

	for(int Team = TEAM_FLOCK + 1; Team < TEAM_SUPER; ++Team)
	{
		if(aTeamCounts[Team] > 0)
			m_vTeams.push_back({Team, aTeamCounts[Team]});
	}
}

int CLiveDirector::SelectRandomTeam(unsigned Seed) const
{
	if(m_vTeams.empty())
		return -1;

	return m_vTeams[Seed % m_vTeams.size()].m_Team;
}
