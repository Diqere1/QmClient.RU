#!/usr/bin/env python3
"""
Generate QmClient multi-language translation files under data/qmclient/languages/.

Strategy:
- Each language gets a file with the same base name as DDNet's data/languages/<lang>.txt
- simplified_chinese.txt is intentionally omitted for QmClient additive translations
- english.txt: Chinese keys → English translations (English keys are identity, omitted)
- Other languages: Chinese keys → English placeholder, English keys → identity (placeholder)
  They should later be translated by the community.

Format: Same as DDNet — key on one line, == translation on next line.
"""

import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
STRINGS_FILE = os.path.join(SCRIPT_DIR, "extracted_strings.txt")
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "data", "qmclient", "languages")

# ---------------------------------------------------------------------------
# Language metadata — MUST match DDNet's data/languages/index.txt
# We add "english" which DDNet hardcodes but doesn't have a file for.
# ---------------------------------------------------------------------------
LANGUAGES = [
    ("arabic", "العربية", "682", "ar"),
    ("azerbaijani", "Azərbaycan dili", "031", "az"),
    ("belarusian", "Беларуская", "112", "be"),
    ("bosnian", "Bosanski", "070", "bs-Latn"),
    ("brazilian_portuguese", "Português brasileiro", "076", "pt-BR"),
    ("bulgarian", "Български", "100", "bg"),
    ("catalan", "Català", "906", "ca"),
    ("chuvash", "Чăвашла", "643", "cv"),
    ("czech", "Česky", "203", "cs"),
    ("danish", "Dansk", "208", "da"),
    ("dutch", "Nederlands", "528", "nl"),
    ("english", "English", "826", "en"),
    ("esperanto", "Esperanto", "999", "eo"),
    ("estonian", "Eesti", "233", "et"),
    ("finnish", "Suomi", "246", "fi"),
    ("french", "Français", "250", "fr"),
    ("galician", "Galego", "906", "gl"),
    ("german", "Deutsch", "276", "de"),
    ("greek", "Ελληνικά", "300", "el"),
    ("hungarian", "Magyar", "348", "hu"),
    ("italian", "Italiano", "380", "it"),
    ("japanese", "日本語", "392", "ja"),
    ("korean", "한국어", "410", "ko"),
    ("kyrgyz", "Кыргызча", "417", "ky"),
    ("norwegian", "Norsk", "578", "no"),
    ("persian", "فارسی", "364", "fa"),
    ("polish", "Polski", "616", "pl"),
    ("portuguese", "Português", "620", "pt"),
    ("romanian", "Română", "642", "ro"),
    ("russian", "Русский", "643", "ru"),
    ("serbian", "Српски (Latinica)", "688", "sr-Latn"),
    ("serbian_cyrillic", "Српски (Ћирилица)", "688", "sr-Cyrl"),
    ("slovak", "Slovenčina", "703", "sk"),
    ("spanish", "Español", "724", "es"),
    ("swedish", "Svenska", "752", "sv"),
    ("traditional_chinese", "繁體中文", "158", "zh-Hant;zh-TW;zh-HK"),
    ("turkish", "Türkçe", "792", "tr"),
    ("ukrainian", "Українська", "804", "uk"),
]


def is_chinese(s):
    """Return True if s contains any CJK characters."""
    for ch in s:
        if "\u4e00" <= ch <= "\u9fff" or "\u3400" <= ch <= "\u4dbf":
            return True
    return False


# ---------------------------------------------------------------------------
# English translations for Chinese keys (manually curated)
# ---------------------------------------------------------------------------
EN_TRANSLATIONS = {
    # ── 通用 / General ──
    "关": "Off",
    "开": "On",
    "关闭": "Off",
    "默认": "Default",
    "自动": "Auto",
    "自定义": "Custom",
    "两者": "Both",
    "字面量": "Literal",
    "正则": "Regex",
    "正则表达式": "Regular expression",
    "重置": "Reset",
    "发送": "Send",
    "刷新": "Refresh",
    "设置颜色": "Set color",
    "透明度": "Opacity",
    "选项颜色": "Option color",
    "背景颜色": "Background color",
    "文字颜色": "Text color",
    "控制台颜色": "Console color",
    "辅助线颜色": "Guide line color",
    "线宽": "Line width",
    "动画": "Animation",
    "改名": "Rename",
    "翻译": "Translate",
    "语音": "Voice",
    "麦克风": "Microphone",
    "扬声器": "Speaker",
    # ── Tab & Module names ──
    "消息气泡": "Chat Bubble",
    "Gores演员专用": "Gores Actor",
    "Gores模式": "Gores Mode",
    "禅模式": "Zen Mode",
    "按键绑定": "Key Bindings",
    "梦的小功能": "Dream Features",
    "武器辅助线": "Weapon Trajectory",
    "镜头与视野": "Camera & FOV",
    "分身小窗": "Dummy Window",
    "显示坐标": "Show Coordinates",
    "主播模式": "Streamer Mode",
    "好友提醒": "Friend Notifications",
    "屏蔽词": "Word Filter",
    "聊天翻译按钮UI": "Chat Translate Button UI",
    "关键词回复": "Keyword Reply",
    "饼菜单": "Pie Menu",
    "实体层颜色": "Entity Layer Colors",
    "激光增强": "Laser Enhancement",
    "玩家统计": "Player Stats",
    "HJ辅助": "HJ Assist",
    "速通计时器": "Speedrun Timer",
    "灵动岛": "Dynamic Island",
    "SMTC": "SMTC",
    "3D背景": "3D Background",
    "功能搜索": "Feature Search",
    # ── Descriptions ──
    "在Tee上方显示对话气泡": "Show chat bubbles above Tees",
    "在玩家头顶显示聊天消息": "Show chat messages above players",
    "在水中死亡时自动说话": "Auto chat when dying in water",
    "Gores自动武器切换": "Gores auto weapon switch",
    "隐去UI专注游戏": "Hide UI for focused gameplay",
    "常见的按键绑定": "Common key bindings",
    "常用按键绑定": "Common key bindings",
    "只有你想不到,没有梦做不到": "Only what you can't imagine, nothing Dream can't do",
    "只有你想不到，没有梦做不到！": "Only what you can't imagine, nothing Dream can't do!",
    "显示枪榴弹和激光轨迹预览": "Show grenade and laser trajectory preview",
    "调整游戏镜头和视野设置": "Adjust game camera and FOV settings",
    "看看你操控本体的时候分身有没有被别人欺负": "Check if your dummy is being bullied while you control the main tee",
    "在你操控本体的时候分身有没有被欺负呢?": "Is your dummy being bullied while you control the main tee?",
    "显示自己和他人的坐标": "Show coordinates of yourself and others",
    "他妈的炸弹人都给我死啊!": "Damn grenadiers, just die!",
    "他妈的炸弹人全给我去Spa!": "Damn grenadiers, all go to spa!",
    "好友上线和加入提醒": "Friend online and join notifications",
    "聊天屏蔽词过滤": "Chat word filtering",
    "聊天翻译设置": "Chat translation settings",
    "聊天框按钮外观设置": "Chat box button appearance settings",
    "我 是 机 器 人 ": "I am a robot",
    "对玩家快捷操作菜单": "Quick action menu for players",
    "调整各个实体层的透明度": "Adjust opacity of entity layers",
    "你想做出怎样的激光?": "What kind of laser do you want?",
    "看看你有多演": "See how much you act",
    "显示碰撞和武器交互": "Show collision and weapon interaction",
    "你最爱的地图管家": "Your favorite map manager",
    "事已至此,多说无益": "What's done is done, no use saying more",
    "你想跑出怎样的Gores?": "What kind of Gores do you want to run?",
    "谁把OBS塞进来了": "Who stuffed OBS in here",
    "当然是最好的语音啦!": "The best voice chat, of course!",
    "Only Apple Can Do": "Only Apple Can Do",
    "基于Win的一坨所拉的一泡": "Based on a pile of Windows... stuff",
    "配置背景 3D 粒子效果": "Configure background 3D particle effects",
    "快速定位功能模块": "Quickly locate feature modules",
    "你想拍出怎样的大片": "What kind of blockbuster do you want to shoot?",
    "你想跑出怎样的人生?": "What kind of life do you want to run?",
    "画一张大饼": "Drawing a big pie in the sky",
    "噢噢噢噢噢噢噢噢噢噢噢噢噢噢噢噢": "Ohhhhhhhhhhhhhhhh",
    "Teeworlds的世界不会再出现挡人的实体层了": "No more blocking entity layers in the world of TeeWorlds",
    "在玩家头顶显示坐标": "Show coordinates above players",
    "显示玩家头顶的消息气泡": "Show chat bubbles above players",
    "水中自动发言": "Auto chat in water",
    "水中发送表情": "Send emoticon in water",
    "留空以禁用": "Leave empty to disable",
    "留空以进入公共房间": "Leave empty to join public room",
    "留空使用默认提示词": "Leave empty to use default prompt",
    "用逗号分隔": "Separate with commas",
    "示例: 名字1|名字2|名字3": "Example: name1|name2|name3",
    "请使用 %s 作为好友名称": "Please use %s as friend name",
    "用 ID 替换非好友名称": "Replace non-friend names with ID",
    "用默认皮肤替换非好友皮肤": "Replace non-friend skins with default",
    "在记分板上使用默认旗": "Use default flags on scoreboard",
    "显示自己的坐标": "Show own coordinates",
    "显示其他玩家的坐标": "Show other players' coordinates",
    "与我 X 对齐提示": "X alignment hint with me",
    "严格模式": "Strict mode",
    "判定时间": "Detection time",
    "显示 X": "Show X",
    "显示 Y": "Show Y",
    "X 对齐颜色": "X alignment color",
    "启用 Gores 模式": "Enable Gores mode",
    "Gores 模式下自动启用": "Auto enable in Gores mode",
    "Gores 模式按键": "Gores mode key",
    "启用禅模式": "Enable Zen mode",
    "禅模式按键": "Zen mode key",
    "启用复读": "Enable repeat",
    "启用关键词回复": "Enable keyword reply",
    "启用屏蔽词列表": "Enable word filter list",
    "启用思考模式（较慢）": "Enable thinking mode (slower)",
    "启用相机漂移": "Enable camera drift",
    "启用立体声定位": "Enable stereo positioning",
    "启用系统媒体控制": "Enable system media control",
    "启用语音": "Enable voice",
    "启用饼菜单": "Enable pie menu",
    "启用 3D 背景粒子": "Enable 3D background particles",
    "启用 FTAPI 自动翻译（可能导致服务过载）": "Enable FTAPI auto-translate (may overload the service)",
    "启用分身小窗": "Enable dummy window",
    "启用动态FOV": "Enable dynamic FOV",
    "自动登录 Axiom 服务器": "Auto login Axiom server",
    "自动刷新服务器列表": "Auto refresh server list",
    "自动问候进图的好友": "Auto greet friends entering map",
    "自动检测说话时开麦": "Auto unmute when speaking",
    "自动平衡麦克风音量（AGC，实验性）": "Auto gain control for mic (AGC, experimental)",
    "当好友上线时提醒": "Notify when friends come online",
    "在控制台中显示被屏蔽的词语": "Show blocked words in console",
    "在左上角显示歌曲信息": "Show song info in top-left corner",
    "大字播报进服好友": "Large text announcement for friend joining",
    "大字文案": "Large text content",
    "招呼文案": "Greeting text",
    "本地粒子效果": "Local particle effects",
    "远程粒子效果": "Remote particle effects",
    "计分板Qm标识": "Scoreboard Qm badge",
    "计分板查分": "Scoreboard point check",
    "显示版本过旧提示": "Show outdated version warning",
    "新版UI": "New UI",
    "锤中偷皮": "Hammer skin steal",
    "随机表情": "Random emoticon",
    "连击": "Combo",
    "隐藏输入表情": "Hide input emoticon",
    "彩虹名字": "Rainbow name",
    "位置跳跃提示": "Position jump hint",
    "显示模式": "Display mode",
    "始终显示": "Always show",
    "按键显示": "Show on key",
    "显示碰撞箱模式": "Show hitbox mode",
    "显示 Tee、地图和武器交互碰撞箱": "Show Tee, map, and weapon interaction hitboxes",
    "碰撞箱模式": "Hitbox mode",
    "武器交互范围": "Weapon interaction range",
    "武器范围颜色": "Weapon range color",
    "Tee 碰撞箱": "Tee hitbox",
    "Tee 碰撞箱颜色": "Tee hitbox color",
    "Freeze 边界颜色": "Freeze border color",
    "拾取物范围": "Pickup range",
    "拾取其他武器后禁用": "Disable after picking up other weapons",
    "隐藏辅助线": "Hide guide lines",
    "发送概率": "Send probability",
    "聊天消息": "Chat message",
    "表情 ID": "Emoticon ID",
    "关键词规则": "Keyword rules",
    "关键词规则的两侧都必须填写": "Both sides of keyword rules must be filled",
    "替换字符": "Replacement chars",
    "替换模式": "Replace mode",
    "根据词语长度使用多字符替换": "Use multi-char replacement based on word length",
    "最少匹配字符数": "Minimum match chars",
    "系统媒体控制": "System media control",
    "动态FOV平滑度": "Dynamic FOV smoothness",
    "动态FOV强度": "Dynamic FOV intensity",
    "漂移平滑度": "Drift smoothness",
    "漂移强度": "Drift intensity",
    "漂移方向": "Drift direction",
    "当前宽高比:": "Current aspect ratio:",
    "当前宽高比: 显示默认": "Current aspect ratio: Show default",
    "自定义宽高比": "Custom aspect ratio",
    "宽高比预设": "Aspect ratio preset",
    "分身小窗大小": "Dummy window size",
    "分身小窗缩放": "Dummy window zoom",
    "仅本地": "Local only",
    "本地 + 分身": "Local + Dummy",
    "所有玩家": "All players",
    "显示队伍": "Show team",
    "玩家范围": "Player range",
    "检测距离": "Detection distance",
    "地图危险边界": "Map danger border",
    "解冻不透明度": "Unfreeze opacity",
    "冻结不透明度": "Freeze opacity",
    "深度冻结不透明度": "Deep freeze opacity",
    "深度解冻不透明度": "Deep unfreeze opacity",
    "死亡不透明度": "Death opacity",
    "CP点不透明度": "CP opacity",
    "传送不透明度": "Teleport opacity",
    "开关不透明度": "Switch opacity",
    "叠层不透明度": "Tune layer opacity",
    "进度条颜色": "Progress bar color",
    "进度条宽度": "Progress bar width",
    "进度条高度": "Progress bar height",
    "饼菜单透明度": "Pie menu opacity",
    "翻译按钮": "Translate button",
    "翻译服务": "Translation service",
    "LLM 提供商": "LLM provider",
    "目标语言": "Target language",
    "目标语言比例": "Target language ratio",
    "并发数（0 = 自动）": "Concurrency (0 = auto)",
    "发送源语言": "Send source language",
    "发送目标语言": "Send target language",
    "发送翻译方式": "Send translation method",
    "始终翻译": "Always translate",
    "仅在需要时翻译": "Translate only when needed",
    "自动翻译发送的消息": "Auto translate sent messages",
    "自动翻译收到的消息": "Auto translate received messages",
    "翻译按钮和菜单的颜色": "Translate button and menu colors",
    "自定义翻译按钮和菜单的颜色": "Customize translate button and menu colors",
    "高级选项": "Advanced options",
    "语音激活释放延迟": "Voice activation release delay",
    "语音码率": "Voice bitrate",
    "语音距离半径（格）": "Voice distance radius (tiles)",
    "说话触发阈值": "Speech trigger threshold",
    "麦克风音量": "Microphone volume",
    "静音麦克风": "Mute microphone",
    "播放音量": "Playback volume",
    "输入设备": "Input device",
    "输出设备": "Output device",
    "输入切换": "Input switch",
    "输出切换": "Output switch",
    "降噪模式": "Noise reduction mode",
    "不降噪": "No noise reduction",
    "简单降噪": "Simple noise reduction",
    "简单降噪强度": "Simple noise reduction strength",
    "RNNoise 降噪": "RNNoise noise reduction",
    "RNNoise 降噪强度": "RNNoise noise reduction strength",
    "RNNoise 降噪（当前构建不可用）": "RNNoise noise reduction (unavailable in this build)",
    "当前构建未集成 RNNoise，将自动回退到简单降噪": "RNNoise not integrated in current build, will fallback to simple noise reduction",
    "回退后的简单降噪强度": "Fallback simple noise reduction strength",
    "系统默认": "System default",
    "立体声定位": "Stereo positioning",
    "左右声道宽度": "Left/right channel width",
    "房间": "Room",
    "房间密码": "Room password",
    "同房间全图收听": "Full map listen in same room",
    "服务器IP": "Server IP",
    "等待打开": "Waiting to open",
    "已打开": "Opened",
    "已连接": "Connected",
    "已断开": "Disconnected",
    "已静音": "Muted",
    "已切换": "Switched to",
    "切换失败": "Switch failed",
    "未启用": "Not enabled",
    "语音连接状态": "Voice connection status",
    "音频问题": "Audio issue",
    "音频异常未分类": "Unclassified audio issue",
    "音频后端初始化失败": "Audio backend init failed",
    "音频后端初始化失败，建议切换设备后重试，并查看详细原因": "Audio backend init failed, try switching devices and check details",
    "音频初始化失败，建议查看下方详细原因并结合日志排查": "Audio init failed, check details below and logs",
    "系统没有可用输入设备": "No input device available",
    "系统没有可用输出设备": "No output device available",
    "输入设备不存在": "Input device not found",
    "输出设备不存在": "Output device not found",
    "输入设备打开失败": "Input device open failed",
    "输出设备打开失败": "Output device open failed",
    "输入设备打开失败，建议重插耳机/麦克风后刷新，或重新选择输入设备": "Input device open failed, try reconnecting headset/mic or reselecting input device",
    "输出设备打开失败，建议重插耳机/扬声器后刷新，或重新选择输出设备": "Output device open failed, try reconnecting speakers/headphones or reselecting output device",
    "默认输入打开失败": "Default input open failed",
    "默认输出打开失败": "Default output open failed",
    "正在切回默认输入": "Switching back to default input",
    "正在切回默认输出": "Switching back to default output",
    "正在切换": "Switching",
    "等待打开输入设备": "Waiting to open input device",
    "等待打开输出设备": "Waiting to open output device",
    "未打开，请检查输入设备或麦克风权限": "Not open, check input device or mic permission",
    "未打开，请检查输出设备": "Not open, check output device",
    "麦克风权限被系统拒绝": "Microphone permission denied by system",
    "需要在系统设置里允许麦克风权限，然后重新打开语音": "Allow mic permission in system settings, then reopen voice",
    "建议先检查输入设备、系统默认麦克风和麦克风权限": "Check input device, system default mic, and mic permission first",
    "建议先检查输出设备，确认耳机或扬声器仍在线": "Check output device, confirm headphones/speakers are still online",
    "建议重新选择输入设备，并确认默认麦克风或耳机麦克风仍在线": "Try reselecting input device, confirm default mic or headset mic is online",
    "建议重新选择输出设备，并确认耳机或扬声器仍在线": "Try reselecting output device, confirm headphones/speakers are online",
    "建议重新切换语音开关或重试进入服务器": "Try toggling voice or reconnecting to server",
    "建议检查语音服务器地址是否可用": "Check if voice server address is reachable",
    "建议确认双方在同服、同房间，并且对方也支持语音": "Confirm both are on same server, same room, and support voice",
    "未进入服务器": "Not connected to server",
    "需要先进入服务器，语音网络链路才会建立": "Connect to server first to establish voice network link",
    "未发现音频异常": "No audio issues detected",
    "请先启用语音": "Please enable voice first",
    "本地测试模式": "Local test mode",
    "本地测试模式，无需服务器": "Local test mode, no server needed",
    "本地正在发送，建议让对方开麦或确认对方是否能接收": "Sending locally, suggest the other party unmute or confirm they can receive",
    "正在发送，等待对端回声": "Sending, waiting for peer echo",
    "正在发送和接收": "Sending and receiving",
    "正在接收": "Receiving",
    "已匹配到可通话对端": "Matched with callable peer",
    "当前未发现可通话对端": "No callable peer found",
    "暂无对端": "No peer",
    "未知状态": "Unknown status",
    "当前状态正常，如仍异常请查看下方详细原因": "Status normal, check details below if still experiencing issues",
    "详细原因": "Detailed reason",
    "建议排查": "Troubleshooting suggestions",
    "正在解析语音服务器地址": "Parsing voice server address",
    "已连接，等待首个 ping": "Connected, waiting for first ping",
    "已联通，暂时无人说话": "Connected, no one is speaking",
    "更新功能列表...": "Updating feature list...",
    "未找到匹配的功能模块。请尝试其他关键词": "No matching features found. Try other keywords",
    "匹配到 %d 个模块": "Found %d modules",
    "手动并发数：%d": "Manual concurrency: %d",
    "自动并发数：%d（智能默认）": "Auto concurrency: %d (smart default)",
    "使用默认输入": "Use default input",
    "使用默认输出": "Use default output",
    "使用原始样式": "Use original style",
    "使用分身回复": "Reply with dummy",
    "显示语音连接状态": "Show voice connection status",
    "先看这里，可以快速判断卡在设备、服务器还是房间": "Start here to quickly diagnose if the issue is with device, server, or room",
    "UDP 套接字未打开": "UDP socket not open",
    "仅当另一个Tee不在屏幕上时显示": "Only show when the other Tee is not on screen",
    "异常断开": "Abnormal disconnect",
    "傻逼词过滤器": "Idiot word filter",
    "上一首": "Previous",
    "下一首": "Next",
    "播放/暂停": "Play/Pause",
    "显示歌曲信息": "Show song info",
    "系统媒体控制 (SMTC)": "System Media Transport Controls (SMTC)",
    "改名队列": "Rename queue",
    "自定义提示词模板": "Custom prompt template",
    "自定义 API Key": "Custom API Key",
    "API Key": "API Key",
    "API key": "API key",
    "SecretId": "SecretId",
    "SecretKey": "SecretKey",
    "端点": "Endpoint",
    "端点 (可选)": "Endpoint (optional)",
    "模型": "Model",
    "区域": "Region",
    "Append language codes like [ru], [en], [ja] at the end when sending": "Append language codes like [ru], [en], [ja] at the end when sending",
    "Enable thinking mode requires using reasoning models": "Enable thinking mode requires using reasoning models",
    "Ensure backend supports OpenAI-compatible thinking parameter": "Ensure backend supports OpenAI-compatible thinking parameter",
    "空": "Empty",
    "已选中": "Selected",
    "所有": "All",
    "取消": "Cancel",
    "确定": "Confirm",
    "保存": "Save",
    "删除": "Delete",
    "添加": "Add",
    "编辑": "Edit",
    "搜索": "Search",
    "加载中": "Loading",
    "暂无数据": "No data",
    "复制": "Copy",
    "复制成功": "Copied",
    "复制失败": "Copy failed",
    "清空": "Clear",
    "启用": "Enable",
    "禁用": "Disable",
    "成功": "Success",
    "失败": "Failed",
    "警告": "Warning",
    "错误": "Error",
    "信息": "Info",
    "确认": "Confirm",
    "是": "Yes",
    "否": "No",
    "提示": "Hint",
    "详情": "Details",
    "更多": "More",
    "收起": "Collapse",
    "展开": "Expand",
    "上一页": "Previous page",
    "下一页": "Next page",
    "首页": "First page",
    "尾页": "Last page",
    "共 %d 条": "Total %d items",
    "打开颜色选择器": "Open color picker",
    "点击以设置颜色": "Click to set color",
    "大小": "Size",
    "最大尺寸": "Max size",
    "最小尺寸": "Min size",
    "背景不透明度": "Background opacity",
    "UI缩放": "UI scale",
    "气泡不透明度": "Bubble opacity",
    "粒子类型": "Particle type",
    "粒子颜色": "Particle color",
    "粒子数量": "Particle count",
    "粒子透明度": "Particle alpha",
    "粒子发光": "Particle glow",
    "粒子脉动": "Particle pulse",
    "粒子拖尾": "Particle trail",
    "粒子碰撞": "Particle collision",
    "粒子速度": "Particle speed",
    "粒子深度": "Particle depth",
    "粒子闪烁": "Particle twinkle",
    "脉动幅度": "Pulse amplitude",
    "脉动速度": "Pulse speed",
    "脉动强度": "Pulse strength",
    "闪烁强度": "Twinkle strength",
    "拖尾长度": "Trail length",
    "拖尾透明度": "Trail alpha",
    "发光强度": "Glow intensity",
    "发光透明度": "Glow alpha",
    "发光偏移": "Glow offset",
    "推动半径": "Push radius",
    "推动强度": "Push strength",
    "输入叠加层": "Input overlay",
    "输入叠加层显示": "Input overlay display",
    "输入叠加层不透明度": "Input overlay opacity",
    "显示输入": "Show inputs",
    "水平位置": "Horizontal position",
    "垂直位置": "Vertical position",
    "视野边距": "View margin",
    "动态FOV": "Dynamic FOV",
    "相机漂移": "Camera drift",
    "激光预览": "Laser preview",
    "激光设置": "Laser settings",
    "激光大小": "Laser size",
    "激光样式": "Laser style",
    "圆角端点": "Rounded caps",
    "自动禁用时间": "Auto disable when time expires",
    "暂停后自动关闭当前聊天": "Automatically close the current chat after waking from freeze",
    "显示冰冻后的唤醒弹窗": "Show wake-up popup on the other tee",
    "自动换到刚解冻的Tee": "Auto switch to the tee that got unfrozen",
    "自动解除旁观": "Auto unspec on unfreeze",
    "自动锁定队伍": "Auto team lock",
    "自动热重载外部保存": "Auto hot-reload after external saves",
    "不解冻时自动锁定": "Lock delay",
    "地图进度条（实验性）": "Map progress bar (experimental)",
    "使用嵌入式HUD进度条": "Use embedded HUD progress bar",
    "显示虚线地图路线调试": "Show dotted map route debug",
    "启用速通计时器": "Enable speedrun timer",
    "速通倒计时": "Countdown timer for speedruns",
    "重置统计数据": "Reset stats when joining a server",
    "显示玩家统计HUD": "Show player stats HUD",
    "收藏地图管理": "Favorite map manager",
    "收藏的地图": "Favorite maps",
    "暂无收藏地图": "No favorite maps yet",
    "从收藏移除": "Remove from favorites",
    "点击复制地图名称": "Click to copy the map name",
    "复制地图名称": "Copy map name",
    "已复制": "Copied",
    "地图收藏": "Map favorites",
    "玩家统计与信息": "Player stats and info display",
    "换肤": "Swap skin",
    "观战": "Spectate",
    "分身": "Dummy",
    "好友": "Friend",
    "私聊": "Whisper",
    "提到": "Mention",
    "事件": "Event",
    "乐趣": "Fun",
    "Solo": "Solo",
    "Race": "Race",
    "Dummy": "Dummy",
    "Unknown": "Unknown",
    # ── Additional translations ──
    "45°瞄准": "45° Aim",
    "Axiom 主号密码": "Axiom main account password",
    "Axiom 分身密码": "Axiom dummy password",
    "Gores 模式": "Gores Mode",
    "King of Gores 自动切换枪": "King of Gores auto weapon switch",
    "TeeWorlds的世界不会再出现挡人的实体层了": "No more blocking entity layers in the world of TeeWorlds",
    "⚠️ FTAPI 是一个免费服务。过度使用可能导致服务暂停。": "⚠️ FTAPI is a free service. Excessive use may cause service suspension.",
    "仅在另一个Tee不在屏幕上时显示": "Only show when the other Tee is not on screen",
    "右跳": "Right jump",
    "左跳": "Left jump",
    "已切换到": "Switched to",
    "帧率": "Frame rate",
    "弹跳": "Bounce",
    "当前状态": "Current status",
    "感谢您的陪伴与信任.正是如此,才给了我继续前行的勇气": "Thank you for your company and trust. This is what gives me the courage to keep going.",
    "收发": "Send & Receive",
    "收缩": "Shrink",
    "智谱 AI": "Zhipu AI",
    "智谱 API Key": "Zhipu API Key",
    "服务器": "Server",
    "消散": "Dissolve",
    "游戏时间边距": "Game time margin",
    "瞄缝救人": "Gap aim rescue",
}

# Additional Chinese-to-English translations for strings that appear in the extracted list
# but are in the format of "X 不透明度" etc. We'll handle these programmatically with common patterns.
_PATTERN_TRANSLATIONS = {
    "不透明度": "Opacity",
    "颜色": "Color",
    "大小": "Size",
    "模式": "Mode",
    "按键": "Key",
    "设置": "Settings",
    "选项": "Options",
    "配置": "Configuration",
    "显示": "Show",
    "隐藏": "Hide",
    "启用": "Enable",
    "禁用": "Disable",
    "切换": "Toggle",
    "发送": "Send",
    "接收": "Receive",
}


def translate_to_english(chinese_str):
    """Translate a Chinese string to English using the manual dictionary.
    Falls back to the original string if no translation is found."""
    # Check exact match first
    if chinese_str in EN_TRANSLATIONS:
        return EN_TRANSLATIONS[chinese_str]
    # Return original if no translation available
    return chinese_str


def read_strings():
    """Read extracted strings from file."""
    with open(STRINGS_FILE, "r", encoding="utf-8") as f:
        strings = [line.rstrip("\n") for line in f if line.strip()]
    return strings


def write_language_file(filename, entries):
    """Write a language file in the DDNet format: key then == translation."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    filepath = os.path.join(OUTPUT_DIR, filename)
    with open(filepath, "w", encoding="utf-8", newline="\n") as f:
        for key, translation in entries:
            f.write(f"{key}\n")
            f.write(f"== {translation}\n\n")
    print(f"  Wrote {len(entries)} entries to {filepath}")


def generate_english(strings):
    """
    For english.txt: Chinese keys → English translations.
    English keys are already correct (Localize falls back to key).
    """
    entries = []
    missing_translations = []
    for s in strings:
        if is_chinese(s):
            translated = translate_to_english(s)
            if translated != s:
                entries.append((s, translated))
            else:
                missing_translations.append(s)
    if missing_translations:
        print("  WARNING: missing English translations for Chinese keys:")
        for key in missing_translations:
            print(f"    - {key}")
    return entries


def generate_other_language(strings, lang_code, lang_name):
    """
    For other languages: use English translations as placeholder.
    Community translators can replace these later.
    """
    entries = []
    for s in strings:
        if is_chinese(s):
            translated = translate_to_english(s)
            entries.append((s, translated))
        else:
            entries.append((s, s))  # English stays English as placeholder
    return entries


def create_readme():
    """Create a README explaining the translation structure."""
    readme_path = os.path.join(OUTPUT_DIR, "README.txt")
    with open(readme_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("""QmClient Translation Files
=========================

These files contain translations for QmClient-specific UI strings.
They are loaded additively on top of DDNet's base translations.

File format:
  key
  == translation

- Keys that are in Chinese are the original strings from QmClient UI code.
- Keys that are in English are technical terms or configuration labels.

How to contribute translations:
  1. Open the file for your language (e.g., german.txt).
  2. Find entries where the translation matches the English fallback.
  3. Replace the English text with your language's translation.
  4. Keep the "== " prefix before each translation.
  5. Do NOT modify the key line (the line without "== ").

Example:
  Before:
    消息气泡
    == Chat Bubble

  After (German):
    消息气泡
    == Chat-Blase

Current status:
  - English environment: `english.txt` maps Chinese keys back to English
  - Simplified Chinese environment: directly uses source Chinese keys, no extra overlay file
  - All other languages: English placeholder translations

Auto-generated by qmclient_scripts/languages_qmclient/generate_all.py
""")
    print(f"  Created {readme_path}")


def create_index():
    """Create data/qmclient/languages/index.txt (same format as DDNet's)."""
    index_path = os.path.join(OUTPUT_DIR, "index.txt")
    with open(index_path, "w", encoding="utf-8", newline="\n") as f:
        for filename, native_name, country_code, lang_tags in LANGUAGES:
            f.write(f"{filename}\n")
            f.write(f"== {native_name}\n")
            f.write(f"== {country_code}\n")
            f.write(f"== {lang_tags}\n\n")
    print(f"  Created {index_path}")


def main():
    print("=" * 60)
    print("QmClient Translation File Generator")
    print("=" * 60)

    # Read extracted strings
    strings = read_strings()
    print(f"\nLoaded {len(strings)} unique strings")

    # Create output directory
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Create index file
    print("\n--- Creating index.txt ---")
    create_index()

    # Create README
    print("\n--- Creating README ---")
    create_readme()

    # Generate language files
    for filename, native_name, country_code, lang_tags in LANGUAGES:
        print(f"\n--- {filename}.txt ({native_name}) ---")
        if filename == "english":
            entries = generate_english(strings)
        else:
            entries = generate_other_language(strings, filename, native_name)

        if entries:
            write_language_file(f"{filename}.txt", entries)
        else:
            print(f"  No entries for {filename} (all keys are source language)")

    print(f"\n{'=' * 60}")
    print("Done! All language files generated in:")
    print(f"  {OUTPUT_DIR}")
    print(f"{'=' * 60}")

    # Print summary
    chinese_keys = sum(1 for s in strings if is_chinese(s))
    english_keys = len(strings) - chinese_keys
    print("\nSummary:")
    print(f"  Chinese-origin keys: {chinese_keys}")
    print(f"  English-origin keys: {english_keys}")
    print(f"  Total unique strings: {len(strings)}")
    print(f"  Language files created: {len(LANGUAGES)}")
    print(f"  English translations provided: {len(EN_TRANSLATIONS)}")


if __name__ == "__main__":
    main()
