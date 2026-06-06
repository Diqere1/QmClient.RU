QmClient 翻译文件
=================

这些文件用于存放 QmClient 专属 UI 文案的翻译。
它们会在 DDNet 的基础翻译之上，以增量方式继续加载。

文件格式：
  key
  == translation

- QmClient 语言文件以原始的 QmClient UI 文案作为 key，目前主模式为中文 key。
- 仍可能保留少量历史遗留或基础领域继承下来的英文 key，这些应视为兼容性条目，而不是主要模式。

如何补充翻译：
  1. 打开目标语言文件（例如 `german.txt`）。
  2. 找到仍然使用英文占位翻译的条目。
  3. 将英文翻译替换为对应语言的翻译文本。
  4. 保留每一行翻译前面的 `== ` 前缀。
  5. 不要修改 key 行（也就是没有 `== ` 的那一行）。

示例：
  修改前：
    消息气泡
    == Chat Bubble

  修改后（德语）：
    消息气泡
    == Chat-Blase

当前状态：
  - 中文 key 是当前 QmClient 文案的主模式，源码中的中文文案直接作为 key 使用
  - 简体中文环境：直接使用源码中的中文 key，不再额外加载 `simplified_chinese.txt`
  - `english.txt`：为中文 key 提供对应的英文翻译
  - 其他语言文件：目前默认使用英文占位翻译，后续可逐步补全为对应语言

由 `qmclient_scripts/languages_qmclient/generate_all.py` 自动生成
