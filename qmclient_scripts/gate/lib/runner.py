#!/usr/bin/env python3
"""子进程调用、Python 命令查找、Windows 路径转换。"""

from __future__ import annotations

import os
import platform
import shutil
import re
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]


def is_windows_env() -> bool:
    return (
        platform.system().startswith("MINGW")
        or platform.system().startswith("MSYS")
        or platform.system().startswith("CYGWIN")
    )


def is_windows_host() -> bool:
    if is_windows_env():
        return True
    return shutil.which("cmd.exe") is not None


def to_windows_path(path: str) -> str:
    # WSL /mnt/X/...
    m = re.match(r"^/mnt/([A-Za-z])/(.*)$", path)
    if m:
        return f"{m.group(1).upper()}:\\\\{m.group(2).replace('/', '\\')}"
    # /X/...
    m = re.match(r"^/([A-Za-z])/(.*)$", path)
    if m:
        return f"{m.group(1).upper()}:\\\\{m.group(2).replace('/', '\\')}"
    if shutil.which("wslpath"):
        try:
            return subprocess.check_output(["wslpath", "-w", path], text=True).strip()
        except subprocess.CalledProcessError:
            pass
    if shutil.which("cygpath"):
        try:
            return subprocess.check_output(["cygpath", "-w", path], text=True).strip()
        except subprocess.CalledProcessError:
            pass
    return path


def _uses_windows_paths(py_cmd: str) -> bool:
    return bool(re.search(r"py\.exe|python\.exe|^[A-Za-z]:[/\\]", py_cmd))


def resolve_python_cmd() -> str | None:
    for name in ("py.exe", "py", "python", "python.exe"):
        cmd = shutil.which(name)
        if cmd:
            return cmd
    if is_windows_host() and shutil.which("where.exe"):
        for name in ("python", "py"):
            try:
                out = subprocess.check_output(
                    ["where.exe", name], text=True, stderr=subprocess.DEVNULL
                )
                line = out.strip().splitlines()[0]
                if line:
                    return line
            except (subprocess.CalledProcessError, IndexError):
                pass
    return None


def resolve_cmake_command() -> str | None:
    if (
        is_windows_host()
        and (REPO_ROOT / "qmclient_scripts" / "cmake-windows.cmd").exists()
    ):
        return "cmd.exe"
    cmake = shutil.which("cmake")
    return cmake if cmake else None


def run_python(
    *args: str,
    title: str = "",
    fail_level: str = "FAIL",
    dry_run: bool = False,
    check: bool = True,
) -> tuple[int, str]:
    py = resolve_python_cmd()
    if not py:
        return (1, "未找到可用的 Python")
    cmd = [py]
    if re.search(r"py\.exe|py$", py, re.IGNORECASE) and not py.lower().endswith(
        "python.exe"
    ):
        cmd.append("-3")
    for arg in args:
        if _uses_windows_paths(py) and (
            arg.startswith("/") or re.match(r"^/mnt/[A-Za-z]/", arg)
        ):
            cmd.append(to_windows_path(arg))
        else:
            cmd.append(arg)
    return run(cmd, title=title, fail_level=fail_level, dry_run=dry_run, check=check)


def run(
    cmd: list[str],
    title: str = "",
    fail_level: str = "FAIL",
    dry_run: bool = False,
    check: bool = True,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    stdin=None,
) -> tuple[int, str]:
    if title:
        print(f"\n==> {title}")
    print(f"命令: {' '.join(cmd)}")
    if dry_run:
        return (0, "DryRun，仅展示命令")

    merged_env = {**os.environ, **env} if env else None
    proc = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        cwd=str(cwd) if cwd else None,
        env=merged_env,
        stdin=stdin,
    )
    output = (proc.stdout or "") + (proc.stderr or "")
    # 过滤 showincludes 行
    lines = [
        line for line in output.splitlines() if not line.startswith("注意: 包含文件:")
    ]
    filtered = "\n".join(lines)
    if filtered:
        print(filtered)

    if proc.returncode != 0:
        return (
            proc.returncode,
            f"{title or '命令'} 失败，退出码 {proc.returncode}\n{filtered}",
        )

    if check:
        # 检查 warning 文本（只对需要 fail_on_warnings 的调用方生效）
        pass

    return (0, filtered)


# 保持与旧脚本的 print_section 兼容
def print_section(name: str) -> None:
    print(f"\n==> {name}")


def print_result(level: str, message: str) -> None:
    print(f"[{level}] {message}")
