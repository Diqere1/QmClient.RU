#!/usr/bin/env python3
"""
QmClient 治理文档检查入口。

合并原 sync_agents_claude.py 与 check_workflow_docs.py 的功能，
统一处理 AGENTS / Claude 同步与工作流文档一致性。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
AGENTS_PATH = REPO_ROOT / "AGENTS.md"
CLAUDE_PATH = REPO_ROOT / "Claude.md"
MANIFEST_PATH = REPO_ROOT / ".ai" / "workflow-manifest.json"
STATE_PATH = Path(tempfile.gettempdir()) / "qmclient-agents-claude-sync-state.json"


# ------------------------------------------------------------------
# AGENTS / Claude 同步
# ------------------------------------------------------------------


@dataclass
class SyncResult:
    ok: bool
    changed: bool
    title: str
    detail: str


def normalize_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n")


def detect_newline(text: str) -> str:
    if "\r\n" in text:
        return "\r\n"
    if "\r" in text:
        return "\r"
    return "\n"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str, newline: str) -> None:
    normalized = normalize_text(text)
    rewritten = normalized.replace("\n", newline)
    path.write_text(rewritten, encoding="utf-8", newline="")


def read_state() -> dict:
    if not STATE_PATH.exists():
        return {}
    with STATE_PATH.open("r", encoding="utf-8") as file:
        return json.load(file)


def write_state(agents_text: str, claude_text: str) -> None:
    STATE_PATH.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "agents_normalized": normalize_text(agents_text),
        "claude_normalized": normalize_text(claude_text),
    }
    with STATE_PATH.open("w", encoding="utf-8", newline="\n") as file:
        json.dump(payload, file, ensure_ascii=False, indent=2)
        file.write("\n")


def choose_source(
    agents_text: str,
    claude_text: str,
    state: dict,
    prefer: str,
) -> tuple[str | None, str]:
    agents_normalized = normalize_text(agents_text)
    claude_normalized = normalize_text(claude_text)
    if agents_normalized == claude_normalized:
        return None, "AGENTS.md 与 Claude.md 已一致"

    last_agents = str(state.get("agents_normalized", ""))
    last_claude = str(state.get("claude_normalized", ""))
    has_state = bool(last_agents or last_claude)
    agents_changed = agents_normalized != last_agents
    claude_changed = claude_normalized != last_claude

    if prefer == "agents":
        return "agents", "按 AGENTS.md 作为源文件同步 Claude.md"
    if prefer == "claude":
        return "claude", "按 Claude.md 作为源文件同步 AGENTS.md"

    if not has_state:
        agents_mtime = AGENTS_PATH.stat().st_mtime_ns
        claude_mtime = CLAUDE_PATH.stat().st_mtime_ns
        if agents_mtime > claude_mtime:
            return "agents", "首次同步未找到历史状态，按更新较新的 AGENTS.md 作为源文件"
        if claude_mtime > agents_mtime:
            return "claude", "首次同步未找到历史状态，按更新较新的 Claude.md 作为源文件"
        return (
            "agents",
            "首次同步未找到历史状态，时间戳相同，默认按 AGENTS.md 作为源文件",
        )

    if agents_changed and not claude_changed:
        return "agents", "检测到仅 AGENTS.md 有新改动，将同步到 Claude.md"
    if claude_changed and not agents_changed:
        return "claude", "检测到仅 Claude.md 有新改动，将同步到 AGENTS.md"
    if not agents_changed and not claude_changed:
        return "agents", "状态文件缺失或过旧，默认按 AGENTS.md 同步 Claude.md"

    return None, "AGENTS.md 与 Claude.md 都发生了未登记改动且内容不一致，已拒绝自动覆盖"


def sync_files(prefer: str) -> SyncResult:
    if not AGENTS_PATH.exists() or not CLAUDE_PATH.exists():
        missing = []
        if not AGENTS_PATH.exists():
            missing.append("AGENTS.md")
        if not CLAUDE_PATH.exists():
            missing.append("Claude.md")
        return SyncResult(
            False, False, "AGENTS / Claude 镜像同步", f"缺少文件: {', '.join(missing)}"
        )

    agents_text = read_text(AGENTS_PATH)
    claude_text = read_text(CLAUDE_PATH)
    state = read_state()
    source, reason = choose_source(agents_text, claude_text, state, prefer)

    if source is None:
        if normalize_text(agents_text) == normalize_text(claude_text):
            write_state(agents_text, claude_text)
            return SyncResult(True, False, "AGENTS / Claude 镜像同步", reason)
        return SyncResult(False, False, "AGENTS / Claude 镜像同步", reason)

    if source == "agents":
        target_newline = detect_newline(claude_text)
        write_text(CLAUDE_PATH, agents_text, target_newline)
        final_agents = agents_text
        final_claude = agents_text
    else:
        target_newline = detect_newline(agents_text)
        write_text(AGENTS_PATH, claude_text, target_newline)
        final_agents = claude_text
        final_claude = claude_text

    write_state(final_agents, final_claude)
    target_name = "Claude.md" if source == "agents" else "AGENTS.md"
    return SyncResult(
        True, True, "AGENTS / Claude 镜像同步", f"{reason}，已更新 {target_name}"
    )


# ------------------------------------------------------------------
# 工作流文档一致性检查
# ------------------------------------------------------------------


@dataclass
class CheckResult:
    ok: bool
    title: str
    detail: str


def ordered_contains(text: str, snippets: list[str]) -> bool:
    cursor = 0
    for snippet in snippets:
        index = text.find(snippet, cursor)
        if index < 0:
            return False
        cursor = index + len(snippet)
    return True


def missing_snippets(text: str, snippets: list[str]) -> list[str]:
    return [snippet for snippet in snippets if snippet not in text]


def collect_markdown_paths(text: str) -> list[str]:
    seen: set[str] = set()
    matches: list[str] = []
    for match in re.findall(
        r"`((?:\.ai|qmclient_scripts|scripts)/[^`]+?\.(?:md|ps1|py|sh|json))`", text
    ):
        if match not in seen:
            seen.add(match)
            matches.append(match)
    return matches


def path_exists_or_is_glob(relative_path: str) -> bool:
    if "*" in relative_path:
        anchor = relative_path.split("*", 1)[0].rstrip("/\\")
        if not anchor:
            return True
        return (REPO_ROOT / anchor).exists()
    return (REPO_ROOT / relative_path).exists()


def discover_unmanaged_governance_files(
    manifest: dict, required_files: list[str]
) -> list[str]:
    unmanaged: set[str] = set()
    required_file_set = set(required_files)

    for rule in manifest.get("managed_governance_directories", []):
        base = str(rule.get("base", "")).strip()
        glob = str(rule.get("glob", "")).strip()
        ignored_files = {
            str(item).replace("\\", "/").strip()
            for item in rule.get("ignored_files", [])
            if str(item).strip()
        }
        if not base or not glob:
            continue

        base_path = REPO_ROOT / base
        if not base_path.exists():
            continue

        for path in base_path.glob(glob):
            if not path.is_file():
                continue
            relative_path = path.relative_to(REPO_ROOT).as_posix()
            if relative_path in required_file_set or relative_path in ignored_files:
                continue
            unmanaged.add(relative_path)

    return sorted(unmanaged)


def run_checks(prefer: str = "auto") -> list[CheckResult]:
    results: list[CheckResult] = []
    if not MANIFEST_PATH.exists():
        return [
            CheckResult(
                ok=False,
                title="工作流 manifest 存在性",
                detail=f"缺失: {MANIFEST_PATH.relative_to(REPO_ROOT).as_posix()}",
            )
        ]

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))

    # AGENTS / Claude 自动同步
    sync_mode = str(manifest.get("claude_sync_mode", ""))
    if sync_mode:
        derived = (
            sync_mode.replace("sync_", "")
            .replace("_first", "")
            .replace("_auto", "auto")
        )
        if derived not in ("auto", "agents", "claude"):
            derived = "auto"
        sync_result = sync_files(prefer)
        results.append(
            CheckResult(
                ok=sync_result.ok,
                title="AGENTS / Claude 镜像同步",
                detail=sync_result.detail,
            )
        )
        if not sync_result.ok:
            return results
    else:
        # manifest 未要求同步时，使用命令行指定的 prefer
        sync_result = sync_files(prefer)
        results.append(
            CheckResult(
                ok=sync_result.ok,
                title="AGENTS / Claude 镜像同步",
                detail=sync_result.detail,
            )
        )
        if not sync_result.ok:
            return results

    required_files = manifest.get("required_files", [])

    missing = [path for path in required_files if not (REPO_ROOT / path).exists()]
    results.append(
        CheckResult(
            ok=not missing,
            title="必需工作流文件存在性",
            detail="通过" if not missing else f"缺失: {', '.join(missing)}",
        )
    )

    unmanaged_governance_files = discover_unmanaged_governance_files(
        manifest, required_files
    )
    results.append(
        CheckResult(
            ok=not unmanaged_governance_files,
            title="治理文件登记完整性",
            detail="通过"
            if not unmanaged_governance_files
            else f"发现未登记治理文件: {', '.join(unmanaged_governance_files)}",
        )
    )

    agents = read_text(REPO_ROOT / "AGENTS.md")
    claude = read_text(REPO_ROOT / "Claude.md")
    reference = read_text(REPO_ROOT / ".ai/reference.md")

    agents_order = manifest.get("agents_root_order", [])
    results.append(
        CheckResult(
            ok=ordered_contains(agents, agents_order),
            title="AGENTS 文档入口顺序",
            detail="通过"
            if ordered_contains(agents, agents_order)
            else "未按 AGENTS -> reference.md 顺序声明入口",
        )
    )

    required_agent_sections = manifest.get("agents_required_sections", [])
    missing_agent_sections = missing_snippets(agents, required_agent_sections)
    results.append(
        CheckResult(
            ok=not missing_agent_sections,
            title="AGENTS 根规则分层",
            detail="通过"
            if not missing_agent_sections
            else f"缺少节: {', '.join(missing_agent_sections)}",
        )
    )

    claude_required = manifest.get("claude_sync_references", [])
    missing_claude = [snippet for snippet in claude_required if snippet not in claude]
    results.append(
        CheckResult(
            ok=not missing_claude,
            title="Claude 文档同步入口",
            detail="通过"
            if not missing_claude
            else f"缺少引用: {', '.join(missing_claude)}",
        )
    )

    claude_sync_mode = manifest.get("claude_sync_mode", "")
    if claude_sync_mode == "match_agents_full_text_except_line_endings":
        agents_normalized = normalize_text(agents)
        claude_normalized = normalize_text(claude)
        results.append(
            CheckResult(
                ok=agents_normalized == claude_normalized,
                title="Claude 与 AGENTS 全文同步",
                detail="通过"
                if agents_normalized == claude_normalized
                else "Claude.md 与 AGENTS.md 未保持 1:1 全文一致（仅允许换行差异）",
            )
        )

    referenced_paths = collect_markdown_paths(reference)
    missing_referenced = [
        path for path in referenced_paths if not path_exists_or_is_glob(path)
    ]
    results.append(
        CheckResult(
            ok=not missing_referenced,
            title="reference.md 引用可达性",
            detail="通过"
            if not missing_referenced
            else f"引用存在断链: {', '.join(missing_referenced)}",
        )
    )

    missing_routes: list[str] = []
    for route in manifest.get("workflow_routes", []):
        route_name = route.get("name", "unknown")
        for required_path in route.get("must_reference", []):
            quoted = f"`{required_path}`"
            if quoted not in reference:
                missing_routes.append(f"{route_name}:{required_path}")
    results.append(
        CheckResult(
            ok=not missing_routes,
            title="reference.md 路由完整性",
            detail="通过"
            if not missing_routes
            else f"缺少路由入口: {', '.join(missing_routes)}",
        )
    )

    reference_required_sections = manifest.get("reference_required_sections", [])
    missing_reference_sections = missing_snippets(
        reference, reference_required_sections
    )
    results.append(
        CheckResult(
            ok=not missing_reference_sections,
            title="reference.md 核心分节",
            detail="通过"
            if not missing_reference_sections
            else f"缺少分节: {', '.join(missing_reference_sections)}",
        )
    )

    release_notes_required = manifest.get("release_notes_required_references", [])
    missing_release_notes = missing_snippets(reference, release_notes_required)
    results.append(
        CheckResult(
            ok=not missing_release_notes,
            title="发布说明脚本入口",
            detail="通过"
            if not missing_release_notes
            else f"缺少引用: {', '.join(missing_release_notes)}",
        )
    )

    return results


def main() -> int:
    parser = argparse.ArgumentParser(description="QmClient 治理文档检查入口")
    parser.add_argument(
        "--prefer",
        choices=["auto", "agents", "claude"],
        default="auto",
        help="冲突或状态不明时优先采用哪一侧；默认自动判断",
    )
    _args = parser.parse_args()

    results = run_checks(_args.prefer)
    failed = [result for result in results if not result.ok]

    print("治理文档一致性检查")
    for result in results:
        prefix = "通过" if result.ok else "失败"
        print(f"- {prefix}：{result.title} - {result.detail}")

    if failed:
        print("\n结论：存在治理文档漂移或断链，需要修复。")
        return 1

    print("\n结论：治理文档入口一致，未发现断链。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
