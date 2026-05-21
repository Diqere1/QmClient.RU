#!/usr/bin/env python3
"""
检查仓库工作流文档入口是否仍然一致。

目标：
1. 防止 AGENTS / Claude / reference.md 漂移。
2. 防止 reference.md 引用的脚本失效。
3. 用中文输出可直接并入 check-gate 汇总。
"""

from __future__ import annotations

import re
import sys
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = REPO_ROOT / ".ai" / "workflow-manifest.json"
SYNC_SCRIPT_PATH = REPO_ROOT / "qmclient_scripts" / "gate" / "sync_agents_claude.py"


@dataclass
class CheckResult:
    ok: bool
    title: str
    detail: str


@dataclass
class WorkflowStep:
    name: str
    raw_lines: list[str]
    if_condition: str
    run: str
    uses: str


@dataclass
class WorkflowJob:
    job_id: str
    steps: list[WorkflowStep]


def read_text(relative_path: str) -> str:
    return (REPO_ROOT / relative_path).read_text(encoding="utf-8")


def read_manifest() -> dict:
    with MANIFEST_PATH.open("r", encoding="utf-8") as file:
        return json.load(file)


def run_sync_script(prefer: str = "auto") -> tuple[int, str]:
    command = [sys.executable, str(SYNC_SCRIPT_PATH), "--prefer", prefer]
    completed = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True, encoding="utf-8")
    output = (completed.stdout or "") + (completed.stderr or "")
    return completed.returncode, output.strip()


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


def normalize_full_sync_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n")


def collect_markdown_paths(text: str) -> list[str]:
    seen: set[str] = set()
    matches: list[str] = []
    for match in re.findall(r"`((?:\.ai|qmclient_scripts|scripts)/[^`]+?\.(?:md|ps1|py|sh|json))`", text):
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


def discover_unmanaged_governance_files(manifest: dict, required_files: list[str]) -> list[str]:
    unmanaged: set[str] = set()
    required_file_set = set(required_files)

    for rule in manifest.get("managed_governance_directories", []):
        base = str(rule.get("base", "")).strip()
        glob = str(rule.get("glob", "")).strip()
        ignored_files = {str(item).replace("\\", "/").strip() for item in rule.get("ignored_files", []) if str(item).strip()}
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


def run_checks() -> list[CheckResult]:
    results: list[CheckResult] = []
    if not MANIFEST_PATH.exists():
        return [
            CheckResult(
                ok=False,
                title="工作流 manifest 存在性",
                detail=f"缺失: {MANIFEST_PATH.relative_to(REPO_ROOT).as_posix()}",
            )
        ]

    manifest = read_manifest()
    sync_mode = str(manifest.get("claude_sync_mode", ""))
    if sync_mode == "sync_agents_first":
        sync_exit_code, sync_output = run_sync_script("agents")
        results.append(
            CheckResult(
                ok=sync_exit_code == 0,
                title="Claude / AGENTS 自动同步",
                detail="通过" if sync_exit_code == 0 else (sync_output or "自动同步失败"),
            )
        )
        if sync_exit_code != 0:
            return results
    elif sync_mode == "sync_claude_first":
        sync_exit_code, sync_output = run_sync_script("claude")
        results.append(
            CheckResult(
                ok=sync_exit_code == 0,
                title="Claude / AGENTS 自动同步",
                detail="通过" if sync_exit_code == 0 else (sync_output or "自动同步失败"),
            )
        )
        if sync_exit_code != 0:
            return results
    elif sync_mode == "sync_auto":
        sync_exit_code, sync_output = run_sync_script("auto")
        results.append(
            CheckResult(
                ok=sync_exit_code == 0,
                title="Claude / AGENTS 自动同步",
                detail="通过" if sync_exit_code == 0 else (sync_output or "自动同步失败"),
            )
        )
        if sync_exit_code != 0:
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

    unmanaged_governance_files = discover_unmanaged_governance_files(manifest, required_files)
    results.append(
        CheckResult(
            ok=not unmanaged_governance_files,
            title="治理文件登记完整性",
            detail="通过" if not unmanaged_governance_files else f"发现未登记治理文件: {', '.join(unmanaged_governance_files)}",
        )
    )

    agents = read_text("AGENTS.md")
    claude = read_text("Claude.md")
    reference = read_text(".ai/reference.md")

    agents_order = manifest.get("agents_root_order", [])
    results.append(
        CheckResult(
            ok=ordered_contains(agents, agents_order),
            title="AGENTS 文档入口顺序",
            detail="通过" if ordered_contains(agents, agents_order) else "未按 AGENTS -> reference.md 顺序声明入口",
        )
    )

    required_agent_sections = manifest.get("agents_required_sections", [])
    missing_agent_sections = missing_snippets(agents, required_agent_sections)
    results.append(
        CheckResult(
            ok=not missing_agent_sections,
            title="AGENTS 根规则分层",
            detail="通过" if not missing_agent_sections else f"缺少节: {', '.join(missing_agent_sections)}",
        )
    )

    claude_required = manifest.get("claude_sync_references", [])
    missing_claude = [snippet for snippet in claude_required if snippet not in claude]
    results.append(
        CheckResult(
            ok=not missing_claude,
            title="Claude 文档同步入口",
            detail="通过" if not missing_claude else f"缺少引用: {', '.join(missing_claude)}",
        )
    )

    claude_sync_mode = manifest.get("claude_sync_mode", "")
    if claude_sync_mode == "match_agents_full_text_except_line_endings":
        agents_normalized = normalize_full_sync_text(agents)
        claude_normalized = normalize_full_sync_text(claude)
        results.append(
            CheckResult(
                ok=agents_normalized == claude_normalized,
                title="Claude 与 AGENTS 全文同步",
                detail="通过" if agents_normalized == claude_normalized else "Claude.md 与 AGENTS.md 未保持 1:1 全文一致（仅允许换行差异）",
            )
        )

    referenced_paths = collect_markdown_paths(reference)
    missing_referenced = [path for path in referenced_paths if not path_exists_or_is_glob(path)]
    results.append(
        CheckResult(
            ok=not missing_referenced,
            title="reference.md 引用可达性",
            detail="通过" if not missing_referenced else f"引用存在断链: {', '.join(missing_referenced)}",
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
            detail="通过" if not missing_routes else f"缺少路由入口: {', '.join(missing_routes)}",
        )
    )

    reference_required_sections = manifest.get("reference_required_sections", [])
    missing_reference_sections = missing_snippets(reference, reference_required_sections)
    results.append(
        CheckResult(
            ok=not missing_reference_sections,
            title="reference.md 核心分节",
            detail="通过" if not missing_reference_sections else f"缺少分节: {', '.join(missing_reference_sections)}",
        )
    )

    release_notes_required = manifest.get("release_notes_required_references", [])
    missing_release_notes = missing_snippets(reference, release_notes_required)
    results.append(
        CheckResult(
            ok=not missing_release_notes,
            title="发布说明脚本入口",
            detail="通过" if not missing_release_notes else f"缺少引用: {', '.join(missing_release_notes)}",
        )
    )

    return results


def main() -> int:
    results = run_checks()
    failed = [result for result in results if not result.ok]

    print("工作流文档一致性检查")
    for result in results:
        prefix = "通过" if result.ok else "失败"
        print(f"- {prefix}：{result.title} - {result.detail}")

    if failed:
        print("\n结论：存在工作流文档漂移或断链，需要修复。")
        return 1

    print("\n结论：工作流文档入口一致，未发现断链。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
