#!/usr/bin/env bash
set -euo pipefail

export MSYS2_ARG_CONV_EXCL='*'

BASE_REF="${BASE_REF:-main}"
RUN_QUICK_GATE="${RUN_QUICK_GATE:-0}"
RUN_BUILD="${RUN_BUILD:-0}"
RUN_CXX_TESTS="${RUN_CXX_TESTS:-0}"
RUN_RUST_TESTS="${RUN_RUST_TESTS:-0}"

echo "=== QmClient Harness Initialization ==="

echo "=== Required harness files ==="
for file in \
	AGENTS.md \
	CLAUDE.md \
	.ai/harness.md \
	.ai/session-lifecycle.md \
	.ai/ddnet-development.md \
	.ai/verification.md \
	.ai/review.md \
	.ai/reference.md \
	feature_list.json \
	progress.md \
	session-handoff.md; do
	test -f "$file"
	echo "found $file"
done

echo "=== Workflow document check ==="
python qmclient_scripts/gate/check_workflow_docs.py

if [[ "$RUN_QUICK_GATE" == "1" ]]; then
	echo "=== Quick gate ==="
	bash qmclient_scripts/gate/check-gate.sh --mode quick --base-ref "$BASE_REF"
else
	echo "skip quick gate: set RUN_QUICK_GATE=1 to run it"
fi

if [[ "$RUN_BUILD" == "1" ]]; then
	echo "=== Build game-client ==="
	qmclient_scripts/cmake-windows.cmd --build cmake-build-release --target game-client -j 10
else
	echo "skip build: set RUN_BUILD=1 to build game-client"
fi

if [[ "$RUN_CXX_TESTS" == "1" ]]; then
	echo "=== C++ tests ==="
	qmclient_scripts/cmake-windows.cmd --build cmake-build-release --target run_cxx_tests -j 10
else
	echo "skip C++ tests: set RUN_CXX_TESTS=1 to run them"
fi

if [[ "$RUN_RUST_TESTS" == "1" ]]; then
	echo "=== Rust tests ==="
	qmclient_scripts/cmake-windows.cmd --build cmake-build-release --target run_rust_tests -j 10
else
	echo "skip Rust tests: set RUN_RUST_TESTS=1 to run them"
fi

echo "=== Initialization complete ==="
