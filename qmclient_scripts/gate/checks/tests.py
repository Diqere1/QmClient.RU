#!/usr/bin/env python3
"""CMake 测试目标执行。"""

from __future__ import annotations

from pathlib import Path

from lib import runner
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def _run_cmake_target(
    target: str,
    build_dir: str,
    title: str,
) -> tuple[int, str]:
    cmake_script = REPO_ROOT / "qmclient_scripts" / "cmake-windows.cmd"
    if runner.resolve_cmake_command() == "cmd.exe" and cmake_script.exists():
        cmd = [
            "cmd.exe",
            "/c",
            runner.to_windows_path(str(cmake_script)),
            "--build",
            build_dir,
            "--target",
            target,
            "-j",
            "10",
        ]
    else:
        cmd = [
            "cmake",
            "--build",
            build_dir,
            "--target",
            target,
            "-j",
            "10",
        ]
    return runner.run(cmd, title=title, check=False)


def _run_testrunner_binary(
    build_dir: str,
    title: str,
) -> tuple[int, str]:
    testrunner = REPO_ROOT / build_dir / "testrunner.exe"
    if not testrunner.exists():
        testrunner = REPO_ROOT / build_dir / "testrunner"
    if not testrunner.exists():
        return (1, f"未找到测试二进制: {REPO_ROOT / build_dir / 'testrunner(.exe)'}")
    return runner.run([str(testrunner)], title=title, check=False)


def run(
    results: ResultCollector,
    included: list[str],
    dry_run: bool = False,
    build_dir: str = "cmake-build-release",
    run_all: bool = False,
    run_cxx: bool = False,
    run_rust: bool = False,
) -> None:
    if dry_run:
        results.add("INFO", "CMake 测试目标", "DryRun，仅展示命令")
        return
    if run_all:
        code, out = _run_cmake_target("run_tests", build_dir, "CMake run_tests")
        if code != 0:
            results.add("FAIL", "CMake run_tests", out)
        else:
            results.add("PASS", "CMake run_tests", "通过")
        return
    if run_cxx:
        code, out = _run_cmake_target("testrunner", build_dir, "CMake testrunner build")
        if code == 0:
            code, out = _run_testrunner_binary(build_dir, "C++ tests")
        if code != 0:
            results.add("FAIL", "C++ tests", out)
        else:
            results.add("PASS", "C++ tests", "通过")
    if run_rust:
        code, out = _run_cmake_target(
            "run_rust_tests", build_dir, "CMake run_rust_tests"
        )
        if code != 0:
            results.add("FAIL", "CMake run_rust_tests", out)
        else:
            results.add("PASS", "CMake run_rust_tests", "通过")
