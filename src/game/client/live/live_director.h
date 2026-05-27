#ifndef GAME_CLIENT_LIVE_LIVE_DIRECTOR_H
#define GAME_CLIENT_LIVE_LIVE_DIRECTOR_H

#include <engine/shared/qm_live_observer_session.h>
#include <engine/shared/protocol.h>

#include <array>
#include <vector>

class CLiveDirector
{
public:
	class CTeamEntry
	{
	public:
		int m_Team = -1;
		int m_NumPlayers = 0;
	};

	void Reset();
	void UpdateTeams(const std::array<int, MAX_CLIENTS> &aTeams, const std::array<bool, MAX_CLIENTS> &aActivePlayers);
	bool HasValidTeams() const { return !m_vTeams.empty(); }
	const std::vector<CTeamEntry> &Teams() const { return m_vTeams; }

	int SelectRandomTeam(unsigned Seed) const;
	int FallbackPlayer() const { return m_FallbackPlayer; }

	void SetMode(CLiveObserverSession::EDirectorMode Mode) { m_Mode = Mode; }
	CLiveObserverSession::EDirectorMode Mode() const { return m_Mode; }

private:
	std::vector<CTeamEntry> m_vTeams;
	int m_FallbackPlayer = -1;
	CLiveObserverSession::EDirectorMode m_Mode = CLiveObserverSession::EDirectorMode::FREEVIEW;
};

#endif // GAME_CLIENT_LIVE_LIVE_DIRECTOR_H
