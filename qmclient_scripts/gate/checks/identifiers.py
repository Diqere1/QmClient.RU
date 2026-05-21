#!/usr/bin/env python3
"""标识符命名检查。"""

from __future__ import annotations

import os
import re
import subprocess
import tempfile
from pathlib import Path

from lib import runner
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def run(results: ResultCollector, included: list[str], dry_run: bool = False) -> None:
    source_files = [
        str(REPO_ROOT / f) for f in included if re.search(r"\.(c|cc|cpp)$", f)
    ]
    if not source_files:
        results.add(
            "WARN",
            "标识符命名检查",
            "未找到改动范围内可供 extract_identifiers.py 分析的首方源文件",
        )
        return
    if dry_run:
        results.add("INFO", "标识符命名检查", "DryRun")
        return
    runner.print_section("标识符命名检查")
    with tempfile.NamedTemporaryFile(mode="w+", suffix=".txt", delete=False) as tmp:
        tmp_path = tmp.name
    try:
        py = runner.resolve_python_cmd()
        if not py:
            results.add("FAIL", "标识符命名检查", "未找到 Python")
            return
        with open(tmp_path, "w", encoding="utf-8") as f:
            proc = subprocess.run(
                [
                    py,
                    str(REPO_ROOT / "scripts" / "extract_identifiers.py"),
                    *source_files,
                ],
                stdout=f,
                stderr=subprocess.PIPE,
                text=True,
            )
        if proc.returncode != 0:
            results.add("FAIL", "标识符命名检查", proc.stderr)
            return
        code2, out = runner.run(
            [py, str(REPO_ROOT / "scripts" / "check_identifiers.py")],
            title="检查标识符",
            check=False,
            stdin=open(tmp_path, "r", encoding="utf-8"),
        )
        if code2 != 0:
            results.add("FAIL", "标识符命名检查", out)
            return
        results.add("PASS", "标识符命名检查", "命名风格检查通过")
    finally:
        os.unlink(tmp_path)
