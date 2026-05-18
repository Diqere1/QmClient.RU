# 页面切换性能优化 实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 以方案 A 为主，分轮降低 `assets`、`general`、`tclient`、`qmclient`、`language` 页面切换和首进时的主线程阻塞。

**架构：** 第一阶段不改 UI 语义，围绕“缓存失效明确、可见区优先、冷路径剥离、毫秒预算”做页面级定向优化。P0 优先处理 `assets`、`general`、`tclient`，P1 再处理 `qmclient`、`language`，所有收益继续用现有 perf 日志闭环验证。

**技术栈：** C++、DDNet/QmClient UI、CMake、现有 perf timer/log 体系

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `src/game/client/components/menus.h` | 扩展页面级缓存、assets 预览队列状态、必要的辅助声明 |
| `src/game/client/components/menus_settings_assets.cpp` | 实现 assets 可见区优先调度、ready queue、毫秒预算、纹理上传收敛 |
| `src/game/client/components/menu_background.h` | 为主题元数据/图标缓存补充状态与失效入口 |
| `src/game/client/components/menu_background.cpp` | 把 theme 扫描与 icon 冷加载从 `RenderThemeSelection` 热路径中剥离 |
| `src/game/client/components/menus.cpp` | 使用已缓存的 theme 列表并补充 general 页 perf 验证点 |
| `src/game/client/components/qmclient/menus_qmclient.cpp` | 实现 tclient section 级可视区裁剪、热点控件降频；后续承接 qmclient tab 派生缓存 |
| `src/game/client/components/menus_settings.cpp` | 实现 language 页文本容器/列表缓存 |
| `docs/dyl/卡顿问题运行日志/run*.txt` | 手工验证 perf 收益的日志输入，不修改，仅用于回归比对 |

---

## 任务 1：P0 - assets 可见区优先预览调度

**文件：**
- 修改：`src/game/client/components/menus.h`
- 修改：`src/game/client/components/menus_settings_assets.cpp`
- 验证：`docs/dyl/卡顿问题运行日志/run4.txt`

- [ ] **步骤 1：扩展 assets 预览状态结构**

在 `src/game/client/components/menus.h` 的 `CMenus` 内新增仅服务于 assets 页面的一次性状态，避免继续在 render 中全量扫搜索结果：

```cpp
struct SAssetPreviewBudgetState
{
	int m_LastTab = -1;
	int m_LastVisibleFirst = -1;
	int m_LastVisibleLast = -1;
	int m_LastFilterHash = 0;
	std::deque<SCustomItem *> m_ReadyQueue;
	std::unordered_set<SCustomItem *> m_QueuedReadyItems;
};
```

同时声明辅助函数，约束实现边界：

```cpp
void ResetAssetPreviewBudgetState();
void InvalidateAssetPreviewBudgetStateForTab(int Tab);
void QueueCompletedAssetPreview(SCustomItem *pItem);
bool IsAssetPreviewRangeRelevant(int Index, int FirstVisible, int LastVisible) const;
```

- [ ] **步骤 2：在搜索列表重建、tab 切换、过滤条件变化时显式失效**

在 `src/game/client/components/menus_settings_assets.cpp` 中，当以下条件发生时调用 `ResetAssetPreviewBudgetState()`：

```cpp
if(gs_aInitCustomList[s_CurCustomTab])
{
	ResetAssetPreviewBudgetState();
}

if(s_CurCustomTab != m_AssetPreviewBudgetState.m_LastTab)
{
	InvalidateAssetPreviewBudgetStateForTab(s_CurCustomTab);
}
```

失效条件必须只包含：

- 当前 assets tab 发生变化
- 当前 tab 的搜索过滤条件变化
- 搜索列表被重建
- 页面主动刷新/删除资产导致指针集合变化

- [ ] **步骤 3：把“全集启动 decode”改成“可见区 + 邻近区启动 decode”**

用 `CListBox`/列表绘制过程里可得到的可见项索引替代当前：

```cpp
for(size_t i = 0; i < SearchListSize; ++i)
{
	StartPreviewDecode(i);
}
```

改为按可见区与邻近区调度：

```cpp
const int PrefetchRows = 2;
const int FirstRelevant = maximum(0, FirstVisibleIndex - PrefetchRows * ItemsPerRow);
const int LastRelevant = minimum((int)SearchListSize - 1, LastVisibleIndex + PrefetchRows * ItemsPerRow);
for(int i = FirstRelevant; i <= LastRelevant; ++i)
{
	StartPreviewDecode((size_t)i);
}
```

要求：

- 不为搜索结果全集启动解码
- 已有 `m_RenderTexture` 或 `m_pDecodeJob` 的项继续跳过
- workshop 远端缩略图链路保留现状，不与本轮主改动混在一起

- [ ] **步骤 4：把 completed job 扫描改成 ready queue + 毫秒预算**

保留后台解码线程，只改主线程收尾。先把已完成 job 收敛到 ready queue，再在预算内上传：

```cpp
constexpr double PreviewUploadBudgetMs = 4.0;
CPerfTimer UploadBudgetTimer;
while(!m_AssetPreviewBudgetState.m_ReadyQueue.empty())
{
	SCustomItem *pItem = m_AssetPreviewBudgetState.m_ReadyQueue.front();
	if(UploadBudgetTimer.ElapsedMs() >= PreviewUploadBudgetMs)
		break;

	m_AssetPreviewBudgetState.m_ReadyQueue.pop_front();
	m_AssetPreviewBudgetState.m_QueuedReadyItems.erase(pItem);
	UploadCompletedDecodeJob(pItem);
}
```

要求：

- 禁止继续对 `gs_vpSearch*List` 做每帧全量 completed scan
- 上传预算以毫秒为主，不再以 `MaxGpuUploadsPerFrame` 为唯一保护
- `UploadCompletedDecodeJob` 内部将 `LoadTextureRaw(...)` 改为 `LoadTextureRawMove(...)`

- [ ] **步骤 5：保留并补齐 perf 验证点**

保留现有埋点：

```cpp
assets_preview_gpu_upload_batch
assets_preview_gpu_upload_scan
assets_workshop_thumb_upload_batch
assets_preview_draw_workshop_cards
```

新增或调整为以下更可验证的点：

```cpp
assets_preview_decode_schedule
assets_preview_ready_enqueue
assets_preview_ready_budget
assets_preview_visible_window
```

预期验证：

- `assets_preview_gpu_upload_scan` 不再出现与 `settings_page_content page=assets` 对齐的 100ms~300ms 尖峰
- 单帧上传总耗时被限制在预算附近

- [ ] **步骤 6：构建并手工验证 P0-assets**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

预期：

- 构建成功
- `menus_settings_assets.cpp` 无新增编译错误

手工验证：

```powershell
cd build-ninja
.\DDNet.exe
```

控制台验证点：

- 开启 `qm_perf_debug 1`
- 进入 `assets`
- 复现首进和停留期滚动
- 将新日志保存到 `docs/dyl/卡顿问题运行日志/`

---

## 任务 2：P0 - general 主题列表冷路径剥离

**文件：**
- 修改：`src/game/client/components/menu_background.h`
- 修改：`src/game/client/components/menu_background.cpp`
- 修改：`src/game/client/components/menus.cpp`

- [ ] **步骤 1：为 theme 元数据和 icon 状态补缓存状态**

在 `src/game/client/components/menu_background.h` 中，将“主题列表是否已扫过”和“图标是否已完成加载”显式化：

```cpp
struct SThemeIconState
{
	bool m_LoadAttempted = false;
	bool m_Loaded = false;
};
```

并扩展 `CTheme` / `CMenuBackground` 相关状态，使 `GetThemes()` 不再在 render 中重复承担扫描和冷加载职责。

- [ ] **步骤 2：拆分“枚举主题”和“按需加载图标”**

在 `src/game/client/components/menu_background.cpp` 中把当前 `GetThemes()` 的冷路径拆成两个明确函数：

```cpp
void CMenuBackground::EnsureThemeMetadata();
void CMenuBackground::EnsureThemeIconLoaded(CTheme &Theme);
```

要求：

- `EnsureThemeMetadata()` 只负责目录枚举、排序、基础字段填充
- `EnsureThemeIconLoaded(CTheme &Theme)` 只在可见项或邻近项上调用
- `RefreshThemes()` 只清理缓存并重置状态，不在下一帧 render 中做隐式重工作

- [ ] **步骤 3：让 `RenderThemeSelection` 只消费缓存**

将 `src/game/client/components/menus.cpp` 中：

```cpp
const std::vector<CTheme> &vThemes = MenuBackground.GetThemes();
```

改成仅消费已准备好的元数据，并针对可见项/邻近项触发轻量级图标加载：

```cpp
MenuBackground.EnsureThemeMetadata();
std::vector<CTheme> &vThemes = MenuBackground.GetThemes();
for(int i = FirstVisible; i <= LastVisible; ++i)
{
	MenuBackground.EnsureThemeIconLoaded(vThemes[i]);
}
```

未就绪图标保持空纹理，占位即可，不阻塞列表首帧。

- [ ] **步骤 4：保留 general 页 perf 验证**

验证点至少保留：

```cpp
theme_selection_total
```

如需新增，限定为：

```cpp
theme_metadata_ensure
theme_icon_lazy_load
```

预期：

- 首进 `general` 时 `theme_selection_total` 从 430ms 级降到明显可接受范围
- 刷新按钮触发的重建不会再卡在单帧 render

- [ ] **步骤 5：构建并手工验证 P0-general**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

手工验证：

- 首次进入 `general`
- 点击刷新主题
- 观察图标占位是否能在短暂延迟后补齐
- 对比新日志里的 `theme_selection_total`

---

## 任务 3：P0 - tclient section 级可视区裁剪

**文件：**
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`

- [ ] **步骤 1：提取 section 边界与可视判断辅助逻辑**

在 `RenderSettingsTClientSettings` 同文件内增加轻量辅助结构：

```cpp
struct SSectionCullContext
{
	float m_ViewportTop;
	float m_ViewportBottom;
	float m_PrefetchPadding;
};

bool IsSectionVisible(const CUIRect &SectionRect, const SSectionCullContext &Context);
```

要求：

- 只做当前页面内的局部辅助函数，不引入跨模块抽象
- 保持现有布局代码风格

- [ ] **步骤 2：将左列/右列 section 改成“先算外框，再决定是否展开内部控件”**

把当前这种模式：

```cpp
s_SectionBoxes.push_back(Column);
Column.HSplitTop(HeadlineHeight, &Label, &Column);
Ui()->DoLabel(&Label, Localize("Visual"), HeadlineFontSize, TEXTALIGN_ML);
// 后面直接完整构建控件
```

改成：

```cpp
CUIRect SectionRect = Column;
SectionRect.HSplitTop(EstimatedHeight, &SectionRect, &Column);
if(IsSectionVisible(SectionRect, CullContext))
{
	RenderVisualSection(SectionRect, Column);
}
else
{
	Column = SectionRectAfterLayout;
}
```

第一轮不要求把所有 section 拆成新函数，但至少要让屏幕外 section 不再完整执行内部控件逻辑。

- [ ] **步骤 3：对 `DoLine_KeyReader` 做非交互帧降频**

在 `DoLine_KeyReader` 中缓存最近一次绑定字符串，避免每帧为未交互项反复扫描组合键：

```cpp
struct SKeyReaderCacheEntry
{
	char m_aCommand[128];
	char m_aLabel[128];
	bool m_Dirty = true;
};
```

触发重算的条件只包括：

- 命令字符串变化
- 对应绑定被修改
- 当前控件处于激活编辑态

- [ ] **步骤 4：保留并补齐 tclient perf 验证**

保留现有：

```cpp
tclient_settings_left_column
tclient_settings_right_column
tclient_settings_tile_outlines_section
tclient_tab_content
```

新增 section 级埋点仅限热点块，例如：

```cpp
tclient_settings_visual_section
tclient_settings_player_indicator_section
tclient_settings_hud_section
```

预期：

- 首进 `tclient` 默认设置页不再出现 700ms 级主线程阻塞
- 屏幕外 section 不再贡献主要 CPU 成本

- [ ] **步骤 5：构建并手工验证 P0-tclient**

运行：

```powershell
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10 | Select-Object -Last 30
```

手工验证：

- 首次进入 `tclient` 默认 tab
- 上下滚动到原先热点 section
- 对比 `tclient_settings_left_column`、`right_column`、`tile_outlines_section`

---

## 任务 4：P1 - qmclient tab 派生状态缓存

**文件：**
- 修改：`src/game/client/components/qmclient/menus_qmclient.cpp`

- [ ] **步骤 1：为热点 tab 建立派生状态缓存**

为 `functions`、`visuals` 等已确认热点 tab 增加缓存对象：

```cpp
struct SQmTabDerivedState
{
	bool m_Dirty = true;
	int m_Tab = -1;
	int m_FilterHash = 0;
	std::vector<int> m_vVisibleModuleIndices;
};
```

- [ ] **步骤 2：把搜索匹配、列分配、模块可见性判断移出首帧 render**

仅在以下条件下重算：

- tab 切换
- 搜索文本变化
- 模块开关影响显示集合

render 阶段只消费 `m_vVisibleModuleIndices`，不再重新跑全模块筛选。

- [ ] **步骤 3：优先覆盖已确认热点模块**

第一轮只要求验证这些模块：

```cpp
pie_menu
mini_features
block_words
camera_view
entity_overlay
```

- [ ] **步骤 4：构建并手工验证 P1-qmclient**

手工验证：

- 进入 `qmclient`
- 切换 `functions` / `visuals`
- 对比 `module=*` 级日志与 `tab_content` 首帧

---

## 任务 5：P1 - language 列表与 credits 文本容器缓存

**文件：**
- 修改：`src/game/client/components/menus_settings.cpp`

- [ ] **步骤 1：缓存 credits 文本容器**

把当前每帧 `Create -> Render -> Delete` 的 credits 文本容器改成带失效条件的持有状态：

```cpp
struct SLanguageCreditsCache
{
	STextContainerIndex m_TextContainer;
	float m_LastWidth = 0.0f;
	int m_LastLanguageVersion = 0;
	bool m_Valid = false;
};
```

失效条件只包括：

- 宽度变化
- 语言条目版本变化
- 页面销毁/资源回收

- [ ] **步骤 2：为语言列表增加轻缓存**

将语言名称、筛选结果、选中项索引的派生值缓存化，避免进入页面前几帧反复完整构建。

- [ ] **步骤 3：保留 perf 验证**

保留：

```cpp
language_list_total
language_page_total
```

必要时新增：

```cpp
language_credits_container_build
```

- [ ] **步骤 4：构建并手工验证 P1-language**

手工验证：

- 首次进入 `language`
- 切换语言并停留
- 对比 `language_list_total`、`language_page_total`

---

## 规格覆盖检查

- `assets`：已覆盖可见区优先、ready queue、毫秒预算、`LoadTextureRawMove`、保留 perf 闭环。
- `general`：已覆盖 theme 元数据预热、icon cache、可见区/邻近区懒加载。
- `tclient`：已覆盖 section 级可视区裁剪、重型控件降频、补足 section 级 perf 点。
- `qmclient`：已覆盖 tab 派生状态缓存与热点模块优先。
- `language`：已覆盖文本容器缓存与轻量列表缓存。
- 验证：所有任务均绑定 Windows 构建命令与手工 perf 日志验证。

## 占位符扫描

- 未使用 `TODO`、`待定`、`后续实现`。
- 每个任务都给出了修改文件、数据结构、失效条件和验证动作。

## 类型一致性

- `assets` 统一使用 `SAssetPreviewBudgetState`
- `general` 统一使用 `EnsureThemeMetadata` / `EnsureThemeIconLoaded`
- `qmclient` 统一使用 `SQmTabDerivedState`
- `language` 统一使用 `SLanguageCreditsCache`

---

计划已完成并保存到 `docs/superpowers/plans/2026-04-22-page-switch-performance-plan.md`。

执行方式默认采用 **子代理驱动（推荐）**，但当前会话已经确认要直接推进，因此下一步直接从 **任务 1：P0-assets** 开始实现。
