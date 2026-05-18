#!/usr/bin/env python3
"""
根据 CodeStable 产物生成一份发布说明初稿。

目标不是替代人工，而是把 feature / issue / gate 的事实先汇总成可改写的 Markdown。
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INCLUDED_STATUSES = {
    "accepted",
    "approved",
    "done",
    "fixed",
    "completed",
    "active",
}
STATUS_ALIASES = {
    "pass": "accepted",
    "confirmed": "fixed",
}


@dataclass
class ArtifactItem:
    slug: str
    path: Path
    status: str
    normalized_status: str
    summary: str


def parse_frontmatter(text: str) -> tuple[dict[str, str], str]:
    if not text.startswith("---\n"):
        return {}, text
    end_marker = "\n---\n"
    end_index = text.find(end_marker, 4)
    if end_index < 0:
        return {}, text
    frontmatter_text = text[4:end_index]
    body = text[end_index + len(end_marker) :]
    data: dict[str, str] = {}
    for line in frontmatter_text.splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        data[key.strip()] = value.strip()
    return data, body


def extract_section_bullets(body: str, headings: list[str]) -> list[str]:
    lines = body.splitlines()
    for heading in headings:
        target = f"## {heading}"
        for index, line in enumerate(lines):
            if line.strip() != target:
                continue
            bullets: list[str] = []
            cursor = index + 1
            while cursor < len(lines):
                current = lines[cursor]
                stripped = current.strip()
                if stripped.startswith("## "):
                    break
                if stripped.startswith("- "):
                    bullets.append(stripped[2:].strip())
                cursor += 1
            if bullets:
                return bullets
    return []


def extract_section_paragraph(body: str, headings: list[str]) -> str:
    lines = body.splitlines()
    for heading in headings:
        target = f"## {heading}"
        for index, line in enumerate(lines):
            if line.strip() != target:
                continue
            cursor = index + 1
            collected: list[str] = []
            while cursor < len(lines):
                current = lines[cursor].strip()
                if current.startswith("## "):
                    break
                if current and not current.startswith(">"):
                    collected.append(current)
                cursor += 1
            if collected:
                return " ".join(collected[:3])
    return ""


def normalize_status(status: str) -> str:
    normalized = status.strip().lower()
    return STATUS_ALIASES.get(normalized, normalized)


def collect_artifacts(root: Path, suffix: str, included_statuses: set[str]) -> tuple[list[ArtifactItem], list[ArtifactItem]]:
    items: list[ArtifactItem] = []
    skipped_items: list[ArtifactItem] = []
    if not root.exists():
        return items, skipped_items
    for path in sorted(root.rglob(f"*{suffix}")):
        parent_slug = path.parent.name
        text = path.read_text(encoding="utf-8")
        frontmatter, body = parse_frontmatter(text)
        bullets = extract_section_bullets(body, ["最终结论", "修复内容", "遗留"])
        paragraph = extract_section_paragraph(body, ["最终结论", "修复内容", "遗留"])
        summary = bullets[0] if bullets else paragraph
        normalized_status = normalize_status(frontmatter.get("status", "unknown"))
        item = ArtifactItem(
            slug=parent_slug,
            path=path.relative_to(REPO_ROOT),
            status=frontmatter.get("status", "unknown"),
            normalized_status=normalized_status,
            summary=summary if summary else "未抽取到稳定摘要，请人工补充。",
        )
        if item.normalized_status in included_statuses:
            items.append(item)
        else:
            skipped_items.append(item)
    return items, skipped_items


def load_gate_summary(path: Path | None) -> dict[str, Any] | None:
    if path is None or not path.exists():
        return None
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def resolve_repo_path(path: Path | None) -> Path | None:
    if path is None:
        return None
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def render_markdown(
    version: str,
    features: list[ArtifactItem],
    fixes: list[ArtifactItem],
    skipped_features: list[ArtifactItem],
    skipped_fixes: list[ArtifactItem],
    gate_summary: dict[str, Any] | None,
    included_statuses: set[str],
) -> str:
    lines: list[str] = []
    lines.append(f"# QmClient {version}")
    lines.append("")
    lines.append("## 本次重点")
    if features:
        lines.append(f"- Feature：已沉淀 {len(features)} 项功能产物，见下方条目。")
    else:
        lines.append("- Feature：本次没有自动扫描到 feature acceptance 产物，请人工补充。")
    if fixes:
        lines.append(f"- Fix：已沉淀 {len(fixes)} 项问题修复产物，见下方条目。")
    else:
        lines.append("- Fix：本次没有自动扫描到 issue fix-note 产物，请人工补充。")
    lines.append("")
    lines.append("## 新增")
    if features:
        for item in features:
            lines.append(f"- `{item.slug}` ({item.normalized_status}) - {item.summary} 产物：`{item.path.as_posix()}`")
    else:
        lines.append("- [补充用户可感知的新能力]")
    lines.append("")
    lines.append("## 修复")
    if fixes:
        for item in fixes:
            lines.append(f"- `{item.slug}` ({item.normalized_status}) - {item.summary} 产物：`{item.path.as_posix()}`")
    else:
        lines.append("- [补充用户可感知的问题修复]")
    lines.append("")
    lines.append("## 调整")
    lines.append("- [补充行为、流程或界面调整]")
    lines.append("")
    lines.append("## 兼容性说明")
    lines.append("- [确认是否影响协议、demo、skin、地图行为或客户端配置]")
    lines.append("")
    lines.append("## 已知限制")
    lines.append("- [补充当前仍保留的限制或实验路径]")
    lines.append("")
    if skipped_features or skipped_fixes:
        lines.append("## 待人工确认的非正式产物")
        lines.append(f"- 当前状态白名单：`{', '.join(sorted(included_statuses))}`")
        for item in skipped_features:
            lines.append(f"- 跳过 Feature：`{item.slug}` ({item.normalized_status}) 原始状态：`{item.status}` 产物：`{item.path.as_posix()}`")
        for item in skipped_fixes:
            lines.append(f"- 跳过 Fix：`{item.slug}` ({item.normalized_status}) 原始状态：`{item.status}` 产物：`{item.path.as_posix()}`")
        lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("# Maintainer Notes")
    lines.append("")
    lines.append("## 变更范围")
    lines.append(f"- Feature artifacts: {len(features)}")
    lines.append(f"- Fix artifacts: {len(fixes)}")
    lines.append("- Workflow artifacts: release notes 初稿由 `qmclient_scripts/generate_release_notes.py` 生成")
    lines.append("")
    lines.append("## 验证摘要")
    if gate_summary is not None:
        summary = gate_summary.get("Summary", {})
        lines.append(f"- Gate 模式：`{gate_summary.get('Mode', 'unknown')}`")
        lines.append(f"- Gate 结果：PASS={summary.get('Pass', 'n/a')} WARN={summary.get('Warn', 'n/a')} FAIL={summary.get('Fail', 'n/a')}")
    else:
        lines.append("- Gate：未提供 `--gate-report`，请人工补充最新总入口结果。")
    lines.append("- 构建 / 测试： [补充本次实际执行的验证命令]")
    lines.append("- 发布态验证： [补充是否做过 build-ninja / Release 真实运行]")
    lines.append("")
    lines.append("## 素材来源")
    lines.append("- `.ai/features/*/*-acceptance.md`")
    lines.append("- `.ai/issues/*/*-fix-note.md`")
    lines.append("- `qmclient_scripts/gate/check-gate.sh` JSON 报告（如果提供）")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="生成 QmClient 发布说明初稿")
    parser.add_argument("--version", default="UNRELEASED", help="版本号，如 v2.3.0")
    parser.add_argument("--gate-report", type=Path, default=None, help="check-gate.sh 生成的 JSON 报告路径")
    parser.add_argument("--output", type=Path, default=None, help="输出 Markdown 文件路径；不传则打印到 stdout")
    parser.add_argument(
        "--include-status",
        action="append",
        default=[],
        help="额外允许纳入正式初稿的状态，可重复传递",
    )
    args = parser.parse_args()

    included_statuses = set(DEFAULT_INCLUDED_STATUSES)
    included_statuses.update(normalize_status(status) for status in args.include_status if status.strip())
    gate_report_path = resolve_repo_path(args.gate_report)
    output_path = resolve_repo_path(args.output)
    features, skipped_features = collect_artifacts(REPO_ROOT / ".ai" / "features", "-acceptance.md", included_statuses)
    fixes, skipped_fixes = collect_artifacts(REPO_ROOT / ".ai" / "issues", "-fix-note.md", included_statuses)
    gate_summary = load_gate_summary(gate_report_path)
    markdown = render_markdown(
        args.version,
        features,
        fixes,
        skipped_features,
        skipped_fixes,
        gate_summary,
        included_statuses,
    )

    if output_path is not None:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(markdown, encoding="utf-8")
        print(f"已写入发布说明初稿：{output_path}")
    else:
        print(markdown)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
