#!/usr/bin/env python3
"""全量 .clang-tidy 附加检查（高噪音，默认 WARN）。"""

from __future__ import annotations

import shutil
from pathlib import Path

from lib import runner
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def run(results: ResultCollector, included: list[str], dry_run: bool = False) -> None:
    if not shutil.which("clang-tidy"):
        results.add(
            "WARN", "全量 .clang-tidy 附加检查", "PATH 中未找到 clang-tidy，已跳过"
        )
        return
    cc = REPO_ROOT / "cmake-build-debug" / "compile_commands.json"
    if not cc.exists():
        results.add(
            "WARN",
            "全量 .clang-tidy 附加检查",
            "缺少 cmake-build-debug/compile_commands.json，请先跑 strict-debug-check 或 default/full 构建层",
        )
        return
    for file in included:
        code, out = runner.run(
            [
                "clang-tidy",
                file,
                "-p=cmake-build-debug",
                f"--config-file={REPO_ROOT / '.clang-tidy'}",
                "--extra-arg=-Qunused-arguments",
                "--quiet",
            ],
            title=f"全量 .clang-tidy 附加检查: {file}",
            check=False,
        )
        if code != 0:
            results.add("WARN", f"全量 .clang-tidy 附加检查: {file}", out)
        else:
            results.add("PASS", f"全量 .clang-tidy 附加检查: {file}", "通过")
