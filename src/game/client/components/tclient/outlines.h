#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_OUTLINES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_OUTLINES_H

#include <game/client/component.h>

#include <vector>

class CTile;
class CTeleTile;

class COutlines : public CComponent
{
private:
	ivec2 m_MapDataSize;
	std::vector<int> m_vMapData;

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnMapLoad() override;
	void OnRender() override;
};

#endif
