#include "background_particles.h"

#include <base/math.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int MAX_BACKGROUND_PARTICLES = 200;
constexpr float MIN_VIEW_SIZE = 1.0f;
constexpr float MAX_EFFECTIVE_SIZE = 24.0f;

float ClampDelta(float Delta)
{
	return std::clamp(Delta, 0.0f, 0.05f);
}

float ConfigAlpha()
{
	return std::clamp(g_Config.m_Bc3DParticlesAlpha, 1, 100) / 100.0f;
}

ColorRGBA Shade(ColorRGBA Color, float Brightness, float Alpha)
{
	Color.r = std::clamp(Color.r * Brightness, 0.0f, 1.0f);
	Color.g = std::clamp(Color.g * Brightness, 0.0f, 1.0f);
	Color.b = std::clamp(Color.b * Brightness, 0.0f, 1.0f);
	Color.a = std::clamp(Color.a * Alpha, 0.0f, 1.0f);
	return Color;
}

float ClampedSizeMin()
{
	return (float)std::clamp(g_Config.m_Bc3DParticlesSizeMin, 2, 64);
}

float ClampedSizeMax()
{
	const int SizeMin = std::clamp(g_Config.m_Bc3DParticlesSizeMin, 2, 64);
	const int SizeMax = std::clamp(g_Config.m_Bc3DParticlesSizeMax, SizeMin, 64);
	return (float)SizeMax;
}

vec2 Rotate(vec2 Point, float Rotation)
{
	const float S = std::sin(Rotation);
	const float C = std::cos(Rotation);
	return vec2(Point.x * C - Point.y * S, Point.x * S + Point.y * C);
}
}

void CBackgroundParticles::OnReset()
{
	ResetParticles();
}

void CBackgroundParticles::ResetParticles()
{
	m_vParticles.clear();
	m_vRenderOrder.clear();
	m_LastConfiguredCount = -1;
}

void CBackgroundParticles::CurrentWorldView(float &Left, float &Top, float &Right, float &Bottom) const
{
	const vec2 Center = GameClient()->m_Camera.m_Center;
	const float Zoom = GameClient()->m_Camera.m_Zoom;
	float aPoints[4];
	Graphics()->MapScreenToWorld(Center.x, Center.y, 100, 100, 100, 0, 0, Graphics()->ScreenAspect(), Zoom, aPoints);
	Left = minimum(aPoints[0], aPoints[2]);
	Top = minimum(aPoints[1], aPoints[3]);
	Right = maximum(aPoints[0], aPoints[2]);
	Bottom = maximum(aPoints[1], aPoints[3]);

	if(Right - Left < MIN_VIEW_SIZE)
		Right = Left + MIN_VIEW_SIZE;
	if(Bottom - Top < MIN_VIEW_SIZE)
		Bottom = Top + MIN_VIEW_SIZE;
}

ColorRGBA CBackgroundParticles::ParticleColor() const
{
	if(g_Config.m_Bc3DParticlesColorMode == 2)
	{
		return color_cast<ColorRGBA>(ColorHSLA(random_float(), random_float(0.55f, 0.85f), random_float(0.55f, 0.75f), 1.0f));
	}
	return color_cast<ColorRGBA>(ColorHSLA(g_Config.m_Bc3DParticlesColor, true));
}

int CBackgroundParticles::ParticleType() const
{
	const int Type = std::clamp(g_Config.m_Bc3DParticlesType, 1, 3);
	if(Type != 3)
		return Type;
	return random_float() < 0.5f ? 1 : 2;
}

void CBackgroundParticles::SpawnParticle(SParticle &Particle, bool Initial, float Left, float Top, float Right, float Bottom)
{
	const float Margin = (float)std::clamp(g_Config.m_Bc3DParticlesViewMargin, 0, 1000);
	const float Width = Right - Left;
	const float Height = Bottom - Top;
	const float DepthRange = (float)std::clamp(g_Config.m_Bc3DParticlesDepth, 10, 1000);
	const float SizeMin = ClampedSizeMin();
	const float SizeMax = ClampedSizeMax();
	Particle.m_Depth = random_float(0.0f, DepthRange);
	const float DepthFactor = std::clamp(Particle.m_Depth / DepthRange, 0.0f, 1.0f);
	Particle.m_Size = random_float(SizeMin, SizeMax);
	Particle.m_Rotation = random_float(2.0f * pi);
	Particle.m_RotationSpeed = random_float(-1.4f, 1.4f) * (1.0f - DepthFactor * 0.45f);
	const float Speed = (float)std::clamp(g_Config.m_Bc3DParticlesSpeed, 1, 500) * (1.0f - DepthFactor * 0.60f);
	Particle.m_DriftVel = random_direction() * random_float(0.35f, 1.0f) * Speed;
	Particle.m_PushVel = vec2(0.0f, 0.0f);
	Particle.m_Age = 0.0f;
	Particle.m_Life = random_float(8.0f, 18.0f);
	Particle.m_Type = ParticleType();
	Particle.m_Color = ParticleColor();

	if(Initial)
	{
		Particle.m_Pos = vec2(random_float(Left - Margin, Right + Margin), random_float(Top - Margin, Bottom + Margin));
		Particle.m_Age = random_float(0.0f, Particle.m_Life * 0.75f);
		return;
	}

	switch((int)random_float(4.0f))
	{
	case 0:
		Particle.m_Pos = vec2(Left - Margin, random_float(Top - Margin, Bottom + Margin));
		break;
	case 1:
		Particle.m_Pos = vec2(Right + Margin, random_float(Top - Margin, Bottom + Margin));
		break;
	case 2:
		Particle.m_Pos = vec2(random_float(Left - Margin, Right + Margin), Top - Margin);
		break;
	default:
		Particle.m_Pos = vec2(random_float(Left - Margin, Right + Margin), Bottom + Margin);
		break;
	}
}

void CBackgroundParticles::EnsureParticleCount(float Left, float Top, float Right, float Bottom)
{
	const int Count = std::clamp(g_Config.m_Bc3DParticlesCount, 1, MAX_BACKGROUND_PARTICLES);
	if(m_LastConfiguredCount == Count && (int)m_vParticles.size() == Count)
		return;

	m_LastConfiguredCount = Count;
	if((int)m_vParticles.size() > Count)
	{
		m_vParticles.resize(Count);
		return;
	}

	const size_t OldSize = m_vParticles.size();
	m_vParticles.resize(Count);
	for(size_t ParticleIndex = OldSize; ParticleIndex < m_vParticles.size(); ++ParticleIndex)
		SpawnParticle(m_vParticles[ParticleIndex], true, Left, Top, Right, Bottom);
}

void CBackgroundParticles::ApplyPlayerPush(SParticle &Particle, float Delta) const
{
	const float Radius = (float)std::clamp(g_Config.m_Bc3DParticlesPushRadius, 0, 1000);
	const float Strength = (float)std::clamp(g_Config.m_Bc3DParticlesPushStrength, 0, 2000);
	if(Radius <= 0.0f || Strength <= 0.0f)
		return;

	for(int ClientId = 0; ClientId < MAX_CLIENTS; ++ClientId)
	{
		const auto &ClientData = GameClient()->m_aClients[ClientId];
		if(!ClientData.m_Active)
			continue;

		const vec2 Diff = Particle.m_Pos - ClientData.m_RenderPos;
		const float Dist = length(Diff);
		if(Dist <= 0.001f || Dist >= Radius)
			continue;

		const float Force = 1.0f - Dist / Radius;
		Particle.m_PushVel += normalize(Diff) * Strength * Force * Delta * 10.0f;
	}
}

void CBackgroundParticles::UpdateParticle(SParticle &Particle, float Delta, float Left, float Top, float Right, float Bottom)
{
	Particle.m_Age += Delta;
	if(Particle.m_Age >= Particle.m_Life)
	{
		SpawnParticle(Particle, false, Left, Top, Right, Bottom);
		return;
	}

	ApplyPlayerPush(Particle, Delta);
	Particle.m_Pos += (Particle.m_DriftVel + Particle.m_PushVel) * Delta;
	Particle.m_PushVel *= std::pow(0.12f, Delta);
	Particle.m_Rotation += Particle.m_RotationSpeed * Delta;

	const float Margin = (float)std::clamp(g_Config.m_Bc3DParticlesViewMargin, 0, 1000);
	if(Particle.m_Pos.x < Left - Margin || Particle.m_Pos.x > Right + Margin ||
		Particle.m_Pos.y < Top - Margin || Particle.m_Pos.y > Bottom + Margin)
	{
		SpawnParticle(Particle, false, Left, Top, Right, Bottom);
	}
}

void CBackgroundParticles::ApplyParticleCollisions(float Delta)
{
	if(!g_Config.m_Bc3DParticlesCollide)
		return;

	for(size_t LeftIndex = 0; LeftIndex < m_vParticles.size(); ++LeftIndex)
	{
		for(size_t RightIndex = LeftIndex + 1; RightIndex < m_vParticles.size(); ++RightIndex)
		{
			SParticle &Left = m_vParticles[LeftIndex];
			SParticle &Right = m_vParticles[RightIndex];
			const vec2 Diff = Right.m_Pos - Left.m_Pos;
			const float Dist = length(Diff);
			const float MinDist = (Left.m_Size + Right.m_Size) * 0.45f;
			if(Dist <= 0.001f || Dist >= MinDist)
				continue;

			const vec2 Dir = Diff / Dist;
			const float Push = (MinDist - Dist) * Delta * 12.0f;
			Left.m_PushVel -= Dir * Push;
			Right.m_PushVel += Dir * Push;
		}
	}
}

float CBackgroundParticles::ParticleAlpha(const SParticle &Particle) const
{
	const float FadeIn = std::max(0.001f, g_Config.m_Bc3DParticlesFadeInMs / 1000.0f);
	const float FadeOut = std::max(0.001f, g_Config.m_Bc3DParticlesFadeOutMs / 1000.0f);
	const float In = std::clamp(Particle.m_Age / FadeIn, 0.0f, 1.0f);
	const float Out = std::clamp((Particle.m_Life - Particle.m_Age) / FadeOut, 0.0f, 1.0f);
	return ConfigAlpha() * minimum(In, Out);
}

void CBackgroundParticles::RenderGlow(vec2 Pos, float Size, ColorRGBA Color) const
{
	if(!g_Config.m_Bc3DParticlesGlow)
		return;

	const float GlowAlpha = std::clamp(g_Config.m_Bc3DParticlesGlowAlpha, 1, 100) / 100.0f;
	const float GlowOffset = (float)std::clamp(g_Config.m_Bc3DParticlesGlowOffset, 1, 20);
	const vec2 Offset(GlowOffset, GlowOffset);
	const vec2 AxisX(Size * 0.82f, Size * 0.28f);
	const vec2 AxisY(-Size * 0.82f, Size * 0.28f);
	const vec2 AxisZ(0.0f, -Size * 0.72f);
	const vec2 GlowPos = Pos + Offset;
	IGraphics::CFreeformItem Glow(
		GlowPos + AxisZ,
		GlowPos + AxisX,
		GlowPos - AxisZ,
		GlowPos + AxisY);
	Graphics()->SetColor(Color.WithMultipliedAlpha(GlowAlpha * 0.18f));
	Graphics()->QuadsDrawFreeform(&Glow, 1);
}

void CBackgroundParticles::RenderCube(vec2 Pos, float Size, float Rotation, ColorRGBA Color) const
{
	const vec2 AxisX = Rotate(vec2(Size * 0.62f, Size * 0.22f), Rotation);
	const vec2 AxisY = Rotate(vec2(-Size * 0.62f, Size * 0.22f), Rotation);
	const vec2 AxisZ = Rotate(vec2(0.0f, -Size * 0.64f), Rotation * 0.35f);
	const vec2 aFront[4] = {
		Pos - AxisX,
		Pos + AxisZ,
		Pos - AxisY,
		Pos - AxisZ,
	};

	IGraphics::CFreeformItem LeftFace(aFront[0], aFront[1], Pos, aFront[3]);
	IGraphics::CFreeformItem RightFace(aFront[1], aFront[2], aFront[3], Pos);
	IGraphics::CFreeformItem TopFace(aFront[0], Pos, aFront[2], aFront[1]);

	Graphics()->SetColor(Shade(Color, 0.82f, 0.95f));
	Graphics()->QuadsDrawFreeform(&LeftFace, 1);
	Graphics()->SetColor(Shade(Color, 1.18f, 0.95f));
	Graphics()->QuadsDrawFreeform(&TopFace, 1);
	Graphics()->SetColor(Shade(Color, 0.60f, 0.95f));
	Graphics()->QuadsDrawFreeform(&RightFace, 1);
}

void CBackgroundParticles::RenderHeart(vec2 Pos, float Size, float Rotation, ColorRGBA Color) const
{
	const vec2 Tilt = Rotate(vec2(0.0f, -Size * 0.05f), Rotation);
	const vec2 Right = Rotate(vec2(Size * 0.21f, 0.0f), Rotation);
	const vec2 Down = Rotate(vec2(0.0f, Size * 0.28f), Rotation);
	Graphics()->SetColor(Shade(Color, 0.55f, 1.0f));
	IGraphics::CFreeformItem Shadow(Pos - Right * 1.8f + Tilt + Down * 0.5f, Pos + Right * 1.8f + Tilt + Down * 0.5f, Pos + Down * 2.1f, Pos + Down * 2.1f);
	Graphics()->QuadsDrawFreeform(&Shadow, 1);

	Graphics()->SetColor(Color);
	Graphics()->DrawCircle(Pos.x - Right.x, Pos.y - Right.y, Size * 0.30f, 16);
	Graphics()->DrawCircle(Pos.x + Right.x, Pos.y + Right.y, Size * 0.30f, 16);
	IGraphics::CFreeformItem Body(Pos - Right * 2.05f + Tilt, Pos + Tilt - Down * 0.75f, Pos + Right * 2.05f + Tilt, Pos + Down * 1.8f);
	Graphics()->QuadsDrawFreeform(&Body, 1);
}

void CBackgroundParticles::RenderParticle(const SParticle &Particle, vec2 Center) const
{
	const float DepthRange = (float)std::clamp(g_Config.m_Bc3DParticlesDepth, 10, 1000);
	const float DepthFactor = std::clamp(Particle.m_Depth / DepthRange, 0.0f, 1.0f);
	const float Parallax = 1.0f - DepthFactor * 0.72f;
	const vec2 Pos = Center + (Particle.m_Pos - Center) * Parallax;
	const float Size = minimum(Particle.m_Size, MAX_EFFECTIVE_SIZE) * (0.72f - DepthFactor * 0.25f);
	const float Alpha = ParticleAlpha(Particle) * (1.0f - DepthFactor * 0.35f);
	if(Alpha <= 0.01f || Size <= 0.5f)
		return;

	const ColorRGBA Color = Particle.m_Color.WithMultipliedAlpha(Alpha);
	RenderGlow(Pos, Size, Color);
	if(Particle.m_Type == 2)
		RenderHeart(Pos, Size, Particle.m_Rotation, Color);
	else
		RenderCube(Pos, Size, Particle.m_Rotation, Color);
}

void CBackgroundParticles::OnRender()
{
	if(!g_Config.m_Bc3DParticles)
	{
		m_LastConfiguredCount = -1;
		return;
	}

	float Left;
	float Top;
	float Right;
	float Bottom;
	CurrentWorldView(Left, Top, Right, Bottom);
	EnsureParticleCount(Left, Top, Right, Bottom);
	if(m_vParticles.empty())
		return;

	const float Delta = ClampDelta(Client()->RenderFrameTime());
	const bool ViewJumped = absolute(Left - m_LastLeft) + absolute(Top - m_LastTop) + absolute(Right - m_LastRight) + absolute(Bottom - m_LastBottom) > (Right - Left + Bottom - Top);
	m_LastLeft = Left;
	m_LastTop = Top;
	m_LastRight = Right;
	m_LastBottom = Bottom;
	if(ViewJumped)
	{
		for(SParticle &Particle : m_vParticles)
			SpawnParticle(Particle, true, Left, Top, Right, Bottom);
	}

	for(SParticle &Particle : m_vParticles)
		UpdateParticle(Particle, Delta, Left, Top, Right, Bottom);
	ApplyParticleCollisions(Delta);

	m_vRenderOrder.clear();
	m_vRenderOrder.reserve(m_vParticles.size());
	for(size_t ParticleIndex = 0; ParticleIndex < m_vParticles.size(); ++ParticleIndex)
		m_vRenderOrder.push_back((int)ParticleIndex);
	std::sort(m_vRenderOrder.begin(), m_vRenderOrder.end(), [&](int LeftIndex, int RightIndex) {
		return m_vParticles[LeftIndex].m_Depth > m_vParticles[RightIndex].m_Depth;
	});

	Graphics()->MapScreen(Left, Top, Right, Bottom);
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	const vec2 Center = GameClient()->m_Camera.m_Center;
	for(const int ParticleIndex : m_vRenderOrder)
		RenderParticle(m_vParticles[ParticleIndex], Center);
	Graphics()->QuadsEnd();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}
