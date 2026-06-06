from __future__ import annotations

from dataclasses import dataclass

from lib.agents_sync import sync_files


@dataclass
class CheckResult:
    ok: bool
    title: str
    detail: str
    blocking: bool = True


def run_checks(prefer: str = "auto") -> list[CheckResult]:
    results: list[CheckResult] = []

    sync_result = sync_files(prefer)
    results.append(
        CheckResult(sync_result.ok, "AGENTS / CLAUDE 镜像同步", sync_result.detail)
    )
    return results
