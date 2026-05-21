#!/usr/bin/env python3
"""CMake 测试目标执行。"""

from __future__ import annotations

from pathlib import Path

from lib import runner
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def run(
    results: ResultCollector,
    included: list[str],
    dry_run: bool = False,
    build_dir: str = "build-ninja",
    run_all: bool = False,
    run_cxx: bool = False,
    run_rust: bool = False,
) -> None:
    if dry_run:
        results.add("INFO", "CMake 测试目标", "DryRun，仅展示命令")
        return
    cmake_script = REPO_ROOT / "qmclient_scripts" / "cmake-windows.cmd"
    if run_all:
        code, out = runner.run(
            [
                "bash",
                str(cmake_script),
                "--build",
                build_dir,
                "--target",
                "run_tests",
                "-j",
                "10",
            ],
            title="CMake run_tests",
            check=False,
        )
        if code != 0:
            results.add("FAIL", "CMake run_tests", out)
        else:
            results.add("PASS", "CMake run_tests", "通过")
        return
    if run_cxx:
        code, out = runner.run(
            [
                "bash",
                str(cmake_script),
                "--build",
                build_dir,
                "--target",
                "run_cxx_tests",
                "-j",
                "10",
            ],
            title="CMake run_cxx_tests",
            check=False,
        )
        if code != 0:
            results.add("FAIL", "CMake run_cxx_tests", out)
        else:
            results.add("PASS", "CMake run_cxx_tests", "通过")
    if run_rust:
        code, out = runner.run(
            [
                "bash",
                str(cmake_script),
                "--build",
                build_dir,
                "--target",
                "run_rust_tests",
                "-j",
                "10",
            ],
            title="CMake run_rust_tests",
            check=False,
        )
        if code != 0:
            results.add("FAIL", "CMake run_rust_tests", out)
        else:
            results.add("PASS", "CMake run_rust_tests", "通过")
