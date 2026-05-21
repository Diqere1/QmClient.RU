#!/usr/bin/env python3
"""治理文档一致性检查。"""

from __future__ import annotations

from pathlib import Path

from lib import runner
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def run(results: ResultCollector, included: list[str], dry_run: bool = False) -> None:
    if dry_run:
        results.add("INFO", "治理文档一致性检查", "DryRun，仅展示命令")
        return
    code, out = runner.run_python(
        str(REPO_ROOT / "qmclient_scripts" / "gate" / "check_docs.py"),
        title="治理文档一致性检查",
        fail_level="FAIL",
        check=False,
    )
    if code != 0:
        results.add("FAIL", "治理文档一致性检查", out)
    else:
        results.add("PASS", "治理文档一致性检查", "执行通过")
