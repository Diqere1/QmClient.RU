#!/usr/bin/env python3
"""
QmClient 治理文档检查入口。

单一事实源：
1. 先同步 AGENTS.md / CLAUDE.md。
2. 再检查 harness 文档、状态文件、脚本入口与 workflow 语义是否一致。
"""

from __future__ import annotations

import argparse
import sys

from lib.agents_sync import sync_files
from lib.docs_harness import run_checks


def main() -> int:
    parser = argparse.ArgumentParser(description="QmClient 治理文档检查入口")
    parser.add_argument(
        "--prefer",
        choices=["auto", "agents", "claude"],
        default="auto",
        help="冲突或状态不明时优先采用哪一侧；默认自动判断",
    )
    parser.add_argument(
        "--sync-only",
        action="store_true",
        help="只同步 AGENTS.md / CLAUDE.md，不执行后续治理文档检查",
    )
    args = parser.parse_args()

    if args.sync_only:
        sync_result = sync_files(args.prefer)
        results = [sync_result]
    else:
        results = run_checks(args.prefer)
    failed = [result for result in results if not result.ok and result.blocking]

    print("治理文档一致性检查")
    for result in results:
        prefix = "通过" if result.ok else "失败"
        print(f"- {prefix}：{result.title} - {result.detail}")

    if failed:
        print("\n结论：存在治理文档漂移或断链，需要修复。")
        return 1

    print("\n结论：治理文档入口一致，未发现断链。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
