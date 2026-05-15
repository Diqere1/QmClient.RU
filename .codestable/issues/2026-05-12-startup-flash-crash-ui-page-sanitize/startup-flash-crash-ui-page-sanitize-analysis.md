---
doc_type: issue-analysis
issue: 2026-05-12-startup-flash-crash-ui-page-sanitize
status: confirmed
root_cause_type: state-pollution
related: [startup-flash-crash-ui-page-sanitize-report.md]
tags: [startup, crash, skins7, menus, serverbrowser, state-corruption, chat, uninitialized-state]
---

# Startup Flash Crash Ui Page Sanitize 根因分析

## 1. 问题定位

| 关键位置 | 说明 |
|---|---|
| `src/game/client/components/skins7.cpp:349` | `CSkins7::LoadSkin` 把 `pName` 拷进 `CSkin::m_aName[24]` 时，第三个参数传的是“文件名去掉 `.json` 后的长度”，不是目标缓冲区大小。这个调用本身是内存不安全写点。 |
| `src/game/client/components/skins7.cpp:441` | `build-debug` 最新 crash 已符号化到这里，实际崩在 `if(Config()->m_Debug)`。这说明 `LoadSkin` 走到函数尾时，`this -> m_pClient -> m_pConfig` 链已经被污染。 |
| `src/game/client/component.cpp:101-111` | `CComponentInterfaces::ServerBrowser()` 先信任 `m_pClient->ServerBrowser()`，只有它为 `nullptr` 时才回退到 `Kernel()->RequestInterface<IServerBrowser>()`。它不校验 cached interface 是否已被污染。 |
| `src/game/client/components/menus.cpp:1587-1604` | `CMenus::OnInit` 的 `ui_page_sanitize` 在 favorite-community 分支里直接对 cached `IServerBrowser *` 调 `FavoriteCommunities()`；一旦 cached pointer 非空但已坏，就会在启动期直接崩。 |
| `src/game/client/gameclient.cpp:1225-1228` | `CGameClient::OnInit` 早期日志点明确记录了 `cached_serverbrowser == kernel_serverbrowser`。说明 cached interface 在 `gameclient` 进入组件初始化前还是好的，损坏发生在后续 startup 流程里。 |

## 2. 失败路径还原

**正常路径**：客户端启动后，`CGameClient::OnInit` 先完成接口缓存，然后组件依次 `OnInit()`。`CMenus::OnInit` 里的 `ui_page_sanitize` 应该只读取有效的 server browser 状态；`CSkins7::OnInit` 应该扫描 `skins7/*.json`，构造 `CSkin` 后安全插入 `m_vSkins`，最后继续剩余组件初始化。

**失败路径 A（build-debug，已符号化）**：

1. `CGameClient::OnInit` 进入组件初始化，`CMenus::OnInit` 已经正常跑过，且日志显示 `cached=kernel`、`favorite_communities size=2`。
2. 流程继续到 `CSkins7::OnInit -> Refresh -> Storage()->ListDirectory(..., SkinScan, ...) -> CSkins7::LoadSkin(...)`。
3. `LoadSkin` 在构造局部 `CSkin Skin` 时执行 `str_copy(Skin.m_aName, pName, 1 + str_length(pName) - str_length(".json"))` 这个不安全写点。
4. 到函数尾部执行 `if(Config()->m_Debug)` 时，`Config()` 返回空，RIP 落在 `skins7.cpp:441`。这表明 `LoadSkin` 所在调用链的对象/接口状态已经在函数内部或其紧邻路径上被污染。

**失败路径 B（build-ninja，最新 release crash）**：

1. `CGameClient::OnInit` 早期日志显示 `cached_serverbrowser` 与 `kernel_serverbrowser` 一致。
2. 组件初始化进行到 `CMenus::OnInit` 时，`ui_page_sanitize` 仍走 favorite-community 分支。
3. 同一时刻日志显示 `cached=0000025B5DBF6EA3`，而 `kernel=0000025B5F440AF0`，两者已经分叉，且 cached pointer 明显异常。
4. `menus.cpp:1601` 左右继续对这个非空但已坏的 cached pointer 调 `FavoriteCommunities()`，进而在 release 启动期崩溃。

**分叉点 1**：`src/game/client/components/skins7.cpp:349`  
这是当前已确认 crash 路径上最直接的内存不安全写点，会把“皮肤扫描”从普通资源加载变成 startup state corruption 源头。

**分叉点 2**：`src/game/client/component.cpp:101-111` + `src/game/client/components/menus.cpp:1587-1604`  
cached `IServerBrowser *` 一旦被污染，启动 sanitize 逻辑并不会降级到 kernel interface 或稳定 reset，而是直接解引用坏指针。

## 3. 根因

**根因类型**：`state-pollution`

**根因描述**：当前 startup flash crash 不是单一的“RmlUI 页面渲染失败”，而是启动期状态先被写坏、随后在不同模块上以不同方式炸出来。第一层已确认问题是 `CSkins7::LoadSkin` 的名字拷贝越界风险，会污染 startup state；第二层已确认问题是 `CMenus::OnInit` 的 favorite-community sanitize 会继续信任坏掉的 cached `IServerBrowser *`。在这两层止血后，`build-ninja` 仍然会在 `CChat` 初始化边界出现 release-only 闪退，最终通过显式初始化 `CChat` / `CChat::CLine` 的启动期标量状态稳定消失，说明这里还叠加了未初始化状态触发的 UB。

**是否有多个根因**：是。

- **主根因 A**：`CSkins7::LoadSkin` 启动扫描路径存在内存不安全写点，能够污染 startup state。
- **次级放大因素 B**：`ServerBrowser()` 的 startup 读取只防 `nullptr`，不防“非空但已坏”的 cached interface，因此被污染后的状态会在 `menus` 启动 sanitize 阶段被放大成硬崩。
- **次级根因 C**：`CChat` / `CChat::CLine` 存在多处未显式初始化的启动期标量成员，release 构建下会在 `CChat::OnInit` 早期触发未定义行为；该问题对代码布局敏感，表现为“加日志后崩溃消失”，符合未初始化状态 UB 的典型特征。

**补充说明**：

- 这次本地可见的 `skins7` 文件名集合没有超过 `m_aName[24]` 的 repo 自带样本，因此 fix 阶段还需要补一个最小复现或增加诊断，确认当前触发文件来源；但从代码安全性和 crash 位置看，`skins7.cpp:349` 已经是必须先收口的 blocker。
- `build-debug` 与 `build-ninja` 崩在不同位置，不代表是两个互不相干的问题；更符合现有证据的解释是“同一类 startup state corruption，在不同优化级/时序下先后炸在不同读取点”。

## 4. 影响面

- **影响范围**：不仅影响当前 `g_Config.m_UiPage=PAGE_FAVORITE_COMMUNITY_1` 的启动场景，也会影响任何启动时需要扫描 `skins7`、随后读取 cached client interfaces 的路径。
- **潜在受害模块**：`CMenus`、`CSkins7`、所有通过 `CComponentInterfaces` 读取 cached interface 的 startup 组件，以及后续仍然依赖这些 cached 指针的 RmlUI host / settings 初始化链路。
- **数据完整性风险**：有。当前更像运行期内存/状态污染，而不是单纯逻辑分支错误；它会让后续 crash 落点漂移，导致“有时黑屏、有时闪退、有时卡在菜单”的症状混在一起。
- **严重程度复核**：维持 `P0`。当前产物无法稳定完成启动，直接阻断后续设置页、RmlUI host、release-state 收尾的全部验证。

## 5. 修复方案

### 方案 A：先收口 `skins7` 写坏点，再补 startup guard

- **做什么**：
  - 修 `src/game/client/components/skins7.cpp:349`，把 `str_copy` 第三个参数改成真实目标缓冲区大小，并补一个覆盖长文件名的测试。
  - 同时把 `menus.cpp:1587-1604` 的 startup sanitize 改成“只用 kernel serverbrowser 或校验 cached==kernel，否则直接 reset page”。
- **优点**：同时处理“状态被写坏”和“坏状态被硬解引用”两层问题，最符合当前证据链。
- **缺点 / 风险**：会同时动 `skins7` 和 `menus/component` 两块核心启动逻辑，验证面较广。
- **影响面**：`src/game/client/components/skins7.cpp`、`src/game/client/components/menus.cpp`、`src/game/client/component.cpp`，以及对应测试。

### 方案 B：先把 startup sanitize 收到 kernel interface，暂时绕开 cached pointer 崩点

- **做什么**：
  - 保守处理 `menus.cpp`：在 `ui_page_sanitize` 一律用 `Kernel()->RequestInterface<IServerBrowser>()`，若 unavailable 或与 cached 不一致就直接 reset 到 `PAGE_INTERNET`。
  - 先不动 `skins7`，只加诊断日志继续追污染源。
- **优点**：改动小，能最快止住 release 构建里“刚启动就死在 favorite_communities”的问题。
- **缺点 / 风险**：这只是止血。`build-debug` 已确认 `CSkins7::LoadSkin` 路径也会崩，说明底层污染源还在。
- **影响面**：主要是 `menus.cpp` / `component.cpp` 和少量测试。

### 方案 C：暂时禁用 startup `skins7` 扫描，先恢复可启动性

- **做什么**：
  - 在 `CSkins7::OnInit` / `Refresh` 启动扫描前加临时开关或降级路径，只保留 placeholder/default 皮肤，跳过 JSON 扫描。
  - 等启动恢复后再单独回头修 `skins7`。
- **优点**：如果目标是先恢复“客户端能开起来”，这是最直接的隔离法。
- **缺点 / 风险**：属于功能降级，不是根修；而且会暂时破坏 0.7 皮肤功能，偏离当前主线。
- **影响面**：`skins7.cpp` 和 0.7 皮肤相关体验。

### 推荐方案

**推荐方案 A（并补充 chat 初始化显式化）**，理由：当前证据已经不支持“只是菜单页逻辑错了，回 fallback 就行”。真正的问题是启动期存在多层状态污染/未定义行为：只修 `menus` 不能解释 `build-debug` 已符号化到 `CSkins7::LoadSkin` 的 crash；只修 `skins7` 又会留下 cached interface 在启动 sanitize 上的硬崩点；只修这两处仍无法解释 release-only 的 `CChat` 边界闪退。最终可验证闭环是同时收口 `skins7`、`menus`，并把 `CChat` 启动状态显式初始化。
