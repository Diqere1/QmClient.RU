#!/usr/bin/env python3
"""
检查仓库工作流文档入口是否仍然一致。

目标：
1. 防止 AGENTS / Claude / attention / workflow-entry 漂移。
2. 防止 workflow-entry 引用的专项文档或脚本失效。
3. 用中文输出可直接并入 check-gate 汇总。
"""

from __future__ import annotations

import re
import sys
import json
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = REPO_ROOT / ".codestable" / "reference" / "workflow-manifest.json"


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
    for match in re.findall(r"`((?:\.codestable|qmclient_scripts|scripts)/[^`]+?\.(?:md|ps1|py))`", text):
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


def extract_scalar_value(line: str) -> str:
    if ":" not in line:
        return ""
    return line.split(":", 1)[1].strip().strip('"').strip("'")


def parse_governance_jobs(text: str) -> dict[str, WorkflowJob]:
    lines = text.splitlines()
    jobs: dict[str, WorkflowJob] = {}
    in_jobs = False
    current_job_id = ""
    current_steps: list[WorkflowStep] = []
    current_step: WorkflowStep | None = None
    in_run_block = False
    run_block_indent = 0
    jobs_indent: int | None = None
    step_indent: int | None = None

    def flush_step() -> None:
        nonlocal current_step, current_steps, in_run_block, run_block_indent
        if current_step is not None:
            current_steps.append(current_step)
        current_step = None
        in_run_block = False
        run_block_indent = 0

    def flush_job() -> None:
        nonlocal current_job_id, current_steps, step_indent
        flush_step()
        if current_job_id:
            jobs[current_job_id] = WorkflowJob(job_id=current_job_id, steps=list(current_steps))
        current_job_id = ""
        current_steps = []
        step_indent = None

    for line in lines:
        stripped = line.strip()
        indent = len(line) - len(line.lstrip(" "))

        if not in_jobs:
            if stripped == "jobs:":
                in_jobs = True
            continue

        if indent == 0 and stripped:
            break

        if stripped.endswith(":") and not stripped.startswith("- "):
            if jobs_indent is None and indent > 0:
                jobs_indent = indent
            if jobs_indent is not None and indent == jobs_indent:
                flush_job()
                current_job_id = stripped[:-1].strip()
                continue

        if current_step is not None and in_run_block:
            if stripped and indent > run_block_indent:
                current_step.run = f"{current_step.run}\n{stripped}" if current_step.run else stripped
                current_step.raw_lines.append(stripped)
                continue
            in_run_block = False
            run_block_indent = 0

        if not current_job_id:
            continue

        if stripped.startswith("- name:"):
            if step_indent is None and indent > 0:
                step_indent = indent
            if step_indent is not None and indent == step_indent:
                flush_step()
                current_step = WorkflowStep(
                    name=extract_scalar_value(stripped),
                    raw_lines=[stripped],
                    if_condition="",
                    run="",
                    uses="",
                )
                continue

        if current_step is None:
            continue

        if stripped:
            current_step.raw_lines.append(stripped)

        minimum_field_indent = (step_indent + 1) if step_indent is not None else 1
        if indent >= minimum_field_indent and stripped.startswith("if:"):
            current_step.if_condition = extract_scalar_value(stripped)
        elif indent >= minimum_field_indent and stripped.startswith("uses:"):
            current_step.uses = extract_scalar_value(stripped)
        elif indent >= minimum_field_indent and stripped.startswith("run:"):
            run_value = stripped.split(":", 1)[1].strip()
            if run_value in {"|", ">"}:
                in_run_block = True
                run_block_indent = indent
                current_step.run = ""
            else:
                current_step.run = run_value
            continue

        if jobs_indent is None:
            continue

        if indent < jobs_indent and stripped:
            flush_job()

    flush_job()
    return jobs


def check_governance_workflow_structure(text: str, manifest: dict) -> CheckResult:
    jobs = parse_governance_jobs(text)
    required_jobs = manifest.get("governance_workflow_required_jobs", [])
    problems: list[str] = []

    for required_job in required_jobs:
        job_id = required_job.get("job_id", "")
        workflow_job = jobs.get(job_id)
        if workflow_job is None:
            problems.append(f"缺少 job: {job_id}")
            continue

        steps_by_name = {step.name: step for step in workflow_job.steps}
        for required_step in required_job.get("required_steps", []):
            step_name = required_step.get("name", "")
            workflow_step = steps_by_name.get(step_name)
            if workflow_step is None:
                problems.append(f"{job_id} 缺少 step: {step_name}")
                continue

            expected_if = required_step.get("must_have_if", "")
            if expected_if and workflow_step.if_condition != expected_if:
                problems.append(f"{job_id}/{step_name} 缺少 if: {expected_if}")

            searchable_text = "\n".join(
                part for part in [workflow_step.run, workflow_step.uses, "\n".join(workflow_step.raw_lines)] if part
            )
            for snippet in required_step.get("must_contain", []):
                if snippet not in searchable_text:
                    problems.append(f"{job_id}/{step_name} 缺少片段: {snippet}")

    return CheckResult(
        ok=not problems,
        title="governance CI 结构化语义",
        detail="通过" if not problems else "; ".join(problems),
    )


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
    attention = read_text(".codestable/attention.md")
    workflow = read_text(".codestable/reference/workflow-entry.md")

    agents_order = manifest.get("agents_root_order", [])
    results.append(
        CheckResult(
            ok=ordered_contains(agents, agents_order),
            title="AGENTS 文档入口顺序",
            detail="通过" if ordered_contains(agents, agents_order) else "未按 AGENTS -> attention -> workflow-entry 顺序声明入口",
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

    attention_required_items = manifest.get("attention_required_references", [])
    missing_attention = [snippet for snippet in attention_required_items if snippet not in attention]
    results.append(
        CheckResult(
            ok=not missing_attention,
            title="attention 分流入口",
            detail="通过" if not missing_attention else f"缺少引用: {', '.join(missing_attention)}",
        )
    )

    referenced_paths = collect_markdown_paths(workflow)
    missing_referenced = [path for path in referenced_paths if not path_exists_or_is_glob(path)]
    results.append(
        CheckResult(
            ok=not missing_referenced,
            title="workflow-entry 引用可达性",
            detail="通过" if not missing_referenced else f"引用存在断链: {', '.join(missing_referenced)}",
        )
    )

    missing_routes: list[str] = []
    for route in manifest.get("workflow_routes", []):
        route_name = route.get("name", "unknown")
        for required_path in route.get("must_reference", []):
            quoted = f"`{required_path}`"
            if quoted not in workflow:
                missing_routes.append(f"{route_name}:{required_path}")
    results.append(
        CheckResult(
            ok=not missing_routes,
            title="workflow-entry 路由完整性",
            detail="通过" if not missing_routes else f"缺少路由入口: {', '.join(missing_routes)}",
        )
    )

    workflow_required_sections = manifest.get("workflow_entry_required_sections", [])
    missing_workflow_sections = missing_snippets(workflow, workflow_required_sections)
    results.append(
        CheckResult(
            ok=not missing_workflow_sections,
            title="workflow-entry 核心分节",
            detail="通过" if not missing_workflow_sections else f"缺少分节: {', '.join(missing_workflow_sections)}",
        )
    )

    release_notes_required = manifest.get("release_notes_required_references", [])
    release_notes_text = read_text(".codestable/reference/release-notes-template.md")
    missing_release_notes = missing_snippets(release_notes_text, release_notes_required)
    results.append(
        CheckResult(
            ok=not missing_release_notes,
            title="发布说明脚本入口",
            detail="通过" if not missing_release_notes else f"缺少引用: {', '.join(missing_release_notes)}",
        )
    )

    governance_workflow_text = read_text(".github/workflows/governance.yml")
    governance_required_snippets = manifest.get("governance_workflow_required_snippets", [])
    missing_governance_snippets = missing_snippets(governance_workflow_text, governance_required_snippets)
    results.append(
        CheckResult(
            ok=not missing_governance_snippets,
            title="governance CI 关键语义",
            detail="通过" if not missing_governance_snippets else f"缺少 CI 关键片段: {', '.join(missing_governance_snippets)}",
        )
    )
    results.append(check_governance_workflow_structure(governance_workflow_text, manifest))

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
