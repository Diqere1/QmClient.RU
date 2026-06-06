# QmClient Perf — 性能日志分析工具

## 快速开始

```bash
cd qmclient_scripts/perf
npm install
npx tsx analyze.ts          # 自动读取最新日志
npx tsx analyze.ts path/to/qm_perf_xxx.log  # 指定日志文件
```

## 前置条件

游戏运行时需开启：

```
qm_perf_debug 1
qm_perf_logfile 1
```

日志输出到 `%APPDATA%/DDNet/dumps/QmClient_Perf/qm_perf_*.log`。

## 输出

生成与日志同名的 `_report.html` 文件，浏览器打开即可查看交互式报表。

## 报表内容

- KPI 卡片: p50 / p95 / p99 / Max / 尖峰数量
- 帧时间趋势图（全 session，可缩放）
- 帧时间分布直方图
- 百分位对比柱状图
- 尖峰帧详情表格

## 开发

```bash
npm run analyze    # 等同于 npx tsx analyze.ts
```
