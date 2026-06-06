#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_STATUSBAR_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_STATUSBAR_H

#include <game/client/component.h>

enum
{
	STATUSBAR_MAX_SIZE = 128,
	STATUSBAR_TYPE_LETTERS = 4
};

class CStatusItem
{
public:
	std::function<void()> m_RenderItem;
	std::function<float()> m_GetWidth;
	char m_aName[32];
	char m_aDisplayName[32];
	char m_aDesc[128];
	char m_aLetters[STATUSBAR_TYPE_LETTERS] = {};
	bool m_ShowLabel = true;
	CStatusItem(std::function<void()> Render, std::function<float()> Width, const char *pLetters, const char *pName, const char *pDisplayName, const char *pDesc, bool ShowLabel = true);
};

class CStatusBar : public CComponent
{
public:
	CStatusBar() = default;
	CStatusBar(const CStatusBar &) = delete;
	CStatusBar &operator=(const CStatusBar &) = delete;

	int Sizeof() const override { return sizeof(*this); }
	void OnRender() override;
	void OnInit() override;

	CStatusItem m_Angle = CStatusItem([this] { AngleRender(); }, [this] { return AngleWidth(); },
		"a", "Angle", "", "Displays your current angle in degrees");
	CStatusItem m_Ping = CStatusItem([this] { PingRender(); }, [this] { return PingWidth(); },
		"p", "Ping", "", "Displays your ping to the current server");
	CStatusItem m_Prediction = CStatusItem([this] { PredictionRender(); }, [this] { return PredictionWidth(); },
		"d", "Prediction", "预测值", "显示当前预测值");
	CStatusItem m_Position = CStatusItem([this] { PositionRender(); }, [this] { return PositionWidth(); },
		"c", "Position", "坐标", "显示当前位置");
	CStatusItem m_LocalTime = CStatusItem([this] { LocalTimeRender(); }, [this] { return LocalTimeWidth(); },
		"l", "Local Time", "", "Displays your local time", false);
	CStatusItem m_RaceTime = CStatusItem([this] { RaceTimeRender(); }, [this] { return RaceTimeWidth(); },
		"r", "Race Time", "", "Display your race time", false);
	CStatusItem m_FPS = CStatusItem([this] { FPSRender(); }, [this] { return FPSWidth(); },
		"f", "FPS", "", "Displays your frames per second");
	CStatusItem m_Velocity = CStatusItem([this] { VelocityRender(); }, [this] { return VelocityWidth(); },
		"v", "Velocity", "", "Displays X and Y velocity");
	CStatusItem m_Zoom = CStatusItem([this] { ZoomRender(); }, [this] { return ZoomWidth(); },
		"z", "Zoom", "", "Displays current zoom value");
	CStatusItem m_Downstream = CStatusItem([this] { DownstreamRender(); }, [this] { return DownstreamWidth(); },
		"u", "Snapshot Latency", "延迟", "显示服务器快照延迟");
	CStatusItem m_Upstream = CStatusItem([this] { UpstreamRender(); }, [this] { return UpstreamWidth(); },
		"n", "Prediction Latency", "预测延迟", "显示客户端预测延迟");
	CStatusItem m_Jitter = CStatusItem([this] { JitterRender(); }, [this] { return JitterWidth(); },
		"j", "Latency Jitter", "延迟抖动", "显示延迟波动");
	CStatusItem m_PacketLoss = CStatusItem([this] { PacketLossRender(); }, [this] { return PacketLossWidth(); },
		"k", "Resend Loss", "重发丢包", "显示重发推算的丢包率");
	CStatusItem m_DownRate = CStatusItem([this] { DownRateRender(); }, [this] { return DownRateWidth(); },
		"i", "Receive Rate", "接收速率", "显示客户端接收速率");
	CStatusItem m_UpRate = CStatusItem([this] { UpRateRender(); }, [this] { return UpRateWidth(); },
		"o", "Send Rate", "发送速率", "显示客户端发送速率");
	CStatusItem m_ConnectionGrade = CStatusItem([this] { ConnectionGradeRender(); }, [this] { return ConnectionGradeWidth(); },
		"q", "Connection Quality", "连接质量", "显示连接质量等级");
	CStatusItem m_Cpu = CStatusItem([this] { CpuRender(); }, [this] { return CpuWidth(); },
		"x", "DDNet / Total CPU", "DDNet/总 CPU", "显示 DDNet 进程 CPU 占用率 / 系统总 CPU 占用率");
	CStatusItem m_Memory = CStatusItem([this] { MemoryRender(); }, [this] { return MemoryWidth(); },
		"y", "DDNet Memory", "DDNet 内存", "显示 DDNet 进程内存占用");
	CStatusItem m_Space = CStatusItem([this] { SpaceRender(); }, [this] { return SpaceWidth(); },
		" _", "Space", " ", "Gap between statusbar items", false);

	std::vector<CStatusItem> m_StatusItemTypes = {m_Angle, m_Ping, m_Prediction, m_Position, m_LocalTime, m_RaceTime, m_FPS, m_Velocity, m_Zoom, m_Downstream, m_Upstream, m_Jitter, m_PacketLoss, m_DownRate, m_UpRate, m_ConnectionGrade, m_Cpu, m_Memory, m_Space};
	std::vector<CStatusItem *> m_StatusBarItems = {&m_LocalTime, &m_FPS, &m_Space, &m_Angle, &m_Space, &m_Ping};

	void UpdateStatusBarSize();
	void ApplyStatusBarScheme(const char *pScheme);
	void UpdateStatusBarScheme(char *pScheme);

	bool m_PingActive = false;

private:
	float m_FrameTimeAverage = 0.0f;
	int m_PlayerId = 0;
	float m_FontSize = 12.0f;
	float m_CursorX, m_CursorY, m_BarX = 0.0f, m_BarY;
	float m_Width, m_Height;
	float m_BarHeight, m_Margin;

	int m_CurrentRaceTime = 0;
	float GetDurationWidth(int Duration);
	int GetDigitsIndex(int Value, int Max);
	float AngleWidth();
	void AngleRender();

	float PingWidth();
	void PingRender();

	float PredictionWidth();
	void PredictionRender();

	float PositionWidth();
	void PositionRender();

	float LocalTimeWidth();
	void LocalTimeRender();

	float RaceTimeWidth();
	void RaceTimeRender();

	float FPSWidth();
	void FPSRender();

	float VelocityWidth();
	void VelocityRender();

	float ZoomWidth();
	void ZoomRender();

	float DownstreamWidth();
	void DownstreamRender();

	float UpstreamWidth();
	void UpstreamRender();

	float JitterWidth();
	void JitterRender();

	float PacketLossWidth();
	void PacketLossRender();

	float DownRateWidth();
	void DownRateRender();

	float UpRateWidth();
	void UpRateRender();

	float ConnectionGradeWidth();
	void ConnectionGradeRender();

	float CpuWidth();
	void CpuRender();

	float MemoryWidth();
	void MemoryRender();

	float SpaceWidth();
	void SpaceRender();

	void LabelRender(const char *pLabel);
	float LabelWidth(const char *pLabel);
};

#endif
