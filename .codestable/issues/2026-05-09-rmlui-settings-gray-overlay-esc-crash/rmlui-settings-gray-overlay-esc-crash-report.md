---
doc_type: issue-report
issue: 2026-05-09-rmlui-settings-gray-overlay-esc-crash
status: confirmed
severity: P1
summary: 进入 RmlUI 设置页后顶部和右侧出现灰层，按 Esc 会直接闪退
tags: [rmlui, settings, menu-pilot, crash]
---

# RmlUI 设置页灰层与 Esc 闪退 Issue Report

## 1. 问题现象

进入设置页面后，页面上方和右侧会出现大面积灰层遮罩。设置内容区本身还能显示，但界面整体已经处于异常状态；此时按 `Esc` 会直接闪退。

## 2. 复现步骤

1. 启动客户端，并进入当前启用了 RmlUI 设置页试点的菜单路径。
2. 进入设置页面。
3. 观察到页面上方和右侧出现灰层遮罩，设置内容区仍可显示。
4. 按 `Esc`。
5. 观察到客户端直接闪退。

复现频率：100%

## 3. 期望 vs 实际

**期望行为**：进入设置页时，如果当前 RmlUI 还没有实现该页面的完整 UI，就应该直接回退为旧版设置页，而不是显示异常壳层。

**实际行为**：进入设置页后，页面上方和右侧出现灰层遮罩，设置内容区本身还能显示；此时按 `Esc` 会直接闪退。

## 4. 环境信息

- 涉及模块 / 功能：RmlUI settings page / menu pilot 试点路径
- 相关文件 / 函数：待定
- 运行环境：本地 Windows 开发构建
- 其他上下文：用户附带截图 `C:/Users/11054/AppData/Local/quickclipboard/clipboard_images/3df3f4d3345abf73.png`；用户另有怀疑这是 roadmap 尚未走到可用状态，但当前报告未将其作为根因结论

## 5. 严重程度

**P1** — 设置页入口 100% 复现灰层异常，且按 `Esc` 会直接闪退，属于核心 UI 路径严重受损，但尚非整客户端完全不可启动。

## 备注

- 当前已知最小现象是“进入设置页即出现灰层”，不需要额外手动切换页签才能触发。
- 报告阶段只记录现象，不对根因做结论。
