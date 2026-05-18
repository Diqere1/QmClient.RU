# RmlUI RCSS 官方约束

**日期：** 2026-05-17

**来源：**
- RmlUi 官方文档：`https://mikke89.github.io/RmlUiDoc/pages/rcss`
- RmlUi 官方文档：`https://mikke89.github.io/RmlUiDoc/pages/rcss/animations_transitions_transforms`
- RmlUi 仓库说明：`https://github.com/mikke89/rmlui/blob/master/readme.md`

## 结论

RmlUI 的 RCSS 以 CSS2 为基础，并按游戏 UI 需求删除、调整或扩展了一部分能力。不能把浏览器 CSS 当作完整兼容目标。

本项目写 RmlUI 样式时默认按 CSS2 子集处理。需要使用动画、transition、transform、flex、gradient 等扩展能力时，必须先核对 RmlUi 官方文档或当前集成版本的解析结果。

## RCSS 与浏览器 CSS 的边界

- RCSS 是 RmlUI 自己的样式语言，不是浏览器 CSS 引擎。
- RCSS 文档明确要求和 CSS2 规范一起阅读，但它不是 CSS2 的完整复制。
- RmlUI 支持部分 CSS3 草案能力，例如 animation、transition、transform、flexbox、gradient 等，但语法和行为不等同于现代浏览器。
- 项目内 RCSS 文件必须以 RmlUI 实际 parser 能接受为准。

## transition 约束

RmlUI 官方文档中存在 `transition` 支持，但示例语法不是浏览器常见写法。

官方示例口径：

```rcss
#transition_test {
	transition: padding-left background-color transform 1.6s elastic-out;
	transform: scale(1.0);
	background-color: #c66;
}

#transition_test:hover {
	padding-left: 60px;
	transform: scale(1.5);
	background-color: #ddb700;
}
```

当前项目中以下浏览器式写法已经触发 RmlUI parser 报错：

```rcss
transition: background-color 0.18s ease-out, border-color 0.18s ease-out, color 0.18s ease-out, transform 0.12s ease-out;
```

错误形态：

```text
Syntax error parsing property declaration 'transition: ...'
```

因此在本项目里，未经过运行验证前禁止新增这种逗号分隔、每个属性独立 duration/easing 的浏览器式 transition。

## 本项目落地规则

- 默认不要在 `data/qmclient/rmlui/*.rcss` 中新增 `transition`。
- 按钮反馈优先使用静态 `:hover`、`:focus`、`:active`、`:disabled` 状态。
- 如果必须引入 transition，先在最小 RML/RCSS 片段中验证 RmlUI parser 不报错，再进入主样式文件。
- 不要用浏览器 CSS 经验推断 RCSS 支持度。
- 新增 CSS3 风格能力时，需要在同一变更里补充测试或运行日志证据。

## 当前已确认的安全写法

这些状态样式可以继续使用：

```rcss
.button:hover {
	background-color: #3a2b1f;
	border-color: #a57950;
}

.button:focus {
	background-color: #3a2b1f;
	border-color: #a57950;
}

.button:active {
	transform: translateY(1px);
}

.button:disabled {
	color: rgba(244, 247, 251, 92);
}
```

注意：`transform` 当前没有在运行日志中触发同类解析错误，但仍属于扩展能力。后续如果出现 parser 报错，应按日志行号收敛到具体属性，不要批量删除无关状态样式。

## 验证建议

修改 RCSS 后至少执行：

```powershell
rg -n "transition:" data/qmclient/rmlui
qmclient_scripts\cmake-windows.cmd --build build-ninja --target game-client -j 10
```

如果要确认构建目录资源同步，再检查：

```powershell
rg -n "transition:" build-ninja/data/qmclient/rmlui data/qmclient/rmlui
```

运行客户端时，日志中不应再出现对应 RCSS 文件的 `Syntax error parsing property declaration 'transition: ...'`。
