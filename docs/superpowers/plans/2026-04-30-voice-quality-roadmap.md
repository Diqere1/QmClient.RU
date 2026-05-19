# 语音质量路线图实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在保持 `VOICE_VERSION = 3` 协议兼容的前提下，完成语音默认听感重调：显式设置 `Opus complexity`、优化编码目标表、重调默认动态处理参数、引入轻量 AGC（本轮默认关闭）、调整 RNNoise 默认策略。双声道采集 / 编码与 AEC 保留为后续专题。

**重要声明：** 本计划涉及默认参数大幅调整，是一次**默认听感重调**而非"兼容性维护"。协议层面保持向后兼容，但用户听感会变化。

**架构：** 先冻结纯函数策略和默认参数语义，再让 `voice_core` 按固定发送链路接入这些策略，随后补设置页与状态文案，最后用单测、构建和双端联调验证。

**技术栈：** C++17、gtest、SDL Audio、Opus、RNNoise、DDNet/QmClient UI system、CMake、Windows Ninja build

---

## 文件结构

### 新增文件
- 无

### 修改文件
| 文件 | 职责 |
|------|------|
| `docs/superpowers/specs/2026-04-30-voice-quality-roadmap-design.md` | 本计划对应的设计规格 |
| `docs/superpowers/plans/2026-04-30-voice-quality-roadmap.md` | 本实现计划 |
| `src/game/client/components/qmclient/voice_utils.h` | 新增编码目标、默认参数、轻量 AGC、处理链配置结构声明 |
| `src/game/client/components/qmclient/voice_utils.cpp` | 实现编码目标表、默认参数映射、轻量 AGC / 输入增益 helper、降噪默认策略 helper |
| `src/game/client/components/qmclient/voice_core.h` | **扩展运行时状态**：新增 `m_EncComplexity`、`m_AgcGain` 字段；扩展 `SRClientVoiceConfigSnapshot` 新增 `m_QmVoiceAgcEnable` 字段；新增 `ProcessVoiceCaptureFrame_ForTest` 测试专用接口（`#ifdef CONF_TEST`） |
| `src/game/client/components/qmclient/voice_core.cpp` | 接入显式 `Opus complexity`、固定发送处理链、应用默认参数与轻量 AGC；更新 `GetConfigSnapshot()` 读取新增配置项；实现测试专用接口 |
| `src/engine/shared/config_variables_qmclient.h` | 调整现有语音默认配置值；**新增** `QmVoiceAgcEnable` 配置项（默认关闭） |
| `src/game/client/components/qmclient/menus_qmclient.cpp` | 让语音设置页文案与默认策略保持一致，避免 UI 展示与运行时语义脱节 |
| `src/test/voice_core_test.cpp` | 新增编码目标、默认参数、AGC 边界、处理顺序与回退行为测试 |

---

## 默认值变更对照表

本计划涉及以下默认值调整，执行前需明确知晓影响：

| 参数 | 旧默认值 | 新默认值 | 变化幅度 | 影响范围 |
|------|----------|----------|----------|----------|
| `QmVoiceNoiseSuppressEnable` | 1 (Simple) | 2 (RNNoise) | 模式切换 | 所有未配置用户 |
| `QmVoiceNoiseSuppressStrength` | 50 | 35 | -30% | 降噪强度 |
| `QmVoiceCompThreshold` | 20 | 24 | +20% | 压缩阈值 |
| `QmVoiceCompRatio` | 2.5 | 2.0 | -20% | 压缩比 |
| `QmVoiceCompAttackMs` | 20 | 12 | -40% | 攻击时间 |
| `QmVoiceCompReleaseMs` | 200 | 140 | -30% | 释放时间 |
| `QmVoiceCompMakeup` | 160 | 125 | -22% | 补偿增益 |
| `QmVoiceLimiter` | 50 | 92 | +84% | 限幅阈值 |
| `QmVoiceAgcEnable` | (新增) | 0 | 关闭 | AGC 开关 |

---

## 任务 1：冻结编码目标与默认参数的纯函数语义

**文件：**
- 修改：`src/game/client/components/qmclient/voice_utils.h`
- 修改：`src/game/client/components/qmclient/voice_utils.cpp`
- 修改：`src/test/voice_core_test.cpp`

- [ ] **步骤 1：声明语音出厂默认参数结构**

在 `src/game/client/components/qmclient/voice_utils.h` 中添加结构。

**语义定义：**
- `SVoiceProcessingFactoryDefaults` = 单一真值源，表达这轮 roadmap 推荐的“出厂默认参数”
- `config_variables_qmclient.h` = 新用户首次启动时的默认配置值，必须与 factory defaults 保持一致
- `voice_core.cpp` 运行时 = 始终读取 config snapshot，不在运行路径里额外兜底第二套默认值
- 若未来需要“恢复出厂设置”能力，应显式从 `SVoiceProcessingFactoryDefaults` 生成，而不是在 runtime 路径里隐式回退

```cpp
struct SVoiceProcessingFactoryDefaults
{
	int m_NoiseSuppressMode = VOICE_NOISE_SUPPRESS_RNNOISE;
	int m_NoiseSuppressStrength = 35;
	float m_HpfCutoffHz = 120.0f;
	float m_CompressorThreshold = 0.24f;
	float m_CompressorRatio = 2.0f;
	float m_CompressorAttackSec = 0.012f;
	float m_CompressorReleaseSec = 0.140f;
	float m_CompressorMakeupGain = 1.25f;
	float m_Limiter = 0.92f;
	int m_EncoderComplexity = 8;
};

struct SVoiceAgcConfig
{
	bool m_Enable = false;
	float m_TargetRms = 0.18f;
	float m_MaxGain = 2.0f;
	float m_MinGain = 0.75f;
	float m_AttackSec = 0.050f;
	float m_ReleaseSec = 0.350f;
};
```

- [ ] **步骤 2：声明 helper 接口**

继续在 `voice_utils.h` 中声明以下接口。

**关于 `ComputeVoiceEncoderTargetsWithComplexity` 与现有 `ComputeVoiceEncoderTargets` 的关系：**
- 现有 `ComputeVoiceEncoderTargets` **保留**，用于向后兼容和渐进迁移
- 新增 `ComputeVoiceEncoderTargetsWithComplexity` 作为增强版本，增加 complexity 输出
- 内部实现：`ComputeVoiceEncoderTargets` 调用 `ComputeVoiceEncoderTargetsWithComplexity` 并忽略 complexity

```cpp
SVoiceProcessingFactoryDefaults VoiceProcessingFactoryDefaults();
SVoiceAgcConfig VoiceAgcConfigFromRuntime(bool EnableAgc);
void ComputeVoiceEncoderTargetsWithComplexity(int LossPerc, float JitterMax, int BitrateProfile, int *pTargetBitrate, int *pTargetLoss, bool *pTargetFec, int *pTargetComplexity);
float ComputeVoiceAutoGain(float CurrentGain, float FrameRms, const SVoiceAgcConfig &Config);
```

- [ ] **步骤 3：为新策略写失败测试**

在 `src/test/voice_core_test.cpp` 中新增至少以下测试：

```cpp
TEST(VoiceUtils, VoiceProcessingFactoryDefaultsUseModerateRnnoiseStrength)
{
	const auto Defaults = VoiceProcessingFactoryDefaults();
	EXPECT_EQ(Defaults.m_NoiseSuppressMode, VOICE_NOISE_SUPPRESS_RNNOISE);
	EXPECT_EQ(Defaults.m_NoiseSuppressStrength, 35);
	EXPECT_EQ(Defaults.m_EncoderComplexity, 8);
}

TEST(VoiceUtils, ComputeVoiceEncoderTargetsWithComplexityHealthyNetworkKeepsHighQuality)
{
	int Bitrate = 0;
	int Loss = 0;
	bool Fec = false;
	int Complexity = 0;
	ComputeVoiceEncoderTargetsWithComplexity(0, 0.0f, 0, &Bitrate, &Loss, &Fec, &Complexity);
	EXPECT_EQ(Bitrate, 64000);
	EXPECT_EQ(Loss, 0);
	EXPECT_FALSE(Fec);
	EXPECT_EQ(Complexity, 8);
}

TEST(VoiceUtils, ComputeVoiceAutoGainRaisesQuietFramesButHonorsMaxGain)
{
	const auto Config = VoiceAgcConfigFromRuntime(true);
	const float Next = ComputeVoiceAutoGain(1.0f, 0.05f, Config);
	EXPECT_GT(Next, 1.0f);
	EXPECT_LE(Next, Config.m_MaxGain);
}

TEST(VoiceUtils, ComputeVoiceEncoderTargetsWithComplexityBackwardCompatibleWithOldFunction)
{
	int BitrateOld = 0, LossOld = 0;
	bool FecOld = false;
	ComputeVoiceEncoderTargets(5, 10.0f, 0, &BitrateOld, &LossOld, &FecOld);

	int BitrateNew = 0, LossNew = 0, ComplexityNew = 0;
	bool FecNew = false;
	ComputeVoiceEncoderTargetsWithComplexity(5, 10.0f, 0, &BitrateNew, &LossNew, &FecNew, &ComplexityNew);

	EXPECT_EQ(BitrateOld, BitrateNew);
	EXPECT_EQ(LossOld, LossNew);
	EXPECT_EQ(FecOld, FecNew);
}
```

- [ ] **步骤 4：实现 helper 并保持为纯函数**

在 `src/game/client/components/qmclient/voice_utils.cpp` 中实现：

```cpp
SVoiceProcessingFactoryDefaults VoiceProcessingFactoryDefaults()
{
	return SVoiceProcessingFactoryDefaults();
}

SVoiceAgcConfig VoiceAgcConfigFromRuntime(bool EnableAgc)
{
	SVoiceAgcConfig Config;
	Config.m_Enable = EnableAgc;
	return Config;
}
```

**实现要求：**

- `ComputeVoiceEncoderTargetsWithComplexity` 不访问全局状态
- `ComputeVoiceAutoGain` 只依赖传入 gain、frame RMS 和 config
- manual bitrate profile 仍保持覆盖 adaptive table 的语义
- `ComputeVoiceEncoderTargetsWithComplexity` 必须把 adaptive table 明确写死为可读分段
- `ComputeVoiceAutoGain` 必须是"慢速、有限幅、向 unity 回归"的增益曲线

**更新现有 `ComputeVoiceEncoderTargets` 以复用新实现：**

```cpp
void ComputeVoiceEncoderTargets(int LossPerc, float JitterMax, int BitrateProfile, int *pTargetBitrate, int *pTargetLoss, bool *pTargetFec)
{
	int Complexity = 0;
	ComputeVoiceEncoderTargetsWithComplexity(LossPerc, JitterMax, BitrateProfile, pTargetBitrate, pTargetLoss, pTargetFec, &Complexity);
}
```

`ComputeVoiceEncoderTargetsWithComplexity` 实现（adaptive table 部分）：

```cpp
void ComputeVoiceEncoderTargetsWithComplexity(int LossPerc, float JitterMax, int BitrateProfile, int *pTargetBitrate, int *pTargetLoss, bool *pTargetFec, int *pTargetComplexity)
{
	if(!pTargetBitrate || !pTargetLoss || !pTargetFec || !pTargetComplexity)
		return;

	const int Profile = std::clamp(BitrateProfile, 0, 4);
	
	static constexpr int s_aManualBitrates[] = {0, 24000, 32000, 48000, 64000};
	if(Profile >= 1 && Profile <= 4)
	{
		*pTargetBitrate = s_aManualBitrates[Profile];
		*pTargetLoss = 0;
		*pTargetFec = false;
		*pTargetComplexity = 8;
		return;
	}

	const int ClampedLoss = std::clamp(LossPerc, 0, 30);
	const float ClampedJitter = std::isfinite(JitterMax) ? std::max(0.0f, JitterMax) : 0.0f;
	
	if(ClampedLoss <= 2 && ClampedJitter < 8.0f)
	{
		*pTargetBitrate = 64000;
		*pTargetLoss = 0;
		*pTargetFec = false;
		*pTargetComplexity = 8;
	}
	else if(ClampedLoss <= 5 && ClampedJitter < 16.0f)
	{
		*pTargetBitrate = 48000;
		*pTargetLoss = 5;
		*pTargetFec = true;
		*pTargetComplexity = 8;
	}
	else if(ClampedLoss <= 10 && ClampedJitter < 28.0f)
	{
		*pTargetBitrate = 32000;
		*pTargetLoss = 10;
		*pTargetFec = true;
		*pTargetComplexity = 7;
	}
	else
	{
		*pTargetBitrate = 24000;
		*pTargetLoss = 20;
		*pTargetFec = true;
		*pTargetComplexity = 6;
	}
}
```

`ComputeVoiceAutoGain` 实现：

```cpp
float ComputeVoiceAutoGain(float CurrentGain, float FrameRms, const SVoiceAgcConfig &Config)
{
	if(!Config.m_Enable)
		return 1.0f;

	const float SafeRms = std::clamp(FrameRms, 0.0001f, 1.0f);
	const float TargetGain = std::clamp(Config.m_TargetRms / SafeRms, Config.m_MinGain, Config.m_MaxGain);
	const bool Raising = TargetGain > CurrentGain;
	const float Slew = Raising ? 0.18f : 0.06f;
	const float NextGain = CurrentGain + (TargetGain - CurrentGain) * Slew;
	const float TowardUnity = NextGain + (1.0f - NextGain) * 0.01f;
	return std::clamp(TowardUnity, Config.m_MinGain, Config.m_MaxGain);
}
```

- [ ] **步骤 5：运行定向测试**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- 新增测试全部通过
- 现有 `ComputeVoiceEncoderTargets` 测试仍然通过（向后兼容验证）

- [ ] **步骤 6：可选检查点**

如果当前工作区适合分段提交，可在这里作为一个里程碑单独提交。

---

## 任务 2：显式接入 Opus complexity 与更新后的编码目标表

**文件：**
- 修改：`src/game/client/components/qmclient/voice_core.h`
- 修改：`src/game/client/components/qmclient/voice_core.cpp`
- 修改：`src/test/voice_core_test.cpp`

- [ ] **步骤 1：扩展编码器运行时状态**

在 `src/game/client/components/qmclient/voice_core.h` 的 `CRClientVoice` 类中，在现有编码器状态字段后新增：

```cpp
int m_EncBitrate = 36000;
int m_EncLossPerc = 0;
bool m_EncFec = false;
int m_EncComplexity = 8;  // 新增
```

- [ ] **步骤 2：在 encoder 创建后显式设置 complexity**

在 `src/game/client/components/qmclient/voice_core.cpp` 的 `EnsureAudio()` 中，找到 encoder 初始化代码块，将：

```cpp
VoiceUtils::ComputeVoiceEncoderTargets(0, 0.0f, g_Config.m_QmVoiceBitrateProfile, &m_EncBitrate, &m_EncLossPerc, &m_EncFec);
opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(m_EncBitrate));
opus_encoder_ctl(m_pEncoder, OPUS_SET_PACKET_LOSS_PERC(m_EncLossPerc));
opus_encoder_ctl(m_pEncoder, OPUS_SET_INBAND_FEC(m_EncFec ? 1 : 0));
```

替换为：

```cpp
VoiceUtils::ComputeVoiceEncoderTargetsWithComplexity(0, 0.0f, g_Config.m_QmVoiceBitrateProfile, &m_EncBitrate, &m_EncLossPerc, &m_EncFec, &m_EncComplexity);
opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(m_EncBitrate));
opus_encoder_ctl(m_pEncoder, OPUS_SET_PACKET_LOSS_PERC(m_EncLossPerc));
opus_encoder_ctl(m_pEncoder, OPUS_SET_INBAND_FEC(m_EncFec ? 1 : 0));
const int ComplexityResult = opus_encoder_ctl(m_pEncoder, OPUS_SET_COMPLEXITY(m_EncComplexity));
if(ComplexityResult != OPUS_OK)
{
	log_warn("voice", "OPUS_SET_COMPLEXITY(%d) failed during encoder init with error %d",
	         m_EncComplexity, ComplexityResult);
}
```

初始化阶段要求：

- `OPUS_SET_COMPLEXITY` 在 encoder init 阶段也必须检查返回值
- 初始化失败时必须记录 `log_warn`
- 初始化阶段与运行时更新阶段使用一致的错误处理口径，不能只补其中一条路径
- 若后续需要区分“期望 complexity”和“已应用 complexity”，应在实现时明确拆分状态字段；本轮至少不能静默吞错

- [ ] **步骤 3：更新自适应编码参数刷新逻辑**

在 `UpdateEncoderParams()` 中，将：

```cpp
int TargetBitrate = m_EncBitrate;
int TargetLoss = 0;
bool TargetFec = false;
VoiceUtils::ComputeVoiceEncoderTargets(LossPerc, JitterMax, Config.m_QmVoiceBitrateProfile, &TargetBitrate, &TargetLoss, &TargetFec);
```

替换为：

```cpp
int TargetBitrate = m_EncBitrate;
int TargetLoss = 0;
bool TargetFec = false;
int TargetComplexity = m_EncComplexity;
VoiceUtils::ComputeVoiceEncoderTargetsWithComplexity(LossPerc, JitterMax, Config.m_QmVoiceBitrateProfile, &TargetBitrate, &TargetLoss, &TargetFec, &TargetComplexity);
```

并新增 complexity 更新逻辑：

```cpp
if(TargetComplexity != m_EncComplexity)
{
	const int Result = opus_encoder_ctl(m_pEncoder, OPUS_SET_COMPLEXITY(TargetComplexity));
	if(Result == OPUS_OK)
	{
		m_EncComplexity = TargetComplexity;
	}
	else
	{
		log_warn("voice", "OPUS_SET_COMPLEXITY(%d) failed with error %d, keeping previous complexity %d",
		         TargetComplexity, Result, m_EncComplexity);
	}
}
```

**错误处理要求：**
- 使用 `log_warn` 级别（非致命错误）
- 日志格式：`OPUS_SET_COMPLEXITY(%d) failed with error %d, keeping previous complexity %d`
- 失败时不更新 `m_EncComplexity` 本地状态

- [ ] **步骤 4：补编码器复杂度行为测试**

在 `src/test/voice_core_test.cpp` 中增加：

```cpp
TEST(VoiceCore, ComputeVoiceEncoderTargetsWithComplexityManualProfileUsesStableComplexity)
{
	int Bitrate = 0;
	int Loss = 0;
	bool Fec = false;
	int Complexity = 0;
	ComputeVoiceEncoderTargetsWithComplexity(20, 40.0f, 4, &Bitrate, &Loss, &Fec, &Complexity);
	EXPECT_EQ(Bitrate, 64000);
	EXPECT_EQ(Loss, 0);
	EXPECT_FALSE(Fec);
	EXPECT_EQ(Complexity, 8);
}

TEST(VoiceCore, ComputeVoiceEncoderTargetsWithComplexityReducesComplexityOnPoorNetwork)
{
	int Bitrate1 = 0, Loss1 = 0, Complexity1 = 0;
	bool Fec1 = false;
	ComputeVoiceEncoderTargetsWithComplexity(0, 0.0f, 0, &Bitrate1, &Loss1, &Fec1, &Complexity1);
	EXPECT_EQ(Complexity1, 8);

	int Bitrate2 = 0, Loss2 = 0, Complexity2 = 0;
	bool Fec2 = false;
	ComputeVoiceEncoderTargetsWithComplexity(15, 35.0f, 0, &Bitrate2, &Loss2, &Fec2, &Complexity2);
	EXPECT_EQ(Complexity2, 6);
	EXPECT_LT(Complexity2, Complexity1);
}
```

- [ ] **步骤 5：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `game-client` 编译通过
- 所有编码目标与 complexity 测试通过

- [ ] **步骤 6：可选检查点**

如需保留中间检查点，可在这里提交。

---

## 任务 3：固定发送侧处理链顺序并接入轻量 AGC

**文件：**
- 修改：`src/game/client/components/qmclient/voice_core.h`
- 修改：`src/game/client/components/qmclient/voice_core.cpp`
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/test/voice_core_test.cpp`

- [ ] **步骤 1：新增 AGC 配置项**

在 `src/engine/shared/config_variables_qmclient.h` 中新增：

```cpp
MACRO_CONFIG_INT(QmVoiceAgcEnable, qm_voice_agc_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动增益控制（0=关闭 1=开启）")
```

**本轮语义：** 默认关闭（值为 0）。AGC 功能实现但不默认启用，降低默认行为突变风险。

- [ ] **步骤 2：扩展配置快照结构**

在 `src/game/client/components/qmclient/voice_core.h` 的 `SRClientVoiceConfigSnapshot` 结构体中新增：

```cpp
int m_QmVoiceAgcEnable = 0;
```

并在 `voice_core.cpp` 的 `UpdateConfigSnapshot()` 或 `GetConfigSnapshot()` 实现中读取该配置：

```cpp
Snap.m_QmVoiceAgcEnable = g_Config.m_QmVoiceAgcEnable;
```

- [ ] **步骤 3：扩展 AGC 运行时状态**

在 `src/game/client/components/qmclient/voice_core.h` 的 `CRClientVoice` 类中新增：

```cpp
float m_AgcGain = 1.0f;
```

并在 `ResetTransmitState()` 中重置：

```cpp
m_AgcGain = 1.0f;
```

- [ ] **步骤 4：提取发送侧单帧处理 helper**

在 `src/game/client/components/qmclient/voice_core.cpp` 的匿名命名空间中添加：

```cpp
static void ProcessVoiceCaptureFrame(
	const SRClientVoiceConfigSnapshot &Config,
	int16_t *pSamples,
	int Count,
	float &AgcGain,
	float &NoiseFloor,
	float &NoiseGate,
	DenoiseState *&pNoiseState,
	bool &NoiseFallbackLogged,
	float &HpfPrevIn,
	float &HpfPrevOut,
	float &CompEnv)
{
	const auto AgcConfig = VoiceUtils::VoiceAgcConfigFromRuntime(Config.m_QmVoiceAgcEnable != 0);
	const float FrameRms = VoiceUtils::VoiceFrameRms(pSamples, Count);
	AgcGain = VoiceUtils::ComputeVoiceAutoGain(AgcGain, FrameRms, AgcConfig);
	
	const float MicGain = std::clamp(Config.m_QmVoiceMicVolume / 100.0f, 0.0f, 3.0f) * AgcGain;
	VoiceUtils::ApplyMicGain(MicGain, pSamples, Count);
	
	ApplyNoiseSuppressor(Config, pSamples, Count, NoiseFloor, NoiseGate, pNoiseState, NoiseFallbackLogged);
	ApplyHpfCompressor(Config, pSamples, Count, HpfPrevIn, HpfPrevOut, CompEnv);
}
```

**处理顺序（必须严格遵循）：**
1. AGC 计算增益（基于原始输入 RMS）
2. Mic Gain 应用（AGC 增益 × 用户音量）
3. 降噪处理
4. 动态处理（HPF + 压缩器）
5. VAD 计算（基于处理后样本）

- [ ] **步骤 5：让所有发送侧路径都调用统一 helper**

把 `ProcessCapture()` 中当前两处重复逻辑替换为：

```cpp
ProcessVoiceCaptureFrame(Config, aPcm, VOICE_FRAME_SAMPLES, m_AgcGain, m_NsNoiseFloor, m_NsGain, m_pNoiseSuppress, m_NoiseSuppressFallbackLogged, m_HpfPrevIn, m_HpfPrevOut, m_CompEnv);
```

要求：

- mic level 展示路径与实际发送路径使用同一处理顺序
- VAD 继续基于处理后样本计算 `Peak`

- [ ] **步骤 6：补 AGC 与处理顺序测试**

在 `src/test/voice_core_test.cpp` 中添加：

```cpp
TEST(VoiceUtils, ComputeVoiceAutoGainFallsBackTowardUnityForLoudFrames)
{
	const auto Config = VoiceAgcConfigFromRuntime(true);
	const float Next = ComputeVoiceAutoGain(1.8f, 0.35f, Config);
	EXPECT_LT(Next, 1.8f);
	EXPECT_GE(Next, Config.m_MinGain);
}

TEST(VoiceUtils, ComputeVoiceAutoGainDisabledReturnsUnity)
{
	const auto Config = VoiceAgcConfigFromRuntime(false);
	const float Next = ComputeVoiceAutoGain(1.5f, 0.05f, Config);
	EXPECT_FLOAT_EQ(Next, 1.0f);
}
```

**真实的处理顺序测试（通过可注入的 trace callback）：**

在 `voice_utils.h` 中声明 trace 接口：

```cpp
enum class EVoiceProcessStage
{
	AGC_GAIN,
	MIC_GAIN,
	DENOISE,
	HPF_COMPRESSOR,
};

using VoiceProcessTraceCallback = void(*)(EVoiceProcessStage, void *pUserData);

void SetVoiceProcessTraceCallback(VoiceProcessTraceCallback pCallback, void *pUserData);
void TraceVoiceProcessStage(EVoiceProcessStage Stage);
```

在 `voice_utils.cpp` 中实现：

```cpp
static VoiceProcessTraceCallback s_pVoiceProcessTraceCallback = nullptr;
static void *s_pVoiceProcessTraceUserData = nullptr;

void SetVoiceProcessTraceCallback(VoiceProcessTraceCallback pCallback, void *pUserData)
{
	s_pVoiceProcessTraceCallback = pCallback;
	s_pVoiceProcessTraceUserData = pUserData;
}

void TraceVoiceProcessStage(EVoiceProcessStage Stage)
{
	if(s_pVoiceProcessTraceCallback)
		s_pVoiceProcessTraceCallback(Stage, s_pVoiceProcessTraceUserData);
}
```

修改 `ProcessVoiceCaptureFrame` 在每个阶段通过 `VoiceUtils::TraceVoiceProcessStage(...)` 调用 trace：

```cpp
static void ProcessVoiceCaptureFrame(...)
{
	VoiceUtils::TraceVoiceProcessStage(EVoiceProcessStage::AGC_GAIN);
	
	// AGC 计算...
	
	VoiceUtils::TraceVoiceProcessStage(EVoiceProcessStage::MIC_GAIN);
	
	// Mic Gain 应用...
	
	VoiceUtils::TraceVoiceProcessStage(EVoiceProcessStage::DENOISE);
	
	// 降噪...
	
	VoiceUtils::TraceVoiceProcessStage(EVoiceProcessStage::HPF_COMPRESSOR);
	
	// 动态处理...
}
```

要求：

- `voice_core.cpp` 不允许直接访问 `voice_utils.cpp` 内部静态变量
- trace 状态的拥有者固定为 `voice_utils.cpp`
- `voice_core.cpp` 只能通过头文件暴露的 `SetVoiceProcessTraceCallback(...)` / `TraceVoiceProcessStage(...)` 交互

**暴露测试专用接口：**

由于 `ProcessVoiceCaptureFrame` 是 `voice_core.cpp` 匿名命名空间中的 `static` 函数，测试代码无法直接调用。需要在 `voice_core.h` 中声明测试专用接口：

```cpp
// voice_core.h 中，在 CRClientVoice 类声明之后

#ifdef CONF_TEST
// 测试专用接口，仅用于验证处理链顺序
// 生产代码不应调用此接口
void ProcessVoiceCaptureFrame_ForTest(
	const SRClientVoiceConfigSnapshot &Config,
	int16_t *pSamples,
	int Count,
	float &AgcGain,
	float &NoiseFloor,
	float &NoiseGate,
	DenoiseState *&pNoiseState,
	bool &NoiseFallbackLogged,
	float &HpfPrevIn,
	float &HpfPrevOut,
	float &CompEnv);
#endif
```

在 `voice_core.cpp` 中实现：

```cpp
#ifdef CONF_TEST
void ProcessVoiceCaptureFrame_ForTest(
	const SRClientVoiceConfigSnapshot &Config,
	int16_t *pSamples,
	int Count,
	float &AgcGain,
	float &NoiseFloor,
	float &NoiseGate,
	DenoiseState *&pNoiseState,
	bool &NoiseFallbackLogged,
	float &HpfPrevIn,
	float &HpfPrevOut,
	float &CompEnv)
{
	ProcessVoiceCaptureFrame(Config, pSamples, Count, AgcGain, NoiseFloor, NoiseGate,
	                         pNoiseState, NoiseFallbackLogged, HpfPrevIn, HpfPrevOut, CompEnv);
}
#endif
```

测试代码：

```cpp
TEST(VoiceCore, CaptureProcessOrderIsAgcThenMicGainThenDenoiseThenDynamics)
{
	std::vector<EVoiceProcessStage> vStages;
	SetVoiceProcessTraceCallback([](EVoiceProcessStage Stage, void *pData){
		auto *pStages = static_cast<std::vector<EVoiceProcessStage>*>(pData);
		pStages->push_back(Stage);
	}, &vStages);

	SRClientVoiceConfigSnapshot Config;
	Config.m_QmVoiceAgcEnable = 1;
	Config.m_QmVoiceMicVolume = 100;
	Config.m_QmVoiceNoiseSuppressEnable = 1;
	Config.m_QmVoiceNoiseSuppressStrength = 50;
	Config.m_QmVoiceFilterEnable = 1;

	int16_t aSamples[VOICE_FRAME_SAMPLES] = {};
	float AgcGain = 1.0f, NoiseFloor = 0.0f, NoiseGate = 1.0f;
	float HpfPrevIn = 0.0f, HpfPrevOut = 0.0f, CompEnv = 0.0f;
	DenoiseState *pNoiseState = nullptr;
	bool NoiseFallbackLogged = false;

	ProcessVoiceCaptureFrame_ForTest(Config, aSamples, VOICE_FRAME_SAMPLES, AgcGain, NoiseFloor, NoiseGate,
	                                  pNoiseState, NoiseFallbackLogged, HpfPrevIn, HpfPrevOut, CompEnv);

	SetVoiceProcessTraceCallback(nullptr, nullptr);

	ASSERT_EQ(vStages.size(), 4u);
	EXPECT_EQ(vStages[0], EVoiceProcessStage::AGC_GAIN);
	EXPECT_EQ(vStages[1], EVoiceProcessStage::MIC_GAIN);
	EXPECT_EQ(vStages[2], EVoiceProcessStage::DENOISE);
	EXPECT_EQ(vStages[3], EVoiceProcessStage::HPF_COMPRESSOR);
}
```

**前置条件说明：**
- 此测试依赖 `CONF_TEST` 宏定义（DDNet 测试构建时自动定义）
- 测试运行时需要配置系统已初始化（`g_Config` 可用）

- [ ] **步骤 7：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- `voice_core.cpp` 编译通过
- AGC 边界测试通过
- 处理顺序测试通过

- [ ] **步骤 8：可选检查点**

如需降低返工风险，可在这里保留一个处理链改造检查点。

---

## 任务 4：把默认参数落实到现有配置与 UI 语义

**文件：**
- 修改：`src/engine/shared/config_variables_qmclient.h`
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`
- 修改：`src/test/voice_core_test.cpp`

- [x] **步骤 1：更新配置默认值**

在 `src/engine/shared/config_variables_qmclient.h` 中更新默认值：

```cpp
MACRO_CONFIG_INT(QmVoiceNoiseSuppressEnable, qm_voice_noise_suppress_enable, 2, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "噪声抑制模式（0=关闭 1=简单 2=RNNoise）")
MACRO_CONFIG_INT(QmVoiceNoiseSuppressStrength, qm_voice_noise_suppress_strength, 35, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "噪声抑制强度（百分比）")
MACRO_CONFIG_INT(QmVoiceCompThreshold, qm_voice_comp_threshold, 24, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器阈值（百分比）")
MACRO_CONFIG_INT(QmVoiceCompRatio, qm_voice_comp_ratio, 20, 10, 80, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器比率（x10）")
MACRO_CONFIG_INT(QmVoiceCompAttackMs, qm_voice_comp_attack_ms, 12, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器攻击时间（毫秒）")
MACRO_CONFIG_INT(QmVoiceCompReleaseMs, qm_voice_comp_release_ms, 140, 10, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器释放时间（毫秒）")
MACRO_CONFIG_INT(QmVoiceCompMakeup, qm_voice_comp_makeup, 125, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器补偿增益（百分比）")
MACRO_CONFIG_INT(QmVoiceLimiter, qm_voice_limiter, 92, 10, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "限幅器阈值（百分比）")
```

**参数来源语义确认：**
- `SVoiceProcessingFactoryDefaults` = 出厂推荐值（代码中硬编码的"理想"值）
- `config_variables_qmclient.h` 默认值 = 新用户首次启动时的值
- 两者必须保持一致

- [x] **步骤 2：让语音设置页提示文案与默认策略一致**

在 `src/game/client/components/qmclient/menus_qmclient.cpp` 的 voice 模块区域，补一段只读说明：

```cpp
Ui()->DoLabel(&HintRow, Localize("默认配置已偏向更自然的人声与更稳的响度；如果你的环境底噪较高，可提高噪声抑制强度。AGC 默认关闭，可在需要时手动开启。"), LG_BodySize, TEXTALIGN_ML);
```

要求：

- 不新增 AEC 或双声道上行控制项
- 不把"后续规划中的能力"伪装成现有功能
- AGC 开关应显示但标注"实验性"

- [x] **步骤 3：补默认值语义测试**

在 `src/test/voice_core_test.cpp` 中新增：

```cpp
TEST(VoiceUtils, VoiceProcessingFactoryDefaultsMatchConfigDefaults)
{
	const auto Defaults = VoiceProcessingFactoryDefaults();
	
	EXPECT_EQ(Defaults.m_NoiseSuppressMode, VOICE_NOISE_SUPPRESS_RNNOISE);
	EXPECT_EQ(Defaults.m_NoiseSuppressStrength, 35);
	EXPECT_NEAR(Defaults.m_CompressorThreshold, 0.24f, 0.001f);
	EXPECT_NEAR(Defaults.m_CompressorRatio, 2.0f, 0.001f);
	EXPECT_NEAR(Defaults.m_CompressorAttackSec, 0.012f, 0.001f);
	EXPECT_NEAR(Defaults.m_CompressorReleaseSec, 0.140f, 0.001f);
	EXPECT_NEAR(Defaults.m_CompressorMakeupGain, 1.25f, 0.001f);
	EXPECT_NEAR(Defaults.m_Limiter, 0.92f, 0.001f);
	EXPECT_EQ(Defaults.m_EncoderComplexity, 8);
}

TEST(VoiceCore, ConfigDefaultsMatchFactoryDefaults)
{
	EXPECT_EQ(g_Config.m_QmVoiceNoiseSuppressEnable, 2);
	EXPECT_EQ(g_Config.m_QmVoiceNoiseSuppressStrength, 35);
	EXPECT_EQ(g_Config.m_QmVoiceCompThreshold, 24);
	EXPECT_EQ(g_Config.m_QmVoiceCompRatio, 20);
	EXPECT_EQ(g_Config.m_QmVoiceCompAttackMs, 12);
	EXPECT_EQ(g_Config.m_QmVoiceCompReleaseMs, 140);
	EXPECT_EQ(g_Config.m_QmVoiceCompMakeup, 125);
	EXPECT_EQ(g_Config.m_QmVoiceLimiter, 92);
	EXPECT_EQ(g_Config.m_QmVoiceAgcEnable, 0);
}
```

- [x] **步骤 4：构建验证**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- 设置页编译通过
- 默认值对齐测试通过

- [ ] **步骤 5：可选检查点**

如需要，可在默认值与 UI 对齐后提交一个里程碑。

---

## 任务 5：补主线验证并记录近期不做项

**文件：**
- 修改：`docs/superpowers/plans/2026-04-30-voice-quality-roadmap.md`

- [x] **步骤 1：运行 C++ 测试**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10 | Select-Object -Last 120
```

预期：

- 所有 `voice` 相关测试通过

- [x] **步骤 2：构建客户端**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 80
```

预期：

- `game-client` 构建成功

- [ ] **步骤 3：做最小双端 / loopback 验证**

从构建目录运行：

```powershell
Set-Location build-ninja
.\DDNet.exe
```

**AB 对比验证：**

先冻结两组参数配置：

```powershell
# 旧默认基线（在 console 中执行）
qm_voice_noise_suppress_enable 1
qm_voice_noise_suppress_strength 50
qm_voice_comp_threshold 20
qm_voice_comp_ratio 25
qm_voice_comp_attack_ms 20
qm_voice_comp_release_ms 200
qm_voice_comp_makeup 160
qm_voice_limiter 50
qm_voice_agc_enable 0

# 新默认候选（删除 settings_ddnet.cfg 后重启，或手动设置）
qm_voice_noise_suppress_enable 2
qm_voice_noise_suppress_strength 35
qm_voice_comp_threshold 24
qm_voice_comp_ratio 20
qm_voice_comp_attack_ms 12
qm_voice_comp_release_ms 140
qm_voice_comp_makeup 125
qm_voice_limiter 92
qm_voice_agc_enable 0
```

**测试话术：**
- 话术 A：`12345 testing one two three`
- 话术 B：`freeze hook laser grenade ninja`
- 话术 C：`ssss shhh chhh ppp ttt`

**测试场景（每场景 3 轮 AB 对比）：**

| 场景 | 验证重点 |
|------|----------|
| 安静环境，正常音量 | 基准听感对比 |
| 安静环境，小声说话 | AGC 效果（若开启）、响度一致性 |
| 安静环境，大声说话 | 限幅器效果、削顶检测 |
| 键盘连续敲击背景 | RNNoise 降噪效果、吞尾检测 |
| 弱网 / 抖动场景 | 编码自适应行为 |

**客观验证指标：**

1. **波形 RMS 差异**：录制同话术，计算新旧配置的 RMS 差异，记录数值
2. **削顶检测**：大声说话场景，统计样本值达到 ±32767 的帧数，新配置应 ≤ 旧配置
3. **RNNoise VAD 概率**：键盘背景场景，记录 RNNoise 输出的 VAD 概率，话术 A 尾音期间 VAD > 0.5 的帧占比应 > 80%

- [ ] **步骤 4：弱网验证（可选但有条件执行）**

**方案 A：使用 Clumsi 模拟弱网（Windows 推荐）**

1. 下载 Clumsi: https://jagt.github.io/clumsy/
2. 启动 Clumsi，设置：
   - Filter: Outbound
   - Lag: 勾选，设置 50-100ms
   - Drop: 勾选，设置 5-15%
   - Out of order: 勾选，设置 5%
3. 启动游戏，观察 voice debug 日志中 `ComputeVoiceEncoderTargetsWithComplexity` 的参数切换

**方案 B：使用内置调试日志验证**

在 `voice_core.cpp` 的 `UpdateEncoderParams()` 中增加临时调试日志：

```cpp
if(TargetBitrate != m_EncBitrate || TargetComplexity != m_EncComplexity)
{
	log_debug("voice", "Encoder params change: bitrate %d->%d, complexity %d->%d, loss %d, fec %d",
	          m_EncBitrate, TargetBitrate, m_EncComplexity, TargetComplexity, TargetLoss, TargetFec);
}
```

通过手动断网或限速触发参数切换，验证日志输出符合预期。

**弱网通过标准：**
- 丢包率 > 5% 时，bitrate 应下降，FEC 应开启
- 抖动 > 16ms 时，应进入更保守档位
- 不允许出现"健康网络更好听、弱网直接崩坏"的情况

**执行口径收紧：**

- 如果环境允许安装/运行 Clumsi 或其他限网工具，弱网验证为**必做**
- 如果当前环境明确无法安装外部工具，至少必须执行方案 B 的日志验证，不能整项跳过
- 只有在“外部工具不可用 + 本地也无法制造可观测 loss/jitter 条件”时，才允许把弱网验证标记为 `跳过`，并在验证记录里写明具体阻塞原因

- [ ] **步骤 5：CPU / 稳定性观察**

打开 voice debug 日志，记录：

```powershell
# 在 game console 中
qm_voice_debug 1
```

观察指标：
- Encoder 参数切换频率
- Packet backlog 变化
- Queued frames/packets 数量
- Worker 线程 CPU 占用

**通过标准：**
- Complexity 8 下，CPU 占用增幅 < 5%
- 无持续性 backlog 增长
- 无 worker 线程卡顿

- [x] **步骤 6：回写验证记录**

在本计划底部追加：

```md
## 验证记录

- run_cxx_tests：[ ] 通过 / [ ] 失败
- game-client：[ ] 通过 / [ ] 失败
- 旧默认基线：[ ] 已冻结
- loopback / 双端验证：[ ] 完成 / [ ] 失败原因
- 弱网 / 抖动验证：[ ] 完成 / [ ] 跳过 / [ ] 失败原因
- CPU / 实时稳定性观察：[ ] 通过 / [ ] 失败原因

### 测试数据记录

| 场景 | 旧配置 RMS | 新配置 RMS | 削顶帧数(旧) | 削顶帧数(新) | 备注 |
|------|-----------|-----------|-------------|-------------|------|
| 安静正常 | | | | | |
| 安静小声 | | | | | |
| 安静大声 | | | | | |
| 键盘背景 | | | | | |
```

- [x] **步骤 7：确认近期不做项仍保持未实现状态**

复核并记录以下两项没有被夹带实现：

1. 真正双声道采集 / 编码
2. 默认开启的 AEC

要求：

- 如果代码中出现相关半成品开关，不能在本任务里声称"已支持"
- 只允许保留文档中的规划定位

- [ ] **步骤 8：收尾提交**

验证完成后，根据当前分支与工作区状态决定提交策略。

---

## 回滚计划

如果验证失败需要回滚：

1. **配置回滚**：恢复 `config_variables_qmclient.h` 中的旧默认值
2. **代码回滚**：使用 `git revert` 或 `git reset` 回退相关提交
3. **用户配置迁移**：如果用户已保存新配置，提供配置重置说明

```powershell
# 用户可通过 console 重置为旧默认
qm_voice_noise_suppress_enable 1
qm_voice_noise_suppress_strength 50
qm_voice_comp_threshold 20
qm_voice_comp_ratio 25
qm_voice_comp_attack_ms 20
qm_voice_comp_release_ms 200
qm_voice_comp_makeup 160
qm_voice_limiter 50
```

---

## 自检清单

### 规格覆盖度

本计划已覆盖规格中的核心要求：

- 显式 `Opus complexity`
- 编码目标表优化
- 默认动态处理参数优化
- 轻量 AGC（本轮默认关闭）
- RNNoise 默认策略调整
- AEC 仅保留为后续预留
- 真正双声道采集 / 编码仅保留为后续专题

### 关键风险已识别

- [x] 默认值大幅调整已明确声明
- [x] AGC 配置项已声明
- [x] 新旧函数关系已定义（V2 增强，旧函数保留并委托）
- [x] 处理顺序测试方案已提供（trace callback 注入）
- [x] 弱网验证方案已提供（Clumsi 或调试日志）
- [x] 客观验证指标已定义
- [x] 错误处理格式已明确
- [x] 文件修改列表已完整

### 类型一致性

计划内统一使用以下命名：

- `SVoiceProcessingFactoryDefaults`（原 `SVoiceProcessingDefaults`）
- `SVoiceAgcConfig`
- `VoiceProcessingFactoryDefaults`（原 `VoiceProcessingDefaults`）
- `VoiceAgcConfigFromRuntime`
- `ComputeVoiceEncoderTargetsWithComplexity`（原 `ComputeVoiceEncoderTargetsV2`）
- `ComputeVoiceAutoGain`
- `ProcessVoiceCaptureFrame`
- `EVoiceProcessStage`
- `VoiceProcessTraceCallback`

## 验证记录

- run_cxx_tests：[x] 通过
  - 2026-04-30 实测：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target run_cxx_tests -j 10`
  - 结果：`692 tests` 全部通过
- game-client：[x] 通过
  - 2026-04-30 实测：`qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10`
  - 结果：`DDNet.exe` 链接成功
- 旧默认基线：[ ] 已冻结
  - 说明：本轮只完成代码默认值落地与自动化验证，未进入客户端手动 AB 录制阶段
- loopback / 双端验证：[ ] 失败原因
  - 原因：当前会话未执行人工客户端运行与录音回放验证，暂无可提交的主观听感与波形数据
- 弱网 / 抖动验证：[ ] 跳过
  - 原因：当前会话未接入 Clumsy / 手工断网 / 调试日志专项流程，只完成编码参数与构建测试验证
- CPU / 实时稳定性观察：[ ] 失败原因
  - 原因：当前会话未启动 `qm_voice_debug 1` 做持续运行观测，暂无 CPU / 实时抖动记录

### 测试数据记录

| 场景 | 旧配置 RMS | 新配置 RMS | 削顶帧数(旧) | 削顶帧数(新) | 备注 |
|------|-----------|-----------|-------------|-------------|------|
| 安静正常 | | | | | 未执行手动 AB |
| 安静小声 | | | | | 未执行手动 AB |
| 安静大声 | | | | | 未执行手动 AB |
| 键盘背景 | | | | | 未执行手动 AB |

## 近期不做项确认

- 真正双声道采集 / 编码：仍未实现。本轮仅保留现有接收/输出侧 `QmVoiceStereo` 与 `QmVoiceStereoWidth`，没有新增双声道采集或双声道编码链路。
- 默认开启的 AEC：仍未实现。本轮没有新增 AEC 配置项、默认开关或运行时处理逻辑，文档定位继续保持为后续专题。

## 本轮补充说明

- task 4 的默认值、设置页提示文案和 AGC 实验性开关已落地。
- 截至 2026-05-01 的当前分支增量，线上 `QmVoiceTokenHash` 仍保持“仅房间 hash”的 legacy 语义，以避免在 `VOICE_VERSION` 不变时破坏 mixed-version 兼容；接收侧会额外兼容带 `QmVoiceGroupMode` 高位的 token，并统一按低 30 位 group hash 判断同房间。
- 截至 2026-05-01 的当前分支增量，抖动缓冲起始序列已改为在入队时维护 `m_MinQueuedSeq`，`DecodeJitter()` 不再每次线性扫描队列找最小序列号；这属于当前实现已经发生的稳定性修正，后续计划执行应保留该方向。
- 截至 2026-05-01 的当前分支增量，overlay 和 UI 的“最近听到成员”统计已统一改为基于 `m_aLastHeard`，不再继续混用“房间存在”与“最近实际收听”两个概念。
- 当前分支还把混音临时缓冲从 `MixAudio()` 内部逐次临时分配，改成了 `CRClientVoice::m_MixBuffer` 复用；后续若继续扩展输出链路，不要退回热路径反复分配的旧写法。
- 本次会话的代码审查结论与文档更新同步推进；若后续继续在 `voice_core` / `voice_utils` 上叠加改动，仍需重点盯住协议 token 兼容语义、抖动缓冲序列推进，以及测试是否覆盖生产路径而不是只覆盖镜像 helper。
