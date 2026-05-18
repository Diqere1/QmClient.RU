# Focus Mode、Gores Mode 与栖梦页面数值控件修复规格

## 概述

本规格定义三类问题的目标行为与边界：

1. `Focus Mode` 的隐藏与恢复逻辑
2. `Gores Mode` 对 `tc_fast_input` / `tc_fast_input_others` 的自动联动
3. 栖梦页面内高优先级数值项的“滑块 + 输入框”统一交互

本次不直接扩展新功能，目标是让行为语义与当前确认的产品定义一致，并减少“配置值已恢复但画面仍未恢复”这类混合逻辑问题。

## 动机

当前实现存在三类一致性问题：

1. `Focus Mode` 同时依赖“改配置值”和“渲染层直接短路”两条链路，导致职责边界不清，关闭后容易出现局部元素未恢复。
2. `Gores Mode` 当前沿用“override 后恢复旧值”的语义，但产品语义已经明确为“开启 Gores 时自动开启，关闭 Gores 时自动关闭”。
3. 栖梦页面内同时存在旧的纯滑块和新的“滑块 + 输入框”两种数值编辑方式，提交手感与视觉风格不统一。

## 目标

### 1. Focus Mode

`Focus Mode` 的每个子开关都必须有清晰职责，不能出现“两个开关都可能影响同一元素，但逻辑分散在不同文件”的情况。

#### Hide HUD

只负责标准 HUD 与 DDRace HUD 主体元素，包括但不限于：

- `ClShowhud`
- `ClShowhudHealthAmmo`
- `ClShowhudScore`
- `ClShowLocalTimeAlways`
- `ClShowhudTimer`
- `ClShowhudTimeCpDiff`
- `ClShowhudDDRace`
- `ClShowhudJumpsIndicator`
- `ClShowhudSpectatorCount`
- `ClShowhudSpectator`
- `ClShowhudDummyActions`
- `ClShowhudKeyStatusReset`
- `ClShowhudKeyStatusHammer`
- `ClShowhudKeyStatusControl`
- `ClShowhudKeyStatusSync`
- `ClShowhudPlayerPosition`
- `ClShowhudPlayerSpeed`
- `ClShowhudPlayerAngle`
- `ClShowFreezeBars`
- `ClSpecCursor`
- `ClShowVotesAfterVoting`
- `ClShowIds`

目标效果：

- 开启时，上述 HUD 主体不显示。
- 关闭时，上述 HUD 主体恢复到用户关闭专注模式后的正常配置态。

#### Hide unnecessary UI

只负责非核心 HUD 的附加界面元素，包括但不限于：

- `TcStatusBar`
- `TcNotifyWhenLast` 对应的信息提示
- `QmDummyMiniView`
- `QmPlayerStatsMapProgress`
- `QmSmtcShowHud`
- 动态岛 / 媒体岛相关显示

目标效果：

- 开启时，上述附加 UI 隐藏。
- 关闭时，上述附加 UI 恢复到正常显示逻辑。

#### Hide player names

只负责头顶名字主体显示。

目标效果：

- 开启时，头顶名字隐藏。
- 关闭时，头顶名字恢复。

#### Hide overhead indicators

只负责头顶附加元素，包括但不限于：

- 方向键
- 强弱钩
- 与名字主体分离的其他头顶提示

目标效果：

- 开启时，这些元素隐藏。
- 关闭时，这些元素恢复。

#### Hide effects

只负责视觉特效，包括但不限于：

- 空气跳特效
- 锤击特效
- 粒子效果

#### Hide chat

只负责聊天消息显示与聊天气泡显示。

目标效果：

- 开启时，聊天框消息与 `QmChatBubble` 对应的聊天气泡隐藏。
- 关闭时，恢复正常显示。

#### Hide echo

只负责 echo 消息过滤。

目标效果：

- 普通 echo 可被隐藏。
- 极少数系统关键提示允许保留“强制可见”能力，例如专注模式开关提示。

#### Hide scoreboard

只负责计分板显示。

目标效果：

- 开启时，按 `Tab` 不显示计分板。
- 关闭时，计分板恢复正常逻辑。

### 2. Gores Mode

`QmGoresFastInput` 与 `QmGoresFastInputOthers` 的产品语义改为：

- `QmGoresFastInput`：开启 Gores Mode 时，是否自动开启 `tc_fast_input`
- `QmGoresFastInputOthers`：开启 Gores Mode 时，是否自动开启 `tc_fast_input_others`

目标效果：

- 开启 `Gores Mode` 时，如果对应自动项为开，则自动打开目标配置。
- 关闭 `Gores Mode` 时，自动关闭上述目标配置。
- 不再恢复用户旧值。

对应 UI 文案也必须同步改为“自动开关快速输入 / 自动开关快速输入其他玩家”一类的语义。

### 3. 栖梦页面数值控件

范围只包括 `src/game/client/components/qmclient/menus_qmclient.cpp` 中的栖梦页面，不包括旧 TClient 设置页。

目标行为：

- 高优先级数值项统一为“滑块 + 输入框”
- 纯数值输入框默认居中
- 自定义文本输入框不居中
- 回车可提交
- 点击外部可提交
- 输入非法值时自动钳制到合法范围

## 架构设计

### Focus Mode

保留 `ApplyFocusModeEffects()` 作为专注模式状态收敛入口，但需要重新整理两层职责：

1. 配置层覆写
2. 渲染层短路

要求：

- 同一元素不能由两个不同子开关交叉负责
- `Hide HUD` 与 `Hide unnecessary UI` 的元素集合必须显式区分
- 配置层与渲染层的职责要能互相解释，不允许一边恢复而另一边继续挡住

### Gores Mode

`ApplyGoresFastInputLink()` 改为状态推导型逻辑：

- 以 `GoresActive` 与两个自动开关配置推导最终值
- 不再把“用户旧值恢复”当作目标行为

### 栖梦页面数值控件

继续复用现有 `DoValueSelectorWithState(...)` 能力，但收敛成更统一的 helper，避免页面内散落多种编辑实现。

## 成功标准

1. 关闭 `Focus Mode` 后：
- HUD 恢复
- DDRace HUD 恢复
- 状态栏恢复
- 信息提示恢复
- 分身小窗恢复
- 地图进度条恢复
- SMTC HUD / 动态岛恢复
- 玩家名字与头顶元素恢复
- 聊天与聊天气泡恢复

2. 开启 `Gores Mode` 时：
- 自动快输开关按配置联动生效

3. 关闭 `Gores Mode` 时：
- `tc_fast_input`
- `tc_fast_input_others`
自动关闭

4. 栖梦页面中：
- 高优先级数值项使用统一的“滑块 + 输入框”
- 数值输入支持回车与外点提交
- 数值会自动钳制

## 文件范围

预期涉及的核心文件：

- `src/game/client/components/qmclient/qmclient.cpp`
- `src/game/client/components/qmclient/qmclient.h`
- `src/game/client/components/qmclient/config_override.h`
- `src/game/client/components/hud.cpp`
- `src/game/client/components/chat.cpp`
- `src/game/client/components/nameplates.cpp`
- `src/game/client/components/scoreboard.cpp`
- `src/game/client/components/infomessages.cpp`
- `src/game/client/components/qmclient/statusbar.cpp`
- `src/game/client/components/qmclient/menus_qmclient.cpp`
- `src/test/qmclient_state_override_test.cpp`

## 风险与非目标

### 风险

1. `Focus Mode` 当前是双轨逻辑，修一边漏一边会继续出恢复问题。
2. `Gores Mode` 语义切换后，旧的 override 测试需要同步重写。
3. 栖梦页数值项较多，本轮只适合先做高优先级项，不适合全量一口气替换。

### 非目标

1. 不在本轮处理非栖梦页面的大规模滑块改造。
2. 不在本轮做与该需求无关的配置前缀迁移。
3. 不新增新的 Focus/Gores 功能开关。
