# RmlUi 与 QmClient 当前集成说明

**日期：** 2026-05-06

**状态：** 当前事实沉淀

## RmlUi 是什么

RmlUi 是一个面向游戏和原生应用的 C++ UI 库，设计思路接近 HTML / CSS。它提供文档树、布局、样式、事件和渲染接口，适合在已有渲染循环中嵌入 UI。

QmClient 当前 vendored 的版本为 `6.2`，来源见 `src/engine/external/rmlui/README.md`。

## 当前项目里的定位

QmClient 目前并没有把 RmlUi 扩展为通用 UI 框架，而是将它限制在一个明确、可回退的实验性场景中：

- 仅用于 `Ctrl+Shift+G` 对应的监控 HUD 渲染路径。
- 监控 HUD 应允许在菜单打开时继续显示，便于直接观察菜单与设置页的性能问题。
- 监控 HUD 本身由 `m_DbgGraphs` 控制显示，RmlUi 分支额外受 `qm_monitoring_use_rmlui`、`qm_rmlui_enable`、`qm_rmlui_monitoring_hud` 共同控制。
- 如果 RmlUi 初始化失败、文档加载失败、或图表区域解析失败，运行时会回退到原有 HUD 渲染路径。

对应配置项定义在 `src/engine/shared/config_variables_qmclient_extra.h`，设置页入口在 `src/game/client/components/qmclient/menus_qmclient.cpp`。

## 代码结构

当前集成可以分成 4 层：

### 1. 第三方源码与构建输入

- vendored 目录：`src/engine/external/rmlui/`
- 当前纳入构建的范围：
  - `Include/`
  - `Source/Core/`
  - `Source/Core/FontEngineDefault/`
  - `Backends/RmlUi_Renderer_GL3.*`
  - `Backends/RmlUi_Include_GL3.h`

构建层在 `CMakeLists.txt` 中通过 `RMLUI_CORE_SRC`、`RMLUI_FONT_ENGINE_DEFAULT_SRC`、`RMLUI_BACKEND_SRC` 聚合源码，并为 `game-client` 目标添加：

- `RMLUI_STATIC_LIB`
- `RMLUI_FONT_ENGINE_FREETYPE`
- `RMLUI_VERSION="6.2"`

同时把以下目录加入 include path：

- `src/engine/external/rmlui/Include`
- `src/engine/external/rmlui/Backends`

## 2. 引擎后端适配层

文件：

- `src/engine/client/rmlui_backend.h`
- `src/engine/client/rmlui_backend.cpp`

职责：

- 封装 `Rml::SystemInterface`
- 封装 `Rml::FileInterface`
- 封装 `Rml::RenderInterface`
- 对接 SDL/OpenGL3 后端初始化与每帧 begin/end

这里做了几件和项目环境强相关的事情：

- 要求存在活动的 OpenGL context，否则初始化失败。
- 通过 `RmlGL3::Initialize` 和 `RenderInterface_GL3` 接上 GL3 渲染器。
- 用 `IStorage` 读取 UI 资源，而不是直接依赖系统文件路径。
- 统一把 RmlUi 资源根目录映射到 `qmclient/rmlui/`。

也就是说，RmlUi 里写 `monitoring_hud.rml` 时，最终会优先按项目存储系统查找，并落到 `data/qmclient/rmlui/` 这一套资源上。

## 3. RmlUi Core 封装层

文件：

- `src/game/client/RmlUi/RmlUiCore.h`
- `src/game/client/RmlUi/RmlUiCore.cpp`

职责：

- 初始化和关闭 RmlUi runtime
- 设置 `SystemInterface` / `FileInterface`
- 调用 `Rml::Initialise`
- 创建名为 `qm_monitoring_hud` 的上下文
- 加载字体
- 提供 `LoadDocument`、`Update`、`Render`、`SetViewport`

当前字体加载策略为尝试以下资源：

- `fonts/SourceHanSans.ttc`
- `fonts/DejaVuSans.ttf`
- `fonts/Icons.ttf`

如果这些字体都没有成功加载，会记录告警日志，但不会在代码层直接中断初始化。

## 4. 业务 HUD 层

文件：

- `src/game/client/RmlUi/RmlUiMonitoringHud.h`
- `src/game/client/RmlUi/RmlUiMonitoringHud.cpp`
- `src/game/client/RmlUi/RmlUiRenderHelpers.h`
- `src/game/client/RmlUi/RmlUiRenderHelpers.cpp`

职责：

- 加载并持有 `monitoring_hud.rml`
- 把 `SQmMonitoringViewModel` 写入文档里的文本节点
- 用 RmlUi DOM 解析各图表容器的最终布局矩形
- 继续复用项目自己的图表绘制函数，在这些矩形内画网格与曲线

这意味着当前方案不是“所有监控 HUD 都交给 RmlUi 绘制”，而是：

- 文字、卡片、面板布局由 RmlUi 负责
- 曲线图和网格仍由 QmClient 原有图形接口负责

这是一个混合式方案，优点是能较快验证布局与样式收益，同时保留现有图表绘制逻辑。

## 资源结构

当前 RmlUi 业务资源位于：

- `data/qmclient/rmlui/monitoring_hud.rml`
- `data/qmclient/rmlui/monitoring_hud.rcss`

其中：

- `monitoring_hud.rml` 定义 DOM 结构、元素 id、面板和指标卡层级
- `monitoring_hud.rcss` 定义整体视觉样式，如 panel、grid、badge、metric card 等

`RmlUiMonitoringHud` 依赖这些稳定的元素 id 来做运行时更新和矩形解析，例如：

- `summary-title`
- `summary-detail`
- `summary-badge`
- `main-graph`
- `fps-graph`
- `card-latency`
- `secondary-frame-time`

如果后续修改 RML 结构，尤其是这些 id，需要同步检查 `UpdateDocument` 和 `ResolveRect` 的调用点。

## 运行链路

当前主要运行链路在 `src/game/client/gameclient.cpp`：

1. 用户开启 `qm_monitoring_use_rmlui`
2. 监控 HUD 进入渲染路径
3. `EnsureRmlUiMonitoringReady` 确保 core 和 HUD 已初始化
4. `CRmlUiCore::SetViewport` 同步当前屏幕尺寸
5. `CRmlUiCore::Update` 更新 RmlUi 文档
6. `CRmlUiMonitoringHud::Render`：
   - 确保文档已加载
   - 更新指标文本与尺寸属性
   - 调用 `CRmlUiCore::Render`
   - 解析 `main-graph` / `fps-graph` 的内容区域
   - 使用项目自身绘图函数叠加曲线与网格

日志中会区分两类结果：

- `render path=rmlui`
- `render path=fallback`

因此线上排查时可以直接通过日志判断当前实际使用的是哪条渲染路径。

## 当前约束与边界

这套集成目前有几个明确边界：

- 不迁移菜单、设置页或通用游戏 UI 本体，但允许监控 HUD 在菜单或设置页打开时继续叠加显示，用于观察这些页面的性能问题。
- 当前明确会抑制这条 HUD host 路径的是 console 激活，而不是菜单打开本身。
- 依赖活动的 OpenGL context，因此初始化时机必须处于可渲染阶段。
- 文档布局和图表绘制是混合实现，不是纯 RmlUi 方案。
- 资源路径和元素 id 目前是业务代码硬编码依赖，改资源结构时要同步改 C++。
- 出错策略偏保守，优先回退，不阻塞原有 HUD 使用。

## 维护建议

如果后续继续扩展这套能力，建议优先保持以下原则：

- 继续把 RmlUi 限定在独立、可开关、可回退的功能面中。
- 先复用现有 view model 与绘图能力，不急着把所有图形逻辑搬进 RmlUi。
- 修改 `monitoring_hud.rml` 或 `monitoring_hud.rcss` 时，同时检查对应元素 id 是否仍满足 `RmlUiMonitoringHud.cpp` 的假设。
- 升级 vendored RmlUi 版本时，先验证 GL3 backend、字体加载和文件接口行为是否保持兼容。

## 相关入口

- vendored 库说明：`src/engine/external/rmlui/README.md`
- 后端适配：`src/engine/client/rmlui_backend.cpp`
- runtime core：`src/game/client/RmlUi/RmlUiCore.cpp`
- 监控 HUD：`src/game/client/RmlUi/RmlUiMonitoringHud.cpp`
- 资源文件：`data/qmclient/rmlui/`
- 渲染接入：`src/game/client/gameclient.cpp`
- 用户开关：`src/game/client/components/qmclient/menus_qmclient.cpp`
