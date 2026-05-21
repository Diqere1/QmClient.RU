#!/usr/bin/env bash
set -euo pipefail
export MSYS2_ARG_CONV_EXCL='*'

# -----------------------------------------------------------------------------
# QmClient 仓库级 bash 门禁总入口
#
# 设计目标：
# 1. 以 bash 作为仓库级门禁规范入口，不再让 ps1 充当事实源。
# 2. 收口源码卫生、严格调试检查、测试、allowlist 和 JSON 报告。
# 3. Windows 下继续通过 qmclient_scripts/cmake-windows.cmd 调用 CMake。
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

BUILD_DIR="build-ninja"
BASE_REF="main"
MODE="default"
ANALYZE_ALL=0
SKIP_PREFLIGHT=0
SKIP_CONFIG_CHECKS=0
SKIP_WORKFLOW_DOCS=0
SKIP_HEADER_CHECKS=0
SKIP_STYLE_CHECK=0
SKIP_STRICT_DEBUG=0
SKIP_CXX_TESTS=0
RUN_RUST_TESTS=0
RUN_ALL_TESTS=0
INCLUDE_IDENTIFIER_CHECK=0
INCLUDE_UNUSED_HEADER_CHECK=0
ENABLE_CLANG_FORMAT_CHECK=0
ENABLE_FULL_CLANG_TIDY_WARN=0
STRICT_ENVIRONMENT=0
DRY_RUN=0
EXPLAIN_SCOPE=0
REPORT_JSON_PATH=""
SCOPE_REPORT_PATH=""
BRANCH_SCOPE_ONLY=0

MODE_TARGET=""
MODE_EXPECTATION=""
MODE_BLOCKING_RULE=""
MODE_RUN_STRICT_DEBUG=0
MODE_RUN_CXX_TESTS=0
MODE_RUN_RUST_TESTS=0
MODE_RUN_IDENTIFIER_CHECK=0
MODE_RUN_UNUSED_HEADER_CHECK=0
MODE_RUN_CLANG_FORMAT_CHECK=0
MODE_RUN_FULL_CLANG_TIDY_WARN=0

RESULT_PASS=0
RESULT_WARN=0
RESULT_FAIL=0
RESULT_ITEMS=()

BASE_REF_AVAILABLE=1
BASE_REF_FAILURE_REASON=""
SCOPE_DIAGNOSTICS_READY=0
SCOPE_BRANCH_FILES=()
SCOPE_UNSTAGED_FILES=()
SCOPE_STAGED_FILES=()
SCOPE_UNTRACKED_FILES=()
SCOPE_INCLUDED_FILES=()
SCOPE_EXCLUDED_PATHS=()
SCOPE_EXCLUDED_REASONS=()

BASELINE_ALLOWLIST_TITLES=()
BASELINE_ALLOWLIST_HASHES=()
BASELINE_ALLOWLIST_REASONS=()

json_escape() {
	local value="${1:-}"
	value="${value//\\/\\\\}"
	value="${value//\"/\\\"}"
	value="${value//$'\n'/\\n}"
	value="${value//$'\r'/\\r}"
	value="${value//$'\t'/\\t}"
	printf '%s' "${value}"
}

is_windows_bash() {
	case "$(uname -s)" in
		MINGW*|MSYS*|CYGWIN*) return 0 ;;
		*) return 1 ;;
	esac
}

to_windows_path() {
	local path="$1"
	if [[ "${path}" =~ ^/mnt/([A-Za-z])/(.*)$ ]]; then
		local drive="${BASH_REMATCH[1]}"
		local rest="${BASH_REMATCH[2]//\//\\}"
		printf '%s:\\%s' "${drive^^}" "${rest}"
		return 0
	fi
	if [[ "${path}" =~ ^/([A-Za-z])/(.*)$ ]]; then
		local drive="${BASH_REMATCH[1]}"
		local rest="${BASH_REMATCH[2]//\//\\}"
		printf '%s:\\%s' "${drive^^}" "${rest}"
		return 0
	fi
	if command -v wslpath >/dev/null 2>&1; then
		wslpath -w "${path}"
		return 0
	fi
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -w "${path}"
		return 0
	fi
	printf '%s' "${path}"
}

python_uses_windows_paths() {
	local py_cmd="$1"
	case "${py_cmd}" in
		py|py.exe|python.exe|*/py|*/py.exe|*\\py|*\\py.exe|*/python.exe|*\\python.exe|[A-Za-z]:/*|[A-Za-z]:\\*)
			return 0
			;;
	esac
	return 1
}

write_section() {
	printf '\n==> %s\n' "$1"
}

write_result_line() {
	printf '[%s] %s\n' "$1" "$2"
}

category_info() {
	local title="$1"
	local detail="$2"
	local text="${title}"$'\n'"${detail}"
	if [[ "${text}" =~ 环境前置检查|Python\ 前置检查|PATH\ 中未找到|未找到可用的\ Python|WindowsApps|缺少必需路径|缺少\ CMake\ 包装脚本|ddnet-libs|当前\ worktree\ 的\ DDNet\.exe\ 仍在运行|差异基线不可用|Found\ no\ clang-format|WinError\ 206|not\ a\ directory ]]; then
		printf 'environment\t环境/工具'
		return
	fi
	if [[ "${title}" =~ 配置变量使用检查|工作流文档一致性检查|头文件\ guard\ 检查|标准头文件检查|代码格式干跑检查|标识符命名检查|未使用头文件检查|clang-format\ 附加检查 ]] || [[ "${detail}" =~ clang-format-violations|未使用配置项|头文件保护宏不正确|缺少头文件保护宏 ]]; then
		printf 'baseline_debt\t仓库基线债务'
		return
	fi
	if [[ "${title}" =~ 严格构建与静态分析入口|Debug\ CRT|MSVC\ /analyze|AddressSanitizer|clang-tidy|CMake\ run_|测试目标|JSON\ 报告 ]] || [[ "${detail}" =~ build-debug|build-analyze|run_cxx_tests|run_rust_tests|run_tests|compile_commands|仓库级检查存在失败项 ]]; then
		printf 'active_blocker\t当前改动/构建阻断'
		return
	fi
	printf 'general\t一般项'
}

normalized_detail_hash() {
	local detail="${1:-}"
	printf '%s' "${detail}" | python_run -c '
import hashlib
import sys

text = sys.stdin.buffer.read().decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "\n")
marker = "\n--- 原始尾部输出 ---\n"
idx = text.find(marker)
if idx >= 0:
    text = text[:idx]
lines = sorted({line.strip() for line in text.split("\n") if line.strip()})
normalized = "\n".join(lines).strip()
print(hashlib.sha256(normalized.encode("utf-8")).hexdigest())
'
}

load_baseline_allowlist() {
	local path="${REPO_ROOT}/qmclient_scripts/gate/baseline_debt_allowlist.json"
	[[ -f "${path}" ]] || return 0
	while IFS=$'\t' read -r title detail_hash reason; do
		[[ -n "${title}" ]] || continue
		BASELINE_ALLOWLIST_TITLES+=("${title}")
		BASELINE_ALLOWLIST_HASHES+=("${detail_hash}")
		BASELINE_ALLOWLIST_REASONS+=("${reason:-known_baseline_debt}")
	done < <(
		python_run - "${path}" <<'PY'
import json
import sys
data = json.load(open(sys.argv[1], encoding="utf-8-sig"))
for entry in data.get("entries", []):
    title = str(entry.get("title", ""))
    detail_hash = str(entry.get("detail_hash", ""))
    reason = str(entry.get("reason", "known_baseline_debt"))
    if title and detail_hash:
        print(f"{title}\t{detail_hash}\t{reason}")
PY
	)
}

match_allowlist() {
	local title="$1"
	local detail_hash="$2"
	local i
	for i in "${!BASELINE_ALLOWLIST_TITLES[@]}"; do
		if [[ "${BASELINE_ALLOWLIST_TITLES[$i]}" == "${title}" && "${BASELINE_ALLOWLIST_HASHES[$i]}" == "${detail_hash}" ]]; then
			printf '%s' "${BASELINE_ALLOWLIST_REASONS[$i]}"
			return 0
		fi
	done
	return 1
}

add_result() {
	local level="$1"
	local title="$2"
	local detail="$3"
	local category_id category_label category allowlist_reason detail_hash stored_level stored_detail
	category="$(category_info "${title}" "${detail}")"
	category_id="${category%%$'\t'*}"
	category_label="${category#*$'\t'}"
	allowlist_reason=""
	detail_hash=""
	stored_level="${level}"
	stored_detail="${detail}"

	if [[ "${category_id}" == "baseline_debt" ]]; then
		detail_hash="$(normalized_detail_hash "${detail}")"
		if [[ "${level}" == "FAIL" ]] && allowlist_reason="$(match_allowlist "${title}" "${detail_hash}")"; then
			stored_level="WARN"
			stored_detail="已按 baseline allowlist 降级为 WARN，reason=${allowlist_reason}, detail_hash=${detail_hash}"$'\n'"${detail}"
		fi
	fi

	case "${stored_level}" in
		PASS) RESULT_PASS=$((RESULT_PASS + 1)) ;;
		WARN) RESULT_WARN=$((RESULT_WARN + 1)) ;;
		FAIL) RESULT_FAIL=$((RESULT_FAIL + 1)) ;;
	esac

	RESULT_ITEMS+=("${stored_level}"$'\t'"${level}"$'\t'"${title}"$'\t'"${stored_detail}"$'\t'"${category_id}"$'\t'"${category_label}"$'\t'"${allowlist_reason}"$'\t'"${detail_hash}")
}

get_python_cmd() {
	if command -v py.exe >/dev/null 2>&1; then
		printf 'py.exe'
		return 0
	fi
	if command -v py >/dev/null 2>&1; then
		printf 'py'
		return 0
	fi
	if command -v python >/dev/null 2>&1; then
		printf 'python'
		return 0
	fi
	if command -v python.exe >/dev/null 2>&1; then
		printf 'python.exe'
		return 0
	fi
	if command -v where.exe >/dev/null 2>&1; then
		local candidate
		candidate="$(where.exe python 2>/dev/null | tr -d '\r' | awk 'NF {print; exit}')"
		if [[ -n "${candidate}" ]]; then
			printf '%s' "${candidate}"
			return 0
		fi
		candidate="$(where.exe py 2>/dev/null | tr -d '\r' | awk 'NF {print; exit}')"
		if [[ -n "${candidate}" ]]; then
			printf '%s' "${candidate}"
			return 0
		fi
	fi
	return 1
}

python_run() {
	local py_cmd
	py_cmd="$(get_python_cmd)"
	local args=()
	local arg
	for arg in "$@"; do
		if python_uses_windows_paths "${py_cmd}" && [[ "${arg}" == /* || "${arg}" =~ ^/mnt/[A-Za-z]/ ]]; then
			args+=("$(to_windows_path "${arg}")")
		else
			args+=("${arg}")
		fi
	done
	case "${py_cmd}" in
		py|py.exe|*/py|*/py.exe|*\\py|*\\py.exe) "${py_cmd}" -3 "${args[@]}" ;;
		*) "${py_cmd}" "${args[@]}" ;;
	esac
}

invoke_repo_command() {
	local title="$1"
	local level_on_fail="$2"
	shift 2
	write_section "${title}"
	printf '命令:'
	printf ' %q' "$@"
	printf '\n'
	if [[ ${DRY_RUN} -eq 1 ]]; then
		add_result "INFO" "${title}" "DryRun，仅展示命令"
		return 0
	fi

	local output exit_code=0
	set +e
	output="$("$@" 2>&1)"
	exit_code=$?
	set -e
	[[ -z "${output}" ]] || printf '%s\n' "${output}"
	if [[ ${exit_code} -ne 0 ]]; then
		local summary="${title} 失败，退出码 ${exit_code}"
		[[ -z "${output}" ]] || summary="${summary}"$'\n'"${output}"
		add_result "${level_on_fail}" "${title}" "${summary}"
		[[ "${level_on_fail}" == "FAIL" ]] && return 1
		return 0
	fi
	add_result "PASS" "${title}" "执行通过"
}

resolve_mode_toggles() {
	case "${MODE}" in
		quick)
			MODE_TARGET="开发期快速自查"
			MODE_EXPECTATION="通常应在数分钟内完成，只扫源码卫生层。"
			MODE_BLOCKING_RULE="只阻断明显的脚本/规范问题，不做真实构建与测试。"
			;;
		default)
			MODE_TARGET="日常提交前严格门"
			MODE_EXPECTATION="需要真实构建、严格静态分析和 C++ 测试。"
			MODE_BLOCKING_RULE="构建、静态分析、测试任一失败都应阻断。"
			;;
		full)
			MODE_TARGET="集中收口 / 准发布门"
			MODE_EXPECTATION="在 default 基础上增加更重的附加检查与 Rust 测试。"
			MODE_BLOCKING_RULE="默认阻断 default 层和 full 的硬失败项；高噪音附加检查先以 WARN 方式试跑。"
			;;
		*)
			printf '未知 mode: %s\n' "${MODE}" >&2
			exit 2
			;;
	esac

	MODE_RUN_STRICT_DEBUG=0
	MODE_RUN_CXX_TESTS=0
	MODE_RUN_RUST_TESTS=0
	MODE_RUN_IDENTIFIER_CHECK=0
	MODE_RUN_UNUSED_HEADER_CHECK=0

	case "${MODE}" in
		default)
			MODE_RUN_STRICT_DEBUG=1
			MODE_RUN_CXX_TESTS=1
			;;
		full)
			MODE_RUN_STRICT_DEBUG=1
			MODE_RUN_CXX_TESTS=1
			MODE_RUN_RUST_TESTS=1
			MODE_RUN_IDENTIFIER_CHECK=1
			;;
	esac

	[[ ${RUN_RUST_TESTS} -eq 1 ]] && MODE_RUN_RUST_TESTS=1
	[[ ${INCLUDE_IDENTIFIER_CHECK} -eq 1 ]] && MODE_RUN_IDENTIFIER_CHECK=1
	[[ ${INCLUDE_UNUSED_HEADER_CHECK} -eq 1 ]] && MODE_RUN_UNUSED_HEADER_CHECK=1
	MODE_RUN_CLANG_FORMAT_CHECK=${ENABLE_CLANG_FORMAT_CHECK}
	MODE_RUN_FULL_CLANG_TIDY_WARN=${ENABLE_FULL_CLANG_TIDY_WARN}
	if [[ ${RUN_ALL_TESTS} -eq 1 ]]; then
		MODE_RUN_CXX_TESTS=0
		MODE_RUN_RUST_TESTS=0
	fi
}

normalize_path() {
	local path="$1"
	path="${path//\\//}"
	printf '%s' "${path}"
}

is_scoped_first_party_file() {
	local path
	path="$(normalize_path "$1")"
	[[ "${path}" =~ ^src/.+\.(c|cc|cpp|h|hpp)$ ]] || return 1
	case "${path}" in
		src/engine/external/*|src/game/generated/*|src/rust-bridge/base/*) return 1 ;;
	esac
	return 0
}

unique_lines() {
	awk 'NF { if(!seen[$0]++) print $0 }'
}

invoke_style_check_batch() {
	local py_cmd="$1"
	local fix_style_path="$2"
	shift 2
	case "${py_cmd}" in
		py|py.exe|*/py|*/py.exe|*\\py|*\\py.exe)
			invoke_repo_command "代码格式干跑检查" FAIL "${py_cmd}" -3 "${fix_style_path}" -n "$@"
			;;
		*)
			invoke_repo_command "代码格式干跑检查" FAIL "${py_cmd}" "${fix_style_path}" -n "$@"
			;;
	esac
}

get_branch_diff_files() {
	local merge_base_output merge_base
	set +e
	merge_base_output="$(git -C "${REPO_ROOT}" merge-base "${BASE_REF}" HEAD 2>&1)"
	local exit_code=$?
	set -e
	merge_base="$(printf '%s\n' "${merge_base_output}" | head -n 1 | tr -d '\r')"
	if [[ ${exit_code} -ne 0 || -z "${merge_base}" ]]; then
		BASE_REF_AVAILABLE=0
		BASE_REF_FAILURE_REASON="$(printf '%s' "${merge_base_output}" | tr '\r' ' ' | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
		[[ -n "${BASE_REF_FAILURE_REASON}" ]] || BASE_REF_FAILURE_REASON="git merge-base 返回空结果"
		return 0
	fi
	BASE_REF_AVAILABLE=1
	BASE_REF_FAILURE_REASON=""
	git -C "${REPO_ROOT}" -c core.safecrlf=false diff --name-only --diff-filter=ACMR "${merge_base}...HEAD" -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>/dev/null | sed 's/\r$//' | awk 'NF' | unique_lines
}

get_worktree_bucket() {
	local bucket="$1"
	case "${bucket}" in
		unstaged)
			git -C "${REPO_ROOT}" -c core.safecrlf=false diff --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>/dev/null
			;;
		staged)
			git -C "${REPO_ROOT}" -c core.safecrlf=false diff --cached --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>/dev/null
			;;
		untracked)
			git -C "${REPO_ROOT}" ls-files --others --exclude-standard -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>/dev/null
			;;
	esac | sed 's/\r$//' | awk 'NF' | unique_lines
}

ensure_scope_diagnostics() {
	[[ ${SCOPE_DIAGNOSTICS_READY} -eq 0 ]] || return 0
	mapfile -t SCOPE_BRANCH_FILES < <(get_branch_diff_files)
	if [[ ${BRANCH_SCOPE_ONLY} -eq 0 ]]; then
		mapfile -t SCOPE_UNSTAGED_FILES < <(get_worktree_bucket unstaged)
		mapfile -t SCOPE_STAGED_FILES < <(get_worktree_bucket staged)
		mapfile -t SCOPE_UNTRACKED_FILES < <(get_worktree_bucket untracked)
	fi

	local combined=("${SCOPE_BRANCH_FILES[@]}" "${SCOPE_UNSTAGED_FILES[@]}" "${SCOPE_STAGED_FILES[@]}" "${SCOPE_UNTRACKED_FILES[@]}")
	local path normalized reason
	for path in "${combined[@]}"; do
		[[ -n "${path}" ]] || continue
		normalized="$(normalize_path "${path}")"
		if is_scoped_first_party_file "${normalized}"; then
			SCOPE_INCLUDED_FILES+=("${normalized}")
		else
			reason="not-in-src"
			case "${normalized}" in
				src/engine/external/*) reason="third-party-external" ;;
				src/game/generated/*) reason="generated" ;;
				src/rust-bridge/base/*) reason="rust-bridge-base" ;;
				src/*)
					if [[ ! "${normalized}" =~ \.(c|cc|cpp|h|hpp)$ ]]; then
						reason="extension-not-supported"
					fi
					;;
			esac
			SCOPE_EXCLUDED_PATHS+=("${normalized}")
			SCOPE_EXCLUDED_REASONS+=("${reason}")
		fi
	done
	mapfile -t SCOPE_INCLUDED_FILES < <(printf '%s\n' "${SCOPE_INCLUDED_FILES[@]}" | unique_lines)
	SCOPE_DIAGNOSTICS_READY=1
}

write_scope_report() {
	[[ -n "${SCOPE_REPORT_PATH}" ]] || return 0
	mkdir -p "$(dirname "${SCOPE_REPORT_PATH}")"
	{
		printf '{\n'
		printf '  "BaseRef": "%s",\n' "$(json_escape "${BASE_REF}")"
		printf '  "BaseRefAvailable": %s,\n' "$( [[ ${BASE_REF_AVAILABLE} -eq 1 ]] && printf 'true' || printf 'false' )"
		printf '  "BaseRefFailureReason": "%s",\n' "$(json_escape "${BASE_REF_FAILURE_REASON}")"
		printf '  "IncludedFiles": ['
		local first=1 item
		for item in "${SCOPE_INCLUDED_FILES[@]}"; do [[ ${first} -eq 1 ]] || printf ','; printf '"%s"' "$(json_escape "${item}")"; first=0; done
		printf '],\n'
		printf '  "ExcludedFiles": [\n'
		local i
		for i in "${!SCOPE_EXCLUDED_PATHS[@]}"; do
			[[ ${i} -eq 0 ]] || printf ',\n'
			printf '    {"Path":"%s","Reason":"%s"}' "$(json_escape "${SCOPE_EXCLUDED_PATHS[$i]}")" "$(json_escape "${SCOPE_EXCLUDED_REASONS[$i]}")"
		done
		printf '\n  ]\n}\n'
	} > "${SCOPE_REPORT_PATH}"
	add_result "INFO" "差异范围报告" "已写入: ${SCOPE_REPORT_PATH}"
}

write_scope_diagnostics() {
	ensure_scope_diagnostics
	add_result "INFO" "差异范围统计" "branch=${#SCOPE_BRANCH_FILES[@]}, unstaged=${#SCOPE_UNSTAGED_FILES[@]}, staged=${#SCOPE_STAGED_FILES[@]}, untracked=${#SCOPE_UNTRACKED_FILES[@]}, included=${#SCOPE_INCLUDED_FILES[@]}, excluded=${#SCOPE_EXCLUDED_PATHS[@]}"
	if [[ ${BASE_REF_AVAILABLE} -eq 0 ]]; then
		local message="差异基线不可用: ${BASE_REF}"
		[[ -z "${BASE_REF_FAILURE_REASON}" ]] || message="${message} (${BASE_REF_FAILURE_REASON})"
		add_result "WARN" "差异基线检查" "${message}"
	fi
	if [[ ${EXPLAIN_SCOPE} -eq 1 ]]; then
		write_section "差异范围说明"
		printf 'BaseRef: %s\n' "${BASE_REF}"
		printf 'BaseRef 可用: %s\n' "$( [[ ${BASE_REF_AVAILABLE} -eq 1 ]] && printf 'true' || printf 'false' )"
		[[ -z "${BASE_REF_FAILURE_REASON}" ]] || printf 'BaseRef 失败原因: %s\n' "${BASE_REF_FAILURE_REASON}"
		printf '纳入首方范围文件数: %s\n' "${#SCOPE_INCLUDED_FILES[@]}"
		printf '排除文件数: %s\n' "${#SCOPE_EXCLUDED_PATHS[@]}"
	fi
	write_scope_report
}

assert_worktree_preflight() {
	write_section "环境前置检查"
	local required_path
	for required_path in \
		"${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" \
		"${REPO_ROOT}/qmclient_scripts/gate/strict-debug-check.sh" \
		"${REPO_ROOT}/qmclient_scripts/gate/check_workflow_docs.py" \
		"${REPO_ROOT}/qmclient_scripts/gate/sync_agents_claude.py" \
		"${REPO_ROOT}/scripts/check_header_guards.py" \
		"${REPO_ROOT}/scripts/check_standard_headers.py" \
		"${REPO_ROOT}/scripts/fix_style.py"; do
		[[ -e "${required_path}" ]] || { add_result "FAIL" "脚本入口存在性" "缺少必需路径: ${required_path}"; return 1; }
	done
	add_result "PASS" "脚本入口存在性" "核心检查脚本均已找到"
}

invoke_python_repo_command() {
	local title="$1"
	shift
	local py_cmd
	py_cmd="$(get_python_cmd)"
	local args=()
	local arg
	for arg in "$@"; do
		if python_uses_windows_paths "${py_cmd}" && [[ "${arg}" == /* || "${arg}" =~ ^/mnt/[A-Za-z]/ ]]; then
			args+=("$(to_windows_path "${arg}")")
		else
			args+=("${arg}")
		fi
	done
	case "${py_cmd}" in
		py|py.exe|*/py|*/py.exe|*\\py|*\\py.exe)
			invoke_repo_command "${title}" FAIL "${py_cmd}" -3 "${args[@]}"
			;;
		*)
			invoke_repo_command "${title}" FAIL "${py_cmd}" "${args[@]}"
			;;
	esac
}

invoke_config_checks() {
	invoke_python_repo_command "配置变量使用检查（Qm/Tc/栖梦）" "${REPO_ROOT}/qmclient_scripts/check_config_variables.py" --qm
}

invoke_workflow_doc_checks() {
	invoke_python_repo_command "Claude / AGENTS 自动同步" "${REPO_ROOT}/qmclient_scripts/gate/sync_agents_claude.py"
	invoke_python_repo_command "工作流文档一致性检查" "${REPO_ROOT}/qmclient_scripts/gate/check_workflow_docs.py"
}

invoke_header_checks() {
	invoke_python_repo_command "头文件 guard 检查" "${REPO_ROOT}/scripts/check_header_guards.py"
	invoke_python_repo_command "标准头文件检查" "${REPO_ROOT}/scripts/check_standard_headers.py"
}

invoke_style_checks() {
	mapfile -t SCOPE_INCLUDED_FILES < <(printf '%s\n' "${SCOPE_INCLUDED_FILES[@]}" | unique_lines)
	if [[ ${#SCOPE_INCLUDED_FILES[@]} -eq 0 ]]; then
		add_result "WARN" "代码格式检查" "未收集到改动范围内可供 fix_style.py 检查的首方 C/C++ 文件"
		return 0
	fi
	add_result "INFO" "代码格式检查范围" "按收敛后的首方源码范围传入 ${#SCOPE_INCLUDED_FILES[@]} 个文件"
	local py_cmd
	py_cmd="$(get_python_cmd)"
	local fix_style_path="${REPO_ROOT}/scripts/fix_style.py"
	if python_uses_windows_paths "${py_cmd}" && [[ "${fix_style_path}" == /* || "${fix_style_path}" =~ ^/mnt/[A-Za-z]/ ]]; then
		fix_style_path="$(to_windows_path "${fix_style_path}")"
	fi
	local batch=()
	local batch_len=0
	local file file_len
	for file in "${SCOPE_INCLUDED_FILES[@]}"; do
		file_len=$((${#file} + 3))
		if [[ ${#batch[@]} -gt 0 && $((batch_len + file_len)) -gt 6000 ]]; then
			invoke_style_check_batch "${py_cmd}" "${fix_style_path}" "${batch[@]}"
			batch=()
			batch_len=0
		fi
		batch+=("${file}")
		batch_len=$((batch_len + file_len))
	done
	if [[ ${#batch[@]} -gt 0 ]]; then
		invoke_style_check_batch "${py_cmd}" "${fix_style_path}" "${batch[@]}"
	fi
}

invoke_identifier_checks() {
	local source_files=()
	local file
	for file in "${SCOPE_INCLUDED_FILES[@]}"; do
		[[ "${file}" =~ \.(c|cc|cpp)$ ]] && source_files+=("${REPO_ROOT}/${file}")
	done
	if [[ ${#source_files[@]} -eq 0 ]]; then
		add_result "WARN" "标识符命名检查" "未找到改动范围内可供 extract_identifiers.py 分析的首方源文件"
		return 0
	fi
	write_section "标识符命名检查"
	local tmp_file
	tmp_file="$(mktemp)"
	python_run "${REPO_ROOT}/scripts/extract_identifiers.py" "${source_files[@]}" > "${tmp_file}"
	python_run "${REPO_ROOT}/scripts/check_identifiers.py" < "${tmp_file}"
	rm -f "${tmp_file}"
	add_result "PASS" "标识符命名检查" "命名风格检查通过"
}

invoke_unused_header_checks() {
	invoke_python_repo_command "未使用头文件检查" "${REPO_ROOT}/scripts/check_unused_header_files.py"
}

invoke_clang_format_checks() {
	command -v clang-format >/dev/null 2>&1 || { add_result "WARN" "clang-format 附加检查" "PATH 中未找到 clang-format，已跳过"; return 0; }
	local file
	for file in "${SCOPE_INCLUDED_FILES[@]}"; do
		invoke_repo_command "clang-format 附加检查: ${file}" WARN clang-format --dry-run --Werror "${REPO_ROOT}/${file}"
	done
}

invoke_full_clang_tidy_warn_checks() {
	command -v clang-tidy >/dev/null 2>&1 || { add_result "WARN" "全量 .clang-tidy 附加检查" "PATH 中未找到 clang-tidy，已跳过"; return 0; }
	[[ -f "${REPO_ROOT}/build-debug/compile_commands.json" ]] || { add_result "WARN" "全量 .clang-tidy 附加检查" "缺少 build-debug/compile_commands.json，请先跑 strict-debug-check 或 default/full 构建层"; return 0; }
	local file
	for file in "${SCOPE_INCLUDED_FILES[@]}"; do
		invoke_repo_command "全量 .clang-tidy 附加检查: ${file}" WARN clang-tidy "${file}" -p=build-debug "--config-file=${REPO_ROOT}/.clang-tidy" --extra-arg=-Qunused-arguments --quiet
	done
}

invoke_strict_debug_gate() {
	local args=(bash "${REPO_ROOT}/qmclient_scripts/gate/strict-debug-check.sh" --base-ref "${BASE_REF}")
	if [[ ${ANALYZE_ALL} -eq 1 ]]; then
		args+=(--analyze-all)
	elif [[ ${#SCOPE_INCLUDED_FILES[@]} -gt 0 ]]; then
		add_result "INFO" "严格构建与静态分析范围" "按首方源码差异传入 ${#SCOPE_INCLUDED_FILES[@]} 个文件（已排除 external/generated 等目录）"
		args+=(--files "${SCOPE_INCLUDED_FILES[@]}")
	else
		add_result "WARN" "严格构建与静态分析范围" "未收集到首方 src 差异文件；strict-debug-check 将保留 Debug CRT 构建，并跳过分析/ tidy 范围检查"
		args+=(--files --skip-analyze --skip-tidy)
	fi
	invoke_repo_command "严格构建与静态分析入口" FAIL "${args[@]}"
}

invoke_test_targets() {
	if [[ ${RUN_ALL_TESTS} -eq 1 ]]; then
		invoke_repo_command "CMake run_tests" FAIL "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" --build "${BUILD_DIR}" --target run_tests -j 10
		return 0
	fi
	if [[ ${MODE_RUN_CXX_TESTS} -eq 1 && ${SKIP_CXX_TESTS} -eq 0 ]]; then
		invoke_repo_command "CMake run_cxx_tests" FAIL "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" --build "${BUILD_DIR}" --target run_cxx_tests -j 10
	fi
	if [[ ${MODE_RUN_RUST_TESTS} -eq 1 ]]; then
		invoke_repo_command "CMake run_rust_tests" FAIL "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" --build "${BUILD_DIR}" --target run_rust_tests -j 10
	fi
}

write_json_report() {
	[[ -n "${REPORT_JSON_PATH}" ]] || return 0
	mkdir -p "$(dirname "${REPORT_JSON_PATH}")"
	{
		printf '{\n'
		printf '  "Mode": "%s",\n' "$(json_escape "${MODE}")"
		printf '  "BaseRef": "%s",\n' "$(json_escape "${BASE_REF}")"
		printf '  "GeneratedAt": "%s",\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
		printf '  "ModeSpec": {"Name":"%s","Target":"%s","Expectation":"%s","BlockingRule":"%s"},\n' \
			"$(json_escape "${MODE}")" "$(json_escape "${MODE_TARGET}")" "$(json_escape "${MODE_EXPECTATION}")" "$(json_escape "${MODE_BLOCKING_RULE}")"
		printf '  "Summary": {"Pass": %s, "Warn": %s, "Fail": %s},\n' "${RESULT_PASS}" "${RESULT_WARN}" "${RESULT_FAIL}"
		printf '  "ScopedFiles": ['
		local first=1 file
		for file in "${SCOPE_INCLUDED_FILES[@]}"; do [[ ${first} -eq 1 ]] || printf ','; printf '"%s"' "$(json_escape "${file}")"; first=0; done
		printf '],\n'
		printf '  "Items": [\n'
		local i item level original title detail category_id category_label allowlist_reason detail_hash
		for i in "${!RESULT_ITEMS[@]}"; do
			item="${RESULT_ITEMS[$i]}"
			level="${item%%$'\t'*}"
			item="${item#*$'\t'}"; original="${item%%$'\t'*}"
			item="${item#*$'\t'}"; title="${item%%$'\t'*}"
			item="${item#*$'\t'}"; detail="${item%%$'\t'*}"
			item="${item#*$'\t'}"; category_id="${item%%$'\t'*}"
			item="${item#*$'\t'}"; category_label="${item%%$'\t'*}"
			item="${item#*$'\t'}"; allowlist_reason="${item%%$'\t'*}"
			detail_hash="${item#*$'\t'}"
			[[ ${i} -eq 0 ]] || printf ',\n'
			printf '    {"Level":"%s","OriginalLevel":"%s","Title":"%s","Detail":"%s","CategoryId":"%s","CategoryLabel":"%s","AllowlistReason":"%s","DetailHash":"%s"}' \
				"$(json_escape "${level}")" "$(json_escape "${original}")" "$(json_escape "${title}")" "$(json_escape "${detail}")" \
				"$(json_escape "${category_id}")" "$(json_escape "${category_label}")" "$(json_escape "${allowlist_reason}")" "$(json_escape "${detail_hash}")"
		done
		printf '\n  ]\n}\n'
	} > "${REPORT_JSON_PATH}"
}

write_summary() {
	write_section "检查汇总"
	write_result_line "INFO" "模式: ${MODE}"
	write_result_line "INFO" "模式目标: ${MODE_TARGET}"
	write_result_line "INFO" "模式预期: ${MODE_EXPECTATION}"
	write_result_line "INFO" "阻断规则: ${MODE_BLOCKING_RULE}"
	write_result_line "INFO" "通过: ${RESULT_PASS}"
	write_result_line "INFO" "警告: ${RESULT_WARN}"
	write_result_line "INFO" "失败: ${RESULT_FAIL}"
}

usage() {
	cat <<'EOF'
用法：
  check-gate.sh [options]

选项：
  --build-dir DIR
  --base-ref REF
  --mode quick|default|full
  --analyze-all
  --skip-preflight
  --skip-config-checks
  --skip-workflow-docs
  --skip-header-checks
  --skip-style-check
  --skip-strict-debug
  --skip-cxx-tests
  --run-rust-tests
  --run-all-tests
  --include-identifier-check
  --include-unused-header-check
  --enable-clang-format-check
  --enable-full-clang-tidy-warn
  --strict-environment
  --dry-run
  --explain-scope
  --branch-scope-only
  --report-json-path PATH
  --scope-report-path PATH
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir) BUILD_DIR="$2"; shift 2 ;;
		--base-ref) BASE_REF="$2"; shift 2 ;;
		--mode) MODE="$2"; shift 2 ;;
		--analyze-all) ANALYZE_ALL=1; shift ;;
		--skip-preflight) SKIP_PREFLIGHT=1; shift ;;
		--skip-config-checks) SKIP_CONFIG_CHECKS=1; shift ;;
		--skip-workflow-docs) SKIP_WORKFLOW_DOCS=1; shift ;;
		--skip-header-checks) SKIP_HEADER_CHECKS=1; shift ;;
		--skip-style-check) SKIP_STYLE_CHECK=1; shift ;;
		--skip-strict-debug) SKIP_STRICT_DEBUG=1; shift ;;
		--skip-cxx-tests) SKIP_CXX_TESTS=1; shift ;;
		--run-rust-tests) RUN_RUST_TESTS=1; shift ;;
		--run-all-tests) RUN_ALL_TESTS=1; shift ;;
		--include-identifier-check) INCLUDE_IDENTIFIER_CHECK=1; shift ;;
		--include-unused-header-check) INCLUDE_UNUSED_HEADER_CHECK=1; shift ;;
		--enable-clang-format-check) ENABLE_CLANG_FORMAT_CHECK=1; shift ;;
		--enable-full-clang-tidy-warn) ENABLE_FULL_CLANG_TIDY_WARN=1; shift ;;
		--strict-environment) STRICT_ENVIRONMENT=1; shift ;;
		--dry-run) DRY_RUN=1; shift ;;
		--explain-scope) EXPLAIN_SCOPE=1; shift ;;
		--branch-scope-only) BRANCH_SCOPE_ONLY=1; shift ;;
		--report-json-path) REPORT_JSON_PATH="$2"; shift 2 ;;
		--scope-report-path) SCOPE_REPORT_PATH="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) printf '未知参数: %s\n' "$1" >&2; usage >&2; exit 2 ;;
	esac
done

resolve_mode_toggles
load_baseline_allowlist
ensure_scope_diagnostics

if [[ ${SKIP_PREFLIGHT} -eq 0 ]]; then
	assert_worktree_preflight || true
fi
write_scope_diagnostics
if [[ ${SKIP_CONFIG_CHECKS} -eq 0 ]]; then
	invoke_config_checks || true
fi
if [[ ${SKIP_WORKFLOW_DOCS} -eq 0 ]]; then
	invoke_workflow_doc_checks || true
fi
if [[ ${SKIP_HEADER_CHECKS} -eq 0 ]]; then
	invoke_header_checks || true
fi
if [[ ${SKIP_STYLE_CHECK} -eq 0 ]]; then
	invoke_style_checks || true
fi
if [[ ${MODE_RUN_IDENTIFIER_CHECK} -eq 1 ]]; then
	invoke_identifier_checks || true
fi
if [[ ${MODE_RUN_UNUSED_HEADER_CHECK} -eq 1 ]]; then
	invoke_unused_header_checks || true
fi
if [[ ${MODE_RUN_CLANG_FORMAT_CHECK} -eq 1 ]]; then
	invoke_clang_format_checks || true
fi
if [[ ${MODE_RUN_STRICT_DEBUG} -eq 1 && ${SKIP_STRICT_DEBUG} -eq 0 ]]; then
	invoke_strict_debug_gate || true
fi
if [[ ${MODE_RUN_FULL_CLANG_TIDY_WARN} -eq 1 ]]; then
	invoke_full_clang_tidy_warn_checks || true
fi
invoke_test_targets || true
write_json_report
write_summary

if [[ ${RESULT_FAIL} -gt 0 ]]; then
	exit 1
fi

printf '\n仓库级检查完成。\n'
