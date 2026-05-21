#!/usr/bin/env python3
"""严格构建与静态分析：Debug CRT 严格编译 + MSVC /analyze + clang-tidy。"""

from __future__ import annotations

import os
import subprocess
import re
import shutil
from pathlib import Path

from lib import runner, scope
from lib.report import ResultCollector

REPO_ROOT = Path(__file__).resolve().parents[3]


def _is_strict_warning_line(line: str) -> bool:
    if not line.strip():
        return False
    patterns = [
        r"^CMake\s+Warning",
        r"(^|: |\s)warning\s+C[0-9]+([^\w_]|$)",
        r":\s*warning([^\w_]|$)",
        r"(^|[^\w_])warning:([^\w_]|$)",
        r"^(cl|clang-cl|link|LINK)\s*:\s*warning([^\w_]|$)",
        r"^[0-9]+\s+warnings\s+generated\.$",
        r"^Suppressed\s[0-9]+\swarnings",
    ]
    for p in patterns:
        if re.search(p, line):
            return True
    return False


def _is_ignorable_tool_warning(line: str) -> bool:
    return line.startswith("CMake Warning (dev)")


def _repo_command(
    results: ResultCollector,
    title: str,
    cmd: list[str],
    fail_on_warnings: bool = False,
    cwd: Path | None = None,
) -> None:
    runner.print_section(title)
    print(f"命令: {' '.join(cmd)}")
    proc = subprocess.run(
        cmd, capture_output=True, text=True, cwd=str(cwd) if cwd else None
    )
    lines = [
        line
        for line in (proc.stdout + proc.stderr).splitlines()
        if not line.startswith("注意: 包含文件:")
    ]
    output = "\n".join(lines)
    if output:
        print(output)

    if proc.returncode != 0:
        summary = ""
        if title.startswith("clang-tidy 严格检查:"):
            unused = [
                line
                for line in lines
                if "clang-diagnostic-unused-command-line-argument" in line
            ][:6]
            if unused:
                summary = (
                    "原因：clang-tidy 看到的是 MSVC compile_commands 参数，其中一部分 clang 自身不会消费。"
                    "这更像工具链调用问题，不一定是源码诊断。相关输出：\n"
                    + "\n".join(unused)
                )
            else:
                relevant = [
                    line
                    for line in lines
                    if re.search(
                        r"^error:|^[0-9]+ warnings and [0-9]+ errors generated\.$"
                        r"|^Error while processing |^Found compiler error\(s\)\.$",
                        line,
                    )
                ][:8]
                if relevant:
                    summary = (
                        "原因：clang-tidy 找到了真实诊断或编译错误。相关输出：\n"
                        + "\n".join(relevant)
                    )
        msg = f"{title} 失败，退出码 {proc.returncode}"
        if summary:
            msg += "\n" + summary
        results.add("FAIL", title, msg)
        return

    if fail_on_warnings:
        warns = [
            line
            for line in lines
            if _is_strict_warning_line(line) and not _is_ignorable_tool_warning(line)
        ]
        if warns:
            results.add(
                "FAIL", title, f"{title} 输出了 warning 文本:\n" + "\n".join(warns)
            )
            return

    results.add("PASS", title, "执行通过")


def _configure_and_build(
    results: ResultCollector,
    title: str,
    build_dir: str,
    fail_on_warnings: bool,
    cmd: list[str],
    skip_build: bool,
) -> None:
    _repo_command(results, f"{title} 配置", cmd, fail_on_warnings=fail_on_warnings)
    if skip_build:
        results.add("WARN", f"{title} 构建", "已显式传入 --skip-build，仅执行配置阶段")
        return
    cmake_script = REPO_ROOT / "qmclient_scripts" / "cmake-windows.cmd"
    if shutil.which("cmd.exe") and cmake_script.exists():
        build_cmd = [
            "cmd.exe",
            "/c",
            str(runner.to_windows_path(str(cmake_script))),
            "--build",
            build_dir,
            "--target",
            "game-client",
            "-j",
            "10",
        ]
    else:
        build_cmd = [
            "cmake",
            "--build",
            build_dir,
            "--target",
            "game-client",
            "-j",
            "10",
        ]
    _repo_command(
        results, f"{title} 构建", build_cmd, fail_on_warnings=fail_on_warnings
    )


def run(
    results: ResultCollector,
    included: list[str],
    dry_run: bool = False,
    base_ref: str = "main",
) -> None:
    del base_ref  # included 已由 gate 层收敛，无需再次收集
    if dry_run:
        results.add("INFO", "严格构建与静态分析入口", "DryRun，仅展示命令")
        return

    cmake_script = REPO_ROOT / "qmclient_scripts" / "cmake-windows.cmd"
    if not cmake_script.exists():
        results.add("FAIL", "入口前置检查", f"缺少 CMake 包装脚本: {cmake_script}")
        return
    results.add("PASS", "入口前置检查", "已找到 qmclient_scripts/cmake-windows.cmd")

    cm_cmd = runner.resolve_cmake_command()
    if not cm_cmd:
        results.add("FAIL", "入口前置检查", "PATH 中未找到 cmake")
        return

    os.chdir(REPO_ROOT)

    # 判断是否需要 /analyze
    run_analyze = False
    analyze_source_files: list[str] = []
    if cm_cmd == "cmd.exe":
        analyze_source_files = [
            f
            for f in included
            if re.search(r"\.(c|cc|cpp)$", f) and f.startswith("src/")
        ]
        if analyze_source_files:
            run_analyze = True
            results.add(
                "INFO",
                "MSVC /analyze 触发范围",
                f"{len(analyze_source_files)} 个改动源文件触发 /analyze；"
                "实际分析范围由 game-client 构建目标和 Ninja 增量状态决定",
            )
        else:
            results.add(
                "WARN",
                "MSVC /analyze 范围",
                "当前改动只有头文件或无可分析编译单元，/analyze 阶段已跳过",
            )

    # Debug CRT 配置与构建
    debug_build_dir = "build-debug"
    if cm_cmd == "cmd.exe":
        win_script = runner.to_windows_path(str(cmake_script))
        configure_cmd = [
            "cmd.exe",
            "/c",
            win_script,
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            debug_build_dir,
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DQM_STRICT_WARNINGS=ON",
        ]
    else:
        configure_cmd = [
            "cmake",
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            debug_build_dir,
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DQM_STRICT_WARNINGS=ON",
        ]
    _configure_and_build(results, "Debug CRT", debug_build_dir, 1, configure_cmd, False)

    # MSVC /analyze
    if cm_cmd != "cmd.exe":
        results.add(
            "WARN",
            "MSVC /analyze",
            "当前不是 Windows/MSVC 环境，bash 版本不会伪造 /analyze；"
            "该阶段已按设计降级跳过",
        )
    elif run_analyze:
        analyze_build_dir = "build-analyze"
        if cm_cmd == "cmd.exe":
            analyze_cmd = [
                "cmd.exe",
                "/c",
                runner.to_windows_path(str(cmake_script)),
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                analyze_build_dir,
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DQM_MSVC_ANALYZE=ON",
            ]
        else:
            analyze_cmd = [
                "cmake",
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                analyze_build_dir,
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DQM_MSVC_ANALYZE=ON",
            ]
        _configure_and_build(
            results, "MSVC /analyze", analyze_build_dir, 1, analyze_cmd, False
        )

    # clang-tidy
    if not included:
        results.add(
            "WARN",
            "clang-tidy",
            "未收集到首方 src 差异文件，跳过 tidy 阶段",
        )
        return

    if not shutil.which("clang-tidy"):
        results.add(
            "FAIL",
            "clang-tidy 前置检查",
            "PATH 中未找到 clang-tidy，无法执行严格 tidy 检查。",
        )
        return
    results.add("PASS", "clang-tidy 前置检查", "已找到 clang-tidy")

    compile_commands = Path(debug_build_dir) / "compile_commands.json"
    if not compile_commands.exists():
        # 重新导出
        if cm_cmd == "cmd.exe":
            reconf_cmd = [
                "cmd.exe",
                "/c",
                runner.to_windows_path(str(cmake_script)),
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                debug_build_dir,
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ]
        else:
            reconf_cmd = [
                "cmake",
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                debug_build_dir,
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ]
        _repo_command(
            results,
            "Debug CRT 重新导出 compile_commands",
            reconf_cmd,
            fail_on_warnings=1,
        )

    if not compile_commands.exists():
        results.add(
            "FAIL",
            "compile_commands 检查",
            f"重新配置后仍未生成 compile_commands.json: {compile_commands}",
        )
        return
    results.add("PASS", "compile_commands 检查", "compile_commands.json 已就绪")

    tidy_files = []
    for f in included:
        if not f:
            continue
        norm = scope.normalize_path(f)
        if not re.search(r"\.(c|cc|cpp)$", norm):
            continue
        if (REPO_ROOT / norm).exists():
            tidy_files.append(str(REPO_ROOT / norm))
        elif os.path.isabs(norm) and Path(norm).exists():
            tidy_files.append(norm)

    tidy_files = sorted(set(tidy_files))
    if not tidy_files:
        results.add(
            "WARN", "clang-tidy", "当前没有可检查的 C/C++ 文件，tidy 阶段已跳过"
        )
        return

    results.add(
        "INFO", "clang-tidy 范围", f"将对 {len(tidy_files)} 个源文件执行严格 tidy 检查"
    )
    tidy_checks = (
        "-*,bugprone-assignment-in-if-condition,bugprone-dangling-handle,"
        "bugprone-inaccurate-erase,bugprone-misplaced-widening-cast,"
        "bugprone-stringview-nullptr,bugprone-suspicious-enum-usage,"
        "bugprone-unchecked-optional-access,bugprone-use-after-move,"
        "clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-nullability.*,"
        "modernize-use-override,performance-for-range-copy,performance-unnecessary-copy-initialization"
    )
    for file in tidy_files:
        _repo_command(
            results,
            f"clang-tidy 严格检查: {file}",
            [
                "clang-tidy",
                file,
                f"-p={debug_build_dir}",
                f"--checks={tidy_checks}",
                "--extra-arg=-Qunused-arguments",
                "--quiet",
            ],
            fail_on_warnings=1,
        )
