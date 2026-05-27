from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from lib.agents_sync import normalize_text, sync_files


REPO_ROOT = Path(__file__).resolve().parents[3]

REQUIRED_FILES = [
    "AGENTS.md",
    "CLAUDE.md",
    ".ai/meta.md",
    ".ai/ddnet-development.md",
    ".ai/verification.md",
    ".ai/review.md",
    ".ai/git-workflow.md",
    "qmclient_scripts/gate/check_gate.py",
    "qmclient_scripts/gate/check_docs.py",
    "qmclient_scripts/scripts_overview.md",
    "qmclient_scripts/gate/baseline_debt_allowlist.json",
    "qmclient_scripts/gate/lib/__init__.py",
    "qmclient_scripts/gate/lib/report.py",
    "qmclient_scripts/gate/lib/runner.py",
    "qmclient_scripts/gate/lib/scope.py",
    "qmclient_scripts/gate/lib/agents_sync.py",
    "qmclient_scripts/gate/lib/docs_harness.py",
    "qmclient_scripts/generate_release_notes.py",
]

AGENTS_REQUIRED_SECTIONS = [
    "## 入口原则",
    "## 启动顺序",
    "## 文档地图",
    "## 全局硬约束",
    "## 机械化入口",
]

AGENTS_REQUIRED_REFERENCES = [
    ".ai/meta.md",
    ".ai/ddnet-development.md",
    ".ai/verification.md",
    ".ai/review.md",
    ".ai/git-workflow.md",
    "qmclient_scripts/scripts_overview.md",
]

CLAUDE_REQUIRED_REFERENCES = [
    ".ai/meta.md",
    ".ai/verification.md",
    ".ai/git-workflow.md",
]

MAX_ROOT_MAP_LINES = 120


@dataclass
class CheckResult:
    ok: bool
    title: str
    detail: str


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def missing_snippets(text: str, snippets: list[str]) -> list[str]:
    return [snippet for snippet in snippets if snippet not in text]


def count_lines(text: str) -> int:
    if not text:
        return 0
    return len(normalize_text(text).splitlines())


def check_required_files() -> CheckResult:
    missing = [path for path in REQUIRED_FILES if not (REPO_ROOT / path).exists()]
    return (
        CheckResult(True, "必需文档/脚本存在性", "通过")
        if not missing
        else CheckResult(False, "必需文档/脚本存在性", f"缺失: {', '.join(missing)}")
    )


def check_root_map(path: Path) -> list[CheckResult]:
    text = read_text(path)
    results: list[CheckResult] = []

    missing_sections = missing_snippets(text, AGENTS_REQUIRED_SECTIONS)
    results.append(
        CheckResult(True, f"{path.name} 必要分节", "通过")
        if not missing_sections
        else CheckResult(
            False, f"{path.name} 必要分节", f"缺少: {', '.join(missing_sections)}"
        )
    )

    missing_refs = [ref for ref in AGENTS_REQUIRED_REFERENCES if f"`{ref}`" not in text]
    results.append(
        CheckResult(True, f"{path.name} 入口引用", "通过")
        if not missing_refs
        else CheckResult(
            False, f"{path.name} 入口引用", f"缺少引用: {', '.join(missing_refs)}"
        )
    )

    line_count = count_lines(text)
    results.append(
        CheckResult(
            line_count <= MAX_ROOT_MAP_LINES,
            f"{path.name} 长度",
            f"通过 ({line_count}/{MAX_ROOT_MAP_LINES} 行)"
            if line_count <= MAX_ROOT_MAP_LINES
            else f"过长: {line_count}/{MAX_ROOT_MAP_LINES} 行",
        )
    )
    return results


def check_claude_doc(path: Path) -> CheckResult:
    text = read_text(path)
    missing_refs = [ref for ref in CLAUDE_REQUIRED_REFERENCES if f"`{ref}`" not in text]
    return (
        CheckResult(True, "CLAUDE 关键引用", "通过")
        if not missing_refs
        else CheckResult(
            False, "CLAUDE 关键引用", f"缺少引用: {', '.join(missing_refs)}"
        )
    )


def run_checks(prefer: str = "auto") -> list[CheckResult]:
    results: list[CheckResult] = []

    sync_result = sync_files(prefer)
    results.append(
        CheckResult(sync_result.ok, "AGENTS / CLAUDE 镜像同步", sync_result.detail)
    )
    if not sync_result.ok:
        return results

    results.append(check_required_files())
    results.extend(check_root_map(REPO_ROOT / "AGENTS.md"))
    results.append(check_claude_doc(REPO_ROOT / "CLAUDE.md"))

    return results
