import assert from 'node:assert/strict';

import { parseLine, parseLog } from './lib/parse.ts';
import { generateReport } from './lib/report.ts';

function testParseKeepsEventOnlyPerfLines() {
  const line = '2026-06-04 12:00:00 I perf/interaction: event=scroll_begin frame=42 page=settings:tee visible_rows=8';
  const entry = parseLine(line);
  assert.ok(entry);
  assert.equal(entry.system, 'perf/interaction');
  assert.equal(entry.fields.event, 'scroll_begin');

  const entries = parseLog([
    line,
    '2026-06-04 12:00:01 I perf/skin-ux: event=first_visible_ready dur_ms=123.500 frame=45 page=settings:tee',
  ].join('\n'));
  assert.equal(entries.length, 2);
}

function testParseSupportsJsonLinesEvents() {
  const entry = parseLine('{"timestamp":"2026-06-04T12:00:02","system":"perf/device","event":"sample","frame":77,"gpu_util_percent":61.5}');
  assert.ok(entry);
  assert.equal(entry?.system, 'perf/device');
  assert.equal(entry?.fields.event, 'sample');
  assert.equal(entry?.fields.frame, 77);

  const prefixed = parseLine('2026-06-04 12:00:03 I perf/settings-invalidate: {"system":"perf/settings-invalidate","frame":88,"session":9,"reason":"config_hash_changed","text":1}');
  assert.ok(prefixed);
  assert.equal(prefixed?.system, 'perf/settings-invalidate');
  assert.equal(prefixed?.fields.reason, 'config_hash_changed');
  assert.equal(prefixed?.fields.frame, 88);
}

function testReportIncludesInteractionAndDeviceSections() {
  const entries = parseLog([
    '2026-06-04 12:00:00 I perf/menu: stage=settings_page_content duration_ms=6.000 frame=10 page=settings:tee',
    '2026-06-04 12:00:01 I perf/interaction: event=tee_enter frame=10 page=settings:tee visible_rows=8 first_visible_skin=default',
    '2026-06-04 12:00:02 I perf/skin-ux: event=first_visible_ready dur_ms=123.500 frame=12 page=settings:tee visible_rows=8',
    '2026-06-04 12:00:03 I perf/device: event=sample frame=12 gpu_util_percent=61.5 gpu_dedicated_vram_mb=512 gpu_shared_vram_mb=96 cpu_process_percent=12 cpu_total_percent=34 memory_process_mb=1024 disk_read_mb_s=4.5',
  ].join('\n'));
  const html = generateReport(entries, 'qm_perf_test.log', null);
  assert.match(html, /交互窗口/);
  assert.match(html, /Tee 收敛/);
  assert.match(html, /设备资源/);
}

testParseKeepsEventOnlyPerfLines();
testParseSupportsJsonLinesEvents();
testReportIncludesInteractionAndDeviceSections();

console.log('qmclient perf tests passed');
