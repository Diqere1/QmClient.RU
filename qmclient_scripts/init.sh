#!/usr/bin/env bash
set -euo pipefail

export MSYS2_ARG_CONV_EXCL='*'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

BASE_REF="${BASE_REF:-main}"
RUN_QUICK_GATE="${RUN_QUICK_GATE:-0}"
RUN_BUILD="${RUN_BUILD:-0}"
RUN_CXX_TESTS="${RUN_CXX_TESTS:-0}"
RUN_RUST_TESTS="${RUN_RUST_TESTS:-0}"

pick_python() {
	local candidates=()
	if [[ -n "${PYTHON:-}" ]]; then
		candidates+=("$PYTHON")
	fi
	if [[ -n "${PYTHON3:-}" ]]; then
		candidates+=("$PYTHON3")
	fi
	if [[ -n "${SCOOP:-}" ]]; then
		candidates+=("$SCOOP/apps/python/current/python.exe")
	fi
	candidates+=(
		"${USERPROFILE:-}/scoop/apps/python/current/python.exe"
		"/d/Scoop/apps/python/current/python.exe"
		"python"
		"python3"
	)

	for candidate in "${candidates[@]}"; do
		[[ -n "$candidate" ]] || continue
		if [[ "$candidate" == *WindowsApps* ]]; then
			continue
		fi
		if "$candidate" -c "import sys; print(sys.executable)" >/dev/null 2>&1; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	return 1
}

BOOTSTRAP_PYTHON="$(pick_python)"
if [[ -z "$BOOTSTRAP_PYTHON" ]]; then
	echo "failed to locate a working Python interpreter"
	exit 1
fi

PYTHON_BIN="$("$BOOTSTRAP_PYTHON" - <<'PY'
import sys
from pathlib import Path

repo_root = Path.cwd()
sys.path.insert(0, str(repo_root / "qmclient_scripts" / "gate"))

from lib import runner  # noqa: E402

print(runner.resolve_python_cmd() or sys.executable)
PY
)"
PYTHON_BIN="${PYTHON_BIN//$'\r'/}"
if [[ -z "$PYTHON_BIN" ]]; then
	PYTHON_BIN="$BOOTSTRAP_PYTHON"
fi

echo "=== QmClient Harness Initialization ==="
echo "using python: $PYTHON_BIN"

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
	.ai/feature_list.json \
	.ai/progress.md \
	.ai/session-handoff.md; do
	test -f "$file"
	echo "found $file"
done

echo "=== Workflow document check ==="
"$PYTHON_BIN" qmclient_scripts/gate/check_docs.py

if [[ "$RUN_QUICK_GATE" == "1" ]]; then
	echo "=== Quick gate ==="
	"$PYTHON_BIN" qmclient_scripts/gate/check_gate.py --mode quick --base-ref "$BASE_REF"
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
	if [[ -f "cmake-build-release/testrunner.exe" ]]; then
		cmake-build-release/testrunner.exe
	else
		qmclient_scripts/cmake-windows.cmd --build cmake-build-release --target testrunner -j 10
		cmake-build-release/testrunner.exe
	fi
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
