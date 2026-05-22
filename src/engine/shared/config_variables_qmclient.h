// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(ConfigName, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(ConfigName, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(ConfigName, ScriptName, Len, Def, Save, Desc) ;
#endif

// Log / 日志
MACRO_CONFIG_INT(QmPerfDebug, qm_perf_debug, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用主线程与渲染阶段性能调试日志")
MACRO_CONFIG_INT(QmPerfDebugThresholdMs, qm_perf_debug_threshold_ms, 20, 1, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "性能调试日志阈值（毫秒）")
MACRO_CONFIG_INT(QmUiRuntimeV2Debug, qm_ui_runtime_v2_debug, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用 UI 运行时 v2 调试日志")

// Report / 举报
MACRO_CONFIG_STR(QmReportEndpoint, qm_report_endpoint, 128, "http://124.222.146.111:8790", CFGFLAG_CLIENT | CFGFLAG_SAVE, "举报服务地址")
MACRO_CONFIG_STR(QmReportAppId, qm_report_app_id, 128, "desktop", CFGFLAG_CLIENT | CFGFLAG_SAVE, "举报服务 App ID")
MACRO_CONFIG_STR(QmReportSecret, qm_report_secret, 128, "SsF-7wLdC9dO-RCb5sGieLII9gVW0v5lPpiK6zitUNo", CFGFLAG_CLIENT | CFGFLAG_SAVE, "举报服务签名密钥")

// Scoreboard / 计分板
MACRO_CONFIG_INT(ClScoreboardPointsLegacy, cl_scoreboard_points, 0, 0, 1, CFGFLAG_CLIENT, "旧版记分牌设置")
MACRO_CONFIG_INT(ClScoreboardSortModeLegacy, cl_scoreboard_sort_mode, 0, 0, 1, CFGFLAG_CLIENT, "旧版记分牌设置")
MACRO_CONFIG_INT(QmScoreboardPoints, qm_scoreboard_points, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "在记分牌中显示分数列（从 DDNet API 获取）")
MACRO_CONFIG_INT(QmScoreboardSortMode, qm_scoreboard_sort_mode, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "记分牌排序模式（0=分数，1=分）")
MACRO_CONFIG_INT(QmScoreboardOnDeath, qm_scoreboard_on_death, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "死亡后显示记分牌")
MACRO_CONFIG_INT(QmScoreboardAnimOptim, qm_scoreboard_anim_optim, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "计分板动画优化")
MACRO_CONFIG_INT(QmChatFadeOutAnim, qm_chat_fade_out_anim, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "聊天框淡出动画")
MACRO_CONFIG_INT(QmEmoticonSelectAnim, qm_emoticon_select_anim, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "表情选择动画")

// Chat / 聊天
MACRO_CONFIG_STR(ClMessageSystemGradient, cl_message_system_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "系统消息文字渐变颜色")
MACRO_CONFIG_STR(ClMessageClientGradient, cl_message_client_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "客户端消息文字渐变颜色")
MACRO_CONFIG_STR(ClMessageHighlightGradient, cl_message_highlight_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "突出显示消息文字渐变颜色")
MACRO_CONFIG_STR(ClMessageTeamGradient, cl_message_team_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "队伍消息文字渐变颜色")
MACRO_CONFIG_STR(ClMessageGradient, cl_message_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "普通消息文字渐变颜色")
MACRO_CONFIG_STR(ClMessageFriendGradient, cl_message_friend_gradient, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友消息文字渐变颜色")

// Rainbow Name / 彩虹名字
MACRO_CONFIG_INT(QmRainbowName, qm_rainbow_name, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用自己名字的彩虹色渲染")
MACRO_CONFIG_INT(QmNameplateCoordX, qm_nameplate_coord_x, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌显示坐标X")
MACRO_CONFIG_INT(QmNameplateCoordXAlignHint, qm_nameplate_coord_x_align_hint, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌显示与本地玩家X对齐的提示")
MACRO_CONFIG_INT(QmNameplateCoordXAlignHintStrict, qm_nameplate_coord_x_align_hint_strict, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌X对齐提示严格模式")
MACRO_CONFIG_INT(QmNameplateCoordXAlignHintWindowMs, qm_nameplate_coord_x_align_hint_window_ms, 1000, 100, 3000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌X对齐提示持续判定窗口（毫秒）")
MACRO_CONFIG_COL(QmNameplateCoordXAlignHintColor, qm_nameplate_coord_x_align_hint_color, 0x21FF99, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌X对齐提示高亮颜色")
MACRO_CONFIG_INT(QmNameplateCoordY, qm_nameplate_coord_y, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌显示坐标Y")
MACRO_CONFIG_INT(QmNameplateCoordsOwn, qm_nameplate_coords_own, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌显示自己坐标")
MACRO_CONFIG_INT(QmNameplateCoords, qm_nameplate_coords, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名字牌显示他人坐标")

// Enhanced Laser Effects (Glow + Pulse) / 增强激光效果（辉光+脉冲）
MACRO_CONFIG_INT(QmLaserEnhanced, qm_laser_enhanced, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用增强激光特效（辉光+脉冲动画）")
MACRO_CONFIG_INT(QmLaserGlowIntensity, qm_laser_glow_intensity, 30, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "激光辉光强度 (0-100)")
MACRO_CONFIG_INT(QmLaserPulseSpeed, qm_laser_pulse_speed, 100, 10, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "脉冲动画速度 ( 百分比, 100=正常)")
MACRO_CONFIG_INT(QmLaserPulseAmplitude, qm_laser_pulse_amplitude, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "脉冲振幅 (0-100)")
MACRO_CONFIG_INT(QmLaserSize, qm_laser_size, 100, 50, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "激光大小/粗细 (百分比, 100=默认)")
MACRO_CONFIG_INT(QmLaserRoundCaps, qm_laser_round_caps, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "激光圆角端点 (0=方角, 1=圆角)")
MACRO_CONFIG_INT(QmLaserAlpha, qm_laser_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "激光半透明度 (0=完全透明, 100=不透明)")

// Collision Hitbox Visualization / 碰撞体积可视化
MACRO_CONFIG_INT(QmShowCollisionHitbox, qm_show_collision_hitbox, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示碰撞体积边框 (Show collision hitbox outlines)")
MACRO_CONFIG_COL(QmCollisionHitboxColorFreeze, qm_collision_hitbox_color_freeze, 16711935, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Freeze碰撞边框颜色 (Freeze collision box color)")
MACRO_CONFIG_INT(QmCollisionHitboxAlpha, qm_collision_hitbox_alpha, 80, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "碰撞体积线条透明度 (Collision hitbox line alpha)")

// Entity Overlay / 实体叠加
MACRO_CONFIG_INT(QmEntityOverlayDeathAlpha, qm_entity_overlay_death_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "覆盖死亡图块的实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayFreezeAlpha, qm_entity_overlay_freeze_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "覆盖冻结图块的实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayUnfreezeAlpha, qm_entity_overlay_unfreeze_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "覆盖实体 alpha 以解冻图块 (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayDeepFreezeAlpha, qm_entity_overlay_deep_freeze_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "深度冻结图块的叠加实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayDeepUnfreezeAlpha, qm_entity_overlay_deep_unfreeze_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "深度解冻图块的叠加实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayTeleAlpha, qm_entity_overlay_tele_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Teletile 的覆盖实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlayTeleCheckpointAlpha, qm_entity_overlay_tele_checkpoint_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "CP点边界的覆盖实体 alpha (0-100)")
MACRO_CONFIG_INT(QmEntityOverlaySwitchAlpha, qm_entity_overlay_switch_alpha, 100, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "开关图块的覆盖实体 alpha (0-100)")

// Q1menG Client Recognition / Q1menG客户端识别
MACRO_CONFIG_INT(QmClientMarkTrail, qm_client_mark_trail, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "远程粒子：通过中心服同步并渲染其他玩家（需对方开启本地+远程）")
MACRO_CONFIG_INT(QmClientShowBadge, qm_client_show_badge, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示Qm标识：通过中心服识别并在名字板/计分板标记QmClient用户")

// Legacy QiaFen Compatibility / 旧恰分兼容配置
MACRO_CONFIG_INT(QmQiaFenEnabled, qm_qiafen_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "旧恰分开关（仅兼容迁移）")
MACRO_CONFIG_INT(QmQiaFenUseDummy, qm_qiafen_use_dummy, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "旧恰分 Dummy 开关（仅兼容迁移）")
MACRO_CONFIG_STR(QmQiaFenRules, qm_qiafen_rules, 2048, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "旧恰分规则（仅兼容迁移）")
MACRO_CONFIG_STR(QmQiaFenKeywords, qm_qiafen_keywords, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "旧恰分关键词（仅兼容迁移）")
MACRO_CONFIG_INT(QmKeywordReplyEnabled, qm_keyword_reply_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用关键词回复")
MACRO_CONFIG_INT(QmKeywordReplyUseDummy, qm_keyword_reply_use_dummy, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "关键词回复使用Dummy")
MACRO_CONFIG_INT(QmKeywordReplyAutoRename, qm_keyword_reply_auto_rename, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "旧全局关键词改名开关（仅兼容迁移）")
MACRO_CONFIG_STR(QmKeywordReplyRules, qm_keyword_reply_rules, 4096, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "关键词回复规则（每行: [rename] [regex] 关键词=>回复）")
MACRO_CONFIG_INT(QmAutoReplyCooldown, qm_auto_reply_cooldown, 3, 0, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动回复冷却时间（秒）")

// Pie Menu / 饼菜单
MACRO_CONFIG_INT(QmPieMenuEnabled, qm_pie_menu_enabled, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用饼菜单 (Enable pie menu for player interactions)")
MACRO_CONFIG_INT(QmPieMenuMaxDistance, qm_pie_menu_max_distance, 400, 100, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "玩家检测最大距离 (Maximum detection distance for nearest player)")
MACRO_CONFIG_INT(QmPieMenuScale, qm_pie_menu_scale, 100, 50, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "UI大小百分比 (UI scale percentage)")
MACRO_CONFIG_INT(QmPieMenuOpacity, qm_pie_menu_opacity, 80, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "菜单不透明度 (Menu opacity 0-100)")
MACRO_CONFIG_STR(QmPieMenuRenameQueue, qm_pie_menu_rename_queue, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "饼菜单改名名单（使用|分隔，如：璇梦1|璇梦2）")
MACRO_CONFIG_INT(QmPieMenuColorFriend, qm_pie_menu_color_friend, 0xE64D66BF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "好友选项颜色")
MACRO_CONFIG_INT(QmPieMenuColorWhisper, qm_pie_menu_color_whisper, 0x8059B3BF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "私聊选项颜色")
MACRO_CONFIG_INT(QmPieMenuColorMention, qm_pie_menu_color_mention, 0xD98033BF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "提及选项颜色")
MACRO_CONFIG_INT(QmPieMenuColorCopySkin, qm_pie_menu_color_copy_skin, 0x408CCCBF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "复制皮肤选项颜色")
MACRO_CONFIG_INT(QmPieMenuColorSwap, qm_pie_menu_color_swap, 0xCC4D4DBF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "交换选项颜色")
MACRO_CONFIG_INT(QmPieMenuColorSpectate, qm_pie_menu_color_spectate, 0x738C99BF, 0, 0, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "观战选项颜色")

// Repeat Message / 复读功能
MACRO_CONFIG_INT(QmRepeatEnabled, qm_repeat_enabled, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用复读功能 (Enable repeat last message)")
MACRO_CONFIG_INT(QmRepeatKey, qm_repeat_key, 278, 0, 512, CFGFLAG_CLIENT | CFGFLAG_SAVE, "复读快捷键 (Repeat key, default: Home=278)")
MACRO_CONFIG_INT(QmSayNoPop, qm_say_nopop, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输入时不显示打字表情 (Hide typing emoticon while chatting)")
MACRO_CONFIG_INT(QmHammerSwapSkin, qm_hammer_swap_skin, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "锤人换皮肤 (Copy target skin on hammer hit)")
MACRO_CONFIG_INT(QmRandomEmoteOnHit, qm_random_emote_on_hit, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "被锤/榴弹击中时随机表情 (Random emote on hammer/grenade hit)")
MACRO_CONFIG_INT(QmWeaponTrajectory, qm_weapon_trajectory, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用武器弹道辅助线 (Enable weapon trajectory)")
MACRO_CONFIG_INT(QmDeepflyMode, qm_deepfly_mode, 0, 0, 3, CFGFLAG_CLIENT, "Deepfly模式（0=正常，1=DF，2=HDF，3=自定义）")

// Auto Unspec on Unfreeze / 解冻自动取消旁观
MACRO_CONFIG_INT(QmAutoUnspecOnUnfreeze, qm_auto_unspec_on_unfreeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "被解冻时自动取消旁观 (Auto exit spectator mode when unfrozen)")

// Auto-Switch on Unfreeze / HJ大佬辅助 - 自动切换到解冻的Tee
MACRO_CONFIG_INT(QmAutoSwitchOnUnfreeze, qm_auto_switch_on_unfreeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "本体和dummy都freeze时，自动切换到先解冻的那个 (Auto switch to unfrozen tee when both are frozen)")
// Auto Close Chat on Unfreeze / HJ大佬辅助 - 解冻后自动关闭聊天
MACRO_CONFIG_INT(QmAutoCloseChatOnUnfreeze, qm_auto_close_chat_on_unfreeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "解冻后自动关闭聊天 (Auto close chat input when unfrozen)")

// Input Overlay / 输入叠加
MACRO_CONFIG_INT(QmInputOverlay, qm_input_overlay, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示输入覆盖")
MACRO_CONFIG_INT(QmInputOverlayScale, qm_input_overlay_scale, 20, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输入叠加比例（百分比）")
MACRO_CONFIG_INT(QmInputOverlayOpacity, qm_input_overlay_opacity, 80, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输入叠加不透明度（百分比）")
MACRO_CONFIG_INT(QmInputOverlayPosX, qm_input_overlay_pos_x, 71, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输入叠加 X 位置（百分比）")
MACRO_CONFIG_INT(QmInputOverlayPosY, qm_input_overlay_pos_y, 80, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输入叠加 Y 位置（百分比）")

// Notification Bar / 通知栏
MACRO_CONFIG_INT(QmHudNotificationsSystem, qm_hud_notifications_system, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏接管服务器系统提示（入场版本信息除外）")
MACRO_CONFIG_INT(QmHudNotificationsEcho, qm_hud_notifications_echo, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏接管 echo 消息")
MACRO_CONFIG_INT(QmHudNotificationsCompatSolo, qm_hud_notifications_compat_solo, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "兼容其他服务器的类似提示接管（例如自定义单人区域提示，按黑名单排除）")
MACRO_CONFIG_COL(QmHudNotificationsBgColor, qm_hud_notifications_bg_color, 0x99000000, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "通知栏背景颜色")
MACRO_CONFIG_COL(QmHudNotificationsTextColor, qm_hud_notifications_text_color, 0xFFFFFFFF, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "通知栏系统提示文字颜色")
MACRO_CONFIG_INT(QmHudNotificationsEchoInheritColor, qm_hud_notifications_echo_inherit_color, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏 echo 继承聊天 echo 颜色")
MACRO_CONFIG_COL(QmHudNotificationsEchoTextColor, qm_hud_notifications_echo_text_color, 0xFF92FFFF, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "通知栏 echo 覆盖文字颜色")
MACRO_CONFIG_INT(QmHudNotificationsTextSize, qm_hud_notifications_text_size, 8, 1, 24, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏文字大小")
MACRO_CONFIG_INT(QmHudNotificationsHoldMs, qm_hud_notifications_hold_ms, 2500, 500, 10000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏停留时间（毫秒）")
MACRO_CONFIG_INT(QmHudNotificationsAnimType, qm_hud_notifications_anim_type, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏动画类型（0=淡入滑入 1=仅淡入 2=无动画）")
MACRO_CONFIG_INT(QmHudNotificationsAnimMs, qm_hud_notifications_anim_ms, 220, 0, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏动画时间（毫秒）")
MACRO_CONFIG_INT(QmHudNotificationsMaxVisible, qm_hud_notifications_max_visible, 3, 1, 8, CFGFLAG_CLIENT | CFGFLAG_SAVE, "通知栏最大显示数量")
MACRO_CONFIG_INT(QmMonitoringHudOpacity, qm_monitoring_hud_opacity, 66, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "调试图表面板不透明度（百分比）")

// Voice / 语音
MACRO_CONFIG_INT(QmVoiceEnable, qm_voice_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用语音聊天")
MACRO_CONFIG_INT(QmVoiceAgcEnable, qm_voice_agc_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "自动增益控制（0=关闭 1=开启）")
MACRO_CONFIG_INT(QmVoiceProtocolVersion, qm_voice_protocol_version, 3, 1, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音协议版本")
MACRO_CONFIG_STR(QmVoiceServer, qm_voice_server, 128, "42.194.185.210:9987", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音服务器地址 host:port")
MACRO_CONFIG_STR(QmVoiceAudioBackend, qm_voice_audio_backend, 64, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音音频后端（SDL 驱动名，空为自动）")
MACRO_CONFIG_STR(QmVoiceInputDevice, qm_voice_input_device, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音输入设备（空为默认）")
MACRO_CONFIG_STR(QmVoiceOutputDevice, qm_voice_output_device, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音输出设备（空为默认）")
MACRO_CONFIG_INT(QmVoiceStereo, qm_voice_stereo, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音立体声输出")
MACRO_CONFIG_INT(QmVoiceStereoWidth, qm_voice_stereo_width, 100, 0, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音立体声宽度（百分比）")
MACRO_CONFIG_STR(QmVoiceToken, qm_voice_token, 128, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音房间 Token（可选）")
MACRO_CONFIG_INT(QmVoiceGroupMode, qm_voice_group_mode, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音分组模式")
MACRO_CONFIG_INT(QmVoiceFilterEnable, qm_voice_filter_enable, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用语音滤波（高通/压缩/限幅）")
MACRO_CONFIG_INT(QmVoiceBitrateProfile, qm_voice_bitrate_profile, 0, 0, 4, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音码率档位（0=自动 1=24kbps 2=32kbps 3=48kbps 4=64kbps）")
MACRO_CONFIG_INT(QmVoiceNoiseSuppressEnable, qm_voice_noise_suppress_enable, 2, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "噪声抑制模式（0=关闭 1=简单 2=RNNoise）")
MACRO_CONFIG_INT(QmVoiceNoiseSuppressStrength, qm_voice_noise_suppress_strength, 35, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "噪声抑制强度（百分比）")
MACRO_CONFIG_INT(QmVoiceCompThreshold, qm_voice_comp_threshold, 24, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器阈值（百分比）")
MACRO_CONFIG_INT(QmVoiceCompRatio, qm_voice_comp_ratio, 20, 10, 80, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器比率（x10）")
MACRO_CONFIG_INT(QmVoiceCompAttackMs, qm_voice_comp_attack_ms, 12, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器攻击时间（毫秒）")
MACRO_CONFIG_INT(QmVoiceCompReleaseMs, qm_voice_comp_release_ms, 140, 10, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器释放时间（毫秒）")
MACRO_CONFIG_INT(QmVoiceCompMakeup, qm_voice_comp_makeup, 125, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "压缩器补偿增益（百分比）")
MACRO_CONFIG_INT(QmVoiceLimiter, qm_voice_limiter, 92, 10, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "限幅器阈值（百分比）")
MACRO_CONFIG_INT(QmVoiceRadius, qm_voice_radius, 50, 1, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音距离半径（Tiles）")
MACRO_CONFIG_INT(QmVoiceVolume, qm_voice_volume, 100, 0, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音播放音量（百分比）")
MACRO_CONFIG_INT(QmVoiceMicVolume, qm_voice_mic_volume, 100, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "麦克风音量（百分比）")
MACRO_CONFIG_INT(QmVoiceMicMute, qm_voice_mic_mute, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "静音麦克风")
MACRO_CONFIG_INT(QmVoiceShowConnectionStatus, qm_voice_show_connection_status, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示语音连接状态")
MACRO_CONFIG_INT(QmVoiceTestMode, qm_voice_test_mode, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音测试模式（0=关 1=本地 2=服务器回环）")
MACRO_CONFIG_INT(QmVoiceVadEnable, qm_voice_vad_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用语音激活（VAD）")
MACRO_CONFIG_INT(QmVoiceVadThreshold, qm_voice_vad_threshold, 8, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "VAD 阈值（百分比）")
MACRO_CONFIG_INT(QmVoiceVadReleaseDelayMs, qm_voice_vad_release_delay_ms, 150, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "VAD 释放延迟（毫秒）")
MACRO_CONFIG_INT(QmVoiceIgnoreDistance, qm_voice_ignore_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "忽略语音距离衰减")
MACRO_CONFIG_INT(QmVoiceGroupGlobal, qm_voice_group_global, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "同组全图收听")
MACRO_CONFIG_INT(QmVoiceVisibilityMode, qm_voice_visibility_mode, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "可见性过滤模式")
MACRO_CONFIG_INT(QmVoiceListMode, qm_voice_list_mode, 0, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "名单过滤模式")
MACRO_CONFIG_STR(QmVoiceWhitelist, qm_voice_whitelist, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音白名单（逗号分隔）")
MACRO_CONFIG_STR(QmVoiceBlacklist, qm_voice_blacklist, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音黑名单（逗号分隔）")
MACRO_CONFIG_STR(QmVoiceMute, qm_voice_mute, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "语音静音名单（逗号分隔）")
MACRO_CONFIG_INT(QmVoiceHearVad, qm_voice_hear_vad, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "接收 VAD 讲话者")
MACRO_CONFIG_STR(QmVoiceVadAllow, qm_voice_vad_allow, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "VAD 允许名单（逗号分隔）")
MACRO_CONFIG_STR(QmVoiceNameVolumes, qm_voice_name_volumes, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "按名字单独音量（name=percent）")
MACRO_CONFIG_INT(QmVoiceShowIndicator, qm_voice_show_indicator, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示语音指示图标")
MACRO_CONFIG_INT(QmVoiceIndicatorAboveSelf, qm_voice_indicator_above_self, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示自己头顶语音指示")
MACRO_CONFIG_INT(QmVoiceShowPing, qm_voice_show_ping, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "显示语音延迟")
MACRO_CONFIG_INT(QmVoiceDebug, qm_voice_debug, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "输出语音调试日志")
MACRO_CONFIG_INT(QmVoiceOffNonActive, qm_voice_off_nonactive, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "窗口失焦时暂停语音")
MACRO_CONFIG_INT(QmVoicePttReleaseDelayMs, qm_voice_ptt_release_delay_ms, 0, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "PTT 释放延迟（毫秒）")
MACRO_CONFIG_INT(QmVoiceHearOnSpecPos, qm_voice_hear_on_spec_pos, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "旁观时按镜头中心收听")
MACRO_CONFIG_INT(QmVoiceHearPeoplesInSpectate, qm_voice_hear_peoples_in_spectate, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "接收旁观/非活跃玩家语音")

// Streamer Mode / 主播模式
MACRO_CONFIG_INT(QmStreamerHideNames, qm_streamer_hide_names, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "隐藏非好友姓名/部落并显示客户端 ID")
MACRO_CONFIG_INT(QmStreamerHideSkins, qm_streamer_hide_skins, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "非好友使用默认皮肤")
MACRO_CONFIG_INT(QmStreamerScoreboardDefaultFlags, qm_streamer_scoreboard_default_flags, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "在记分牌中显示默认国旗")

// Friend Online Notifications / 好友上线提醒
MACRO_CONFIG_INT(QmFriendOnlineNotify, qm_friend_online_notify, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友上线提醒")
MACRO_CONFIG_INT(QmFriendOnlineAutoRefresh, qm_friend_online_auto_refresh, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友提醒自动刷新服务器列表")
MACRO_CONFIG_INT(QmFriendOnlineRefreshSeconds, qm_friend_online_refresh_seconds, 30, 5, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友提醒刷新间隔(秒)")
MACRO_CONFIG_INT(QmFriendEnterAutoGreet, qm_friend_enter_auto_greet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友进图自动打招呼")
MACRO_CONFIG_INT(QmFriendEnterBroadcast, qm_friend_enter_broadcast, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "大字显示好友进服")
MACRO_CONFIG_STR(QmFriendEnterBroadcastText, qm_friend_enter_broadcast_text, 128, "%s好友进入本服", CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友进服大字提示文本（使用%s表示好友名）")
MACRO_CONFIG_STR(QmFriendEnterGreetText, qm_friend_enter_greet_text, 128, "你好啊!", CFGFLAG_CLIENT | CFGFLAG_SAVE, "好友进图自动打招呼文本")

// Block Words / 屏蔽词
MACRO_CONFIG_INT(QmBlockWordsEnabled, qm_block_words_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "启用屏蔽词列表")
MACRO_CONFIG_INT(QmBlockWordsShowConsole, qm_block_words_show_console, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "控制台显示被屏蔽词")
MACRO_CONFIG_COL(QmBlockWordsConsoleColor, qm_block_words_console_color, 0xFFFFFFFF, CFGFLAG_CLIENT | CFGFLAG_SAVE, "被屏蔽词控制台颜色")
MACRO_CONFIG_INT(QmBlockWordsMultiReplace, qm_block_words_multi_replace, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "按屏蔽词长度多字符替换")
MACRO_CONFIG_INT(QmBlockWordsMode, qm_block_words_mode, 2, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "替换方式 0=Regex 1=Full 2=Both")
MACRO_CONFIG_STR(QmBlockWordsReplacementChar, qm_block_words_replacement_char, 8, "*", CFGFLAG_CLIENT | CFGFLAG_SAVE, "屏蔽词替换字符")
MACRO_CONFIG_STR(QmBlockWordsList, qm_block_words_list, 1024, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "屏蔽词列表（用,分隔）")
MACRO_CONFIG_STR(QmSidebarCardOrder, qm_sidebar_card_order, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "栖梦侧栏模块排序")
MACRO_CONFIG_STR(QmSidebarCardCollapsed, qm_sidebar_card_collapsed, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "栖梦侧栏模块折叠状态")
MACRO_CONFIG_STR(QmSidebarCardUsage, qm_sidebar_card_usage, 1024, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "栖梦侧栏模块使用频率")
