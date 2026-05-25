#!/usr/bin/env python3
"""
检查仓库 harness / workflow 文档入口是否仍然一致。

目标：
1. 防止 AGENTS / CLAUDE / .ai 分层文档漂移。
2. 防止 feature_list / progress / session-handoff 状态文件失去结构。
3. 防止 reference.md 引用的脚本失效。
4. 用中文输出可直接并入 check-gate 汇总。
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


def count_lines(text: str) -> int:
    if not text:
        return 0
    return len(text.replace("\r\n", "\n").replace("\r", "\n").splitlines())


def normalize_full_sync_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n")


def collect_markdown_paths(text: str) -> list[str]:
    seen: set[str] = set()
    matches: list[str] = []
    for match in re.findall(
        r"`((?:\.ai|qmclient_scripts|scripts|feature_list|progress|session-handoff|init)[^`]*?\.(?:md|ps1|py|sh|json)|init\.sh|feature_list\.json|progress\.md|session-handoff\.md)`",
        text,
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


def check_feature_list(manifest: dict) -> CheckResult:
    path = REPO_ROOT / "feature_list.json"
    if not path.exists():
        return CheckResult(False, "feature_list.json 结构", "缺失 feature_list.json")

    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        return CheckResult(False, "feature_list.json 结构", f"JSON 解析失败: {error}")

    features = payload.get("features")
    if not isinstance(features, list) or not features:
        return CheckResult(False, "feature_list.json 结构", "features 必须是非空数组")

    allowed_status = set(manifest.get("feature_list_status_values", []))
    max_in_progress = int(manifest.get("feature_list_max_in_progress", 1))
    problems: list[str] = []
    seen_ids: set[str] = set()
    in_progress_count = 0

    for index, feature in enumerate(features):
        if not isinstance(feature, dict):
            problems.append(f"features[{index}] 不是对象")
            continue
        feature_id = str(feature.get("id", "")).strip()
        name = str(feature.get("name", "")).strip()
        description = str(feature.get("description", "")).strip()
        status = str(feature.get("status", "")).strip()
        dependencies = feature.get("dependencies", [])

        if not feature_id:
            problems.append(f"features[{index}] 缺少 id")
        elif feature_id in seen_ids:
            problems.append(f"重复 id: {feature_id}")
        seen_ids.add(feature_id)

        if not name:
            problems.append(f"{feature_id or index} 缺少 name")
        if not description:
            problems.append(f"{feature_id or index} 缺少 description")
        if status not in allowed_status:
            problems.append(f"{feature_id or index} status 非法: {status}")
        if status == "in-progress":
            in_progress_count += 1
        if not isinstance(dependencies, list):
            problems.append(f"{feature_id or index} dependencies 必须是数组")

    if in_progress_count > max_in_progress:
        problems.append(f"in-progress 数量为 {in_progress_count}，最多允许 {max_in_progress}")

    return CheckResult(
        ok=not problems,
        title="feature_list.json 结构",
        detail="通过" if not problems else "; ".join(problems),
    )


def check_state_file_sections(manifest: dict) -> CheckResult:
    problems: list[str] = []
    for relative_path, sections in manifest.get("state_required_sections", {}).items():
        path = REPO_ROOT / relative_path
        if not path.exists():
            problems.append(f"{relative_path} 缺失")
            continue
        text = path.read_text(encoding="utf-8")
        missing = missing_snippets(text, list(sections))
        if missing:
            problems.append(f"{relative_path} 缺少分节: {', '.join(missing)}")

    return CheckResult(
        ok=not problems,
        title="状态文件结构",
        detail="通过" if not problems else "; ".join(problems),
    )


def check_init_script(manifest: dict) -> CheckResult:
    path = REPO_ROOT / "init.sh"
    if not path.exists():
        return CheckResult(False, "init.sh harness 入口", "缺失 init.sh")
    text = path.read_text(encoding="utf-8")
    missing = missing_snippets(text, manifest.get("init_required_snippets", []))
    return CheckResult(
        ok=not missing,
        title="init.sh harness 入口",
        detail="通过" if not missing else f"缺少片段: {', '.join(missing)}",
    )


def check_root_map_size(text: str, manifest: dict) -> CheckResult:
    max_lines = int(manifest.get("root_map_max_lines", 0))
    if max_lines <= 0:
        return CheckResult(True, "AGENTS 根地图长度", "未配置限制")
    line_count = count_lines(text)
    return CheckResult(
        ok=line_count <= max_lines,
        title="AGENTS 根地图长度",
        detail=f"通过 ({line_count}/{max_lines} 行)" if line_count <= max_lines else f"过长: {line_count}/{max_lines} 行，应把细节下沉到 .ai/",
    )


def check_forbidden_snippets(text: str, manifest: dict) -> CheckResult:
    forbidden = [snippet for snippet in manifest.get("agents_forbidden_snippets", []) if snippet in text]
    return CheckResult(
        ok=not forbidden,
        title="AGENTS 地图化约束",
        detail="通过" if not forbidden else f"根入口包含应下沉的旧分节: {', '.join(forbidden)}",
    )


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
    sync_mode = str(manifest.get("claude_sync_mode", ""))
    if sync_mode == "sync_agents_first":
        sync_exit_code, sync_output = run_sync_script("agents")
        results.append(
            CheckResult(
                ok=sync_exit_code == 0,
                title="CLAUDE / AGENTS 自动同步",
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
                title="CLAUDE / AGENTS 自动同步",
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
                title="CLAUDE / AGENTS 自动同步",
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
    claude = read_text("CLAUDE.md")
    reference = read_text(".ai/reference.md")

    agents_order = manifest.get("agents_root_order", [])
    results.append(
        CheckResult(
            ok=ordered_contains(agents, agents_order),
            title="AGENTS 文档入口顺序",
            detail="通过" if ordered_contains(agents, agents_order) else "未按 harness 地图顺序声明 .ai 入口",
        )
    )

    results.append(check_root_map_size(agents, manifest))
    results.append(check_forbidden_snippets(agents, manifest))

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
            title="CLAUDE 文档同步入口",
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
                title="CLAUDE 与 AGENTS 全文同步",
                detail="通过" if agents_normalized == claude_normalized else "CLAUDE.md 与 AGENTS.md 未保持 1:1 全文一致（仅允许换行差异）",
            )
        )

    focused_docs = manifest.get("focused_ai_docs", [])
    missing_focused_refs = [path for path in focused_docs if f"`{path}`" not in agents]
    results.append(
        CheckResult(
            ok=not missing_focused_refs,
            title="AGENTS 分层文档地图",
            detail="通过" if not missing_focused_refs else f"缺少 .ai 地图引用: {', '.join(missing_focused_refs)}",
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

    results.append(check_feature_list(manifest))
    results.append(check_state_file_sections(manifest))
    results.append(check_init_script(manifest))

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
