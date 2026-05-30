#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BACKGROUND_PARTICLES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BACKGROUND_PARTICLES_H

#include <base/color.h>
#include <base/vmath.h>

#include <game/client/component.h>

#include <array>
#include <vector>

class CBackgroundParticles : public CComponent
{
	static constexpr int MAX_TRAIL_POINTS = 6;

	struct SParticle
	{
		vec2 m_Pos = vec2(0.0f, 0.0f);
		vec2 m_DriftVel = vec2(0.0f, 0.0f);
		vec2 m_PushVel = vec2(0.0f, 0.0f);
		float m_Depth = 0.0f;
		float m_Size = 1.0f;
		vec3 m_Rotation = vec3(0.0f, 0.0f, 0.0f);
		vec3 m_RotationSpeed = vec3(0.0f, 0.0f, 0.0f);
		float m_Age = 0.0f;
		float m_Life = 1.0f;
		float m_PulsePhase = 0.0f;
		float m_TwinklePhase = 0.0f;
		float m_TrailSample = 0.0f;
		int m_Type = 1;
		int m_TrailCount = 0;
		std::array<vec2, MAX_TRAIL_POINTS> m_aTrailPos;
		ColorRGBA m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	};

	std::vector<SParticle> m_vParticles;
	std::vector<int> m_vRenderOrder;
	int m_LastConfiguredCount = -1;
	float m_LastLeft = 0.0f;
	float m_LastTop = 0.0f;
	float m_LastRight = 0.0f;
	float m_LastBottom = 0.0f;

	void ResetParticles();
	void EnsureParticleCount(float Left, float Top, float Right, float Bottom);
	void SpawnParticle(SParticle &Particle, bool Initial, float Left, float Top, float Right, float Bottom);
	void UpdateParticle(SParticle &Particle, float Delta, float Left, float Top, float Right, float Bottom);
	void ResetParticleTrail(SParticle &Particle) const;
	void UpdateParticleTrail(SParticle &Particle, float Delta) const;
	void ApplyPlayerPush(SParticle &Particle, float Delta) const;
	void ApplyParticleCollisions(float Delta);
	ColorRGBA ParticleColor() const;
	int ParticleType() const;
	float ParticleAlpha(const SParticle &Particle) const;
	void RenderParticle(const SParticle &Particle, vec2 Center) const;
	void RenderParticleTrail(const SParticle &Particle, vec2 Center, float Parallax, float Size, ColorRGBA Color) const;
	void RenderShape(int Type, vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderCube(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderHeart(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderSphere(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderPyramid(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderDiamond(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderRing(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderStar(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void RenderCrescent(vec2 Pos, float Size, const vec3 &Rotation, ColorRGBA Color) const;
	void CurrentWorldView(float &Left, float &Top, float &Right, float &Bottom) const;

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnReset() override;
	void OnRender() override;
};

#endif
