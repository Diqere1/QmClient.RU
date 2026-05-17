#!/usr/bin/env bash
set -euo pipefail
export MSYS2_ARG_CONV_EXCL='*'

# -----------------------------------------------------------------------------
# QmClient Bash 严格调试检查入口
#
# 设计目标：
# 1. 提供 bash 主实现，不再转调 strict-debug-check.ps1。
# 2. Windows 上继续复用 qmclient_scripts/cmake-windows.cmd，避免假设调用端已手工注入 MSVC 环境。
# 3. 构建目录语义与 AGENTS.md 保持一致，并在这里直接解释“为什么要分这些目录”。
#
# 构建目录口径：
# - build-ninja
#   默认 Release 运行目录。它是“真实运行事实源”，承载发布态行为、资源同步、着色器与 run_*_tests。
#   原理：把运行验证固定绑到单一 Release 目录，避免 Debug/诊断产物污染发布态判断。
# - build-debug
#   默认 Debug 诊断目录。它承载断言、RTC、变量可读性，以及 compile_commands.json 导出。
#   原理：把“便于调试”的编译开关与发布态隔离，避免两种行为混在同一目录里。
# - build-analyze
#   Windows 专用的 Debug + MSVC /analyze 目录，只供静态分析使用。
#   原理：/analyze 会改变诊断与编译行为，必须与普通 Debug 产物隔离，避免误把分析目录当运行目录。
# - build-asan
#   实验性 AddressSanitizer 目录。当前 Rust +crt-static 约束下通常允许跳过。
#   原理：ASan 需要单独的编译/链接选项，和普通 Debug/Release 共用目录会污染缓存与二进制语义。
# - build-release-pdb
#   临时 Release+PDB 诊断目录，不属于本脚本默认主链，只在 release-only 崩溃需要源码栈时按需使用。
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
REPO_ROOT_GIT="${REPO_ROOT}"

DEBUG_BUILD_DIR="build-debug"
ANALYZE_BUILD_DIR="build-analyze"
BASE_REF="main"
ANALYZE_ALL=0
SKIP_BUILD=0
SKIP_TIDY=0
SKIP_ANALYZE=0
PRINT_FILE_SCOPE=0
REPORT_JSON_PATH=""
INPUT_FILES=()
INPUT_FILES_EXPLICIT=0
RAW_INPUT_FILES=()

RESULT_PASS=0
RESULT_WARN=0
RESULT_FAIL=0
DEGRADED=0
INPUT_SCOPE_FILES=()
EFFECTIVE_FILES=()
ANALYZE_SOURCE_FILES=()
RESULT_ITEMS=()

declare -a DEFAULT_SCOPE_BRANCH=()
declare -a DEFAULT_SCOPE_UNSTAGED=()
declare -a DEFAULT_SCOPE_STAGED=()
declare -a DEFAULT_SCOPE_UNTRACKED=()
declare -a DEFAULT_SCOPE_SCOPED=()
declare -a DEFAULT_SCOPE_EXCLUDED=()
declare -a GIT_CMD=()

json_escape() {
	local value="${1:-}"
	value="${value//\\/\\\\}"
	value="${value//\"/\\\"}"
	value="${value//$'\n'/\\n}"
	value="${value//$'\r'/\\r}"
	value="${value//$'\t'/\\t}"
	printf '%s' "${value}"
}

add_result() {
	local level="$1"
	local title="$2"
	local detail="$3"

	case "${level}" in
		PASS) RESULT_PASS=$((RESULT_PASS + 1)) ;;
		WARN) RESULT_WARN=$((RESULT_WARN + 1)) ;;
		FAIL) RESULT_FAIL=$((RESULT_FAIL + 1)) ;;
	esac

	RESULT_ITEMS+=("${level}"$'\t'"${title}"$'\t'"${detail}")
}

write_section() {
	printf '\n==> %s\n' "$1"
}

write_result_line() {
	printf '[%s] %s\n' "$1" "$2"
}

mark_degraded() {
	DEGRADED=1
	add_result "WARN" "$1" "$2"
}

is_windows_env() {
	case "$(uname -s)" in
		MINGW*|MSYS*|CYGWIN*) return 0 ;;
		*) return 1 ;;
	esac
}

is_windows_host_bash() {
	if is_windows_env; then
		return 0
	fi
	command -v cmd.exe >/dev/null 2>&1
}

to_windows_path() {
	local path="$1"
	if is_windows_env && command -v cygpath >/dev/null 2>&1; then
		cygpath -w "${path}"
	elif command -v wslpath >/dev/null 2>&1; then
		wslpath -w "${path}"
	elif command -v cygpath >/dev/null 2>&1; then
		cygpath -w "${path}"
	else
		printf '%s' "${path}"
	fi
}

normalize_path() {
	local path="$1"
	path="${path//\\//}"
	printf '%s' "${path}"
}

normalize_input_file_arg() {
	local path="$1"
	local normalized compare_path compare_repo repo_prefix
	normalized="$(normalize_path "${path}")"
	compare_path="${normalized,,}"

	for repo_prefix in "${REPO_ROOT}" "${REPO_ROOT_GIT}"; do
		compare_repo="$(normalize_path "${repo_prefix}")"
		compare_repo="${compare_repo,,}"
		if [[ -z "${compare_repo}" ]]; then
			continue
		fi
		if [[ "${compare_path}" == "${compare_repo}" ]]; then
			printf '.'
			return 0
		fi
		if [[ "${compare_path}" == "${compare_repo}/"* ]]; then
			normalized="${normalized:${#repo_prefix}}"
			normalized="${normalized#/}"
			printf '%s' "${normalized}"
			return 0
		fi
	done

	if is_windows_host_bash && [[ "${normalized}" =~ ^[A-Za-z]:/ ]]; then
		if command -v wslpath >/dev/null 2>&1; then
			normalized="$(wslpath "${normalized}")"
		elif command -v cygpath >/dev/null 2>&1; then
			normalized="$(cygpath "${normalized}")"
		fi
		compare_path="$(normalize_path "${normalized}")"
		compare_path="${compare_path,,}"
		compare_repo="$(normalize_path "${REPO_ROOT}")"
		compare_repo="${compare_repo,,}"
		if [[ "${compare_path}" == "${compare_repo}" ]]; then
			printf '.'
			return 0
		fi
		if [[ "${compare_path}" == "${compare_repo}/"* ]]; then
			normalized="${normalized:${#REPO_ROOT}}"
			normalized="${normalized#/}"
			printf '%s' "$(normalize_path "${normalized}")"
			return 0
		fi
	fi

	printf '%s' "${normalized}"
}

join_by() {
	local delimiter="$1"
	shift || true
	local first=1
	local item
	for item in "$@"; do
		if [[ ${first} -eq 1 ]]; then
			printf '%s' "${item}"
			first=0
		else
			printf '%s%s' "${delimiter}" "${item}"
		fi
	done
}

unique_lines() {
	awk 'NF { if(!seen[$0]++) print $0 }'
}

capture_git_lines() {
	local output
	if ! output="$("${GIT_CMD[@]}" "$@" 2>/dev/null)"; then
		return 0
	fi
	printf '%s\n' "${output}" | sed 's/\r$//' | awk 'NF'
}

is_scoped_first_party_file() {
	local path
	path="$(normalize_path "$1")"
	if [[ ! "${path}" =~ ^src/.+\.(c|cc|cpp|h|hpp)$ ]]; then
		return 1
	fi
	case "${path}" in
		src/engine/external/*|src/game/generated/*|src/rust-bridge/base/*) return 1 ;;
	esac
	return 0
}

write_default_scope_diagnostics() {
	add_result "INFO" "默认文件范围统计" \
		"branch=${#DEFAULT_SCOPE_BRANCH[@]}, unstaged=${#DEFAULT_SCOPE_UNSTAGED[@]}, staged=${#DEFAULT_SCOPE_STAGED[@]}, untracked=${#DEFAULT_SCOPE_UNTRACKED[@]}, scoped=${#DEFAULT_SCOPE_SCOPED[@]}, excluded=${#DEFAULT_SCOPE_EXCLUDED[@]}"

	if [[ ${PRINT_FILE_SCOPE} -eq 1 ]]; then
		write_section "默认文件范围说明"
		printf 'BaseRef: %s\n' "${BASE_REF}"
		printf '分支差异文件数: %s\n' "${#DEFAULT_SCOPE_BRANCH[@]}"
		printf '未暂存文件数: %s\n' "${#DEFAULT_SCOPE_UNSTAGED[@]}"
		printf '已暂存文件数: %s\n' "${#DEFAULT_SCOPE_STAGED[@]}"
		printf '未跟踪文件数: %s\n' "${#DEFAULT_SCOPE_UNTRACKED[@]}"
		printf '纳入严格检查的首方文件数: %s\n' "${#DEFAULT_SCOPE_SCOPED[@]}"
		printf '排除文件数: %s\n' "${#DEFAULT_SCOPE_EXCLUDED[@]}"
		if [[ ${#DEFAULT_SCOPE_SCOPED[@]} -gt 0 ]]; then
			printf '纳入文件：\n'
			printf '  - %s\n' "${DEFAULT_SCOPE_SCOPED[@]}"
		fi
		if [[ ${#DEFAULT_SCOPE_EXCLUDED[@]} -gt 0 ]]; then
			printf '排除文件：\n'
			printf '  - %s\n' "${DEFAULT_SCOPE_EXCLUDED[@]}"
		fi
	fi
}

collect_default_scope() {
	local merge_base_output=""
	local merge_base=""

	if merge_base_output="$("${GIT_CMD[@]}" -C "${REPO_ROOT_GIT}" merge-base "${BASE_REF}" HEAD 2>&1)" && [[ -n "${merge_base_output}" ]]; then
		merge_base="$(printf '%s\n' "${merge_base_output}" | head -n 1 | tr -d '\r')"
		mapfile -t DEFAULT_SCOPE_BRANCH < <(
			capture_git_lines -C "${REPO_ROOT_GIT}" -c core.safecrlf=false diff --name-only --diff-filter=ACMR "${merge_base}...HEAD" -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" | unique_lines
		)
	else
		local reason
		reason="$(printf '%s' "${merge_base_output}" | tr '\r' ' ' | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
		if [[ -z "${reason}" ]]; then
			reason="git merge-base 返回空结果"
		fi
		mark_degraded "分支差异基线" "无法解析 BaseRef=${BASE_REF}，将退回仅使用工作树差异。原因：${reason}"
		DEFAULT_SCOPE_BRANCH=()
	fi

	mapfile -t DEFAULT_SCOPE_UNSTAGED < <(
		capture_git_lines -C "${REPO_ROOT_GIT}" -c core.safecrlf=false diff --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" | unique_lines
	)
	mapfile -t DEFAULT_SCOPE_STAGED < <(
		capture_git_lines -C "${REPO_ROOT_GIT}" -c core.safecrlf=false diff --cached --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" | unique_lines
	)
	mapfile -t DEFAULT_SCOPE_UNTRACKED < <(
		capture_git_lines -C "${REPO_ROOT_GIT}" ls-files --others --exclude-standard -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" | unique_lines
	)

	local combined=()
	combined+=("${DEFAULT_SCOPE_BRANCH[@]}")
	combined+=("${DEFAULT_SCOPE_UNSTAGED[@]}")
	combined+=("${DEFAULT_SCOPE_STAGED[@]}")
	combined+=("${DEFAULT_SCOPE_UNTRACKED[@]}")

	local path normalized reason
	local scoped_tmp=()
	DEFAULT_SCOPE_EXCLUDED=()
	for path in "${combined[@]}"; do
		[[ -z "${path}" ]] && continue
		normalized="$(normalize_path "${path}")"
		if is_scoped_first_party_file "${normalized}"; then
			scoped_tmp+=("${normalized}")
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
			DEFAULT_SCOPE_EXCLUDED+=("${normalized} [${reason}]")
		fi
	done

	mapfile -t DEFAULT_SCOPE_SCOPED < <(printf '%s\n' "${scoped_tmp[@]}" | unique_lines)
	write_default_scope_diagnostics
}

get_analyze_source_files() {
	local path normalized
	local filtered=()
	for path in "$@"; do
		[[ -z "${path}" ]] && continue
		normalized="$(normalize_path "${path}")"
		if [[ "${normalized}" == src/* && "${normalized}" =~ \.(c|cc|cpp)$ ]]; then
			filtered+=("${normalized}")
		fi
	done
	printf '%s\n' "${filtered[@]}" | unique_lines
}

is_showincludes_line() {
	local line="$1"
	[[ "${line}" == "注意: 包含文件:"* ]]
}

is_strict_warning_line() {
	local line="$1"
	[[ -z "${line// }" ]] && return 1
	printf '%s\n' "${line}" | grep -Eq '^CMake[[:space:]]Warning' && return 0
	printf '%s\n' "${line}" | grep -Eq '(^|: |[[:space:]])warning[[:space:]]C[0-9]+([^[:alnum:]_]|$)' && return 0
	printf '%s\n' "${line}" | grep -Eq ':[[:space:]]*warning([^[:alnum:]_]|$)' && return 0
	printf '%s\n' "${line}" | grep -Eq '(^|[^[:alnum:]_])warning:([^[:alnum:]_]|$)' && return 0
	printf '%s\n' "${line}" | grep -Eq '^(cl|clang-cl|link|LINK)[[:space:]]*:[[:space:]]warning([^[:alnum:]_]|$)' && return 0
	printf '%s\n' "${line}" | grep -Eq '^[0-9]+[[:space:]]warnings[[:space:]]generated\.$' && return 0
	printf '%s\n' "${line}" | grep -Eq '^Suppressed[[:space:]][0-9]+[[:space:]]warnings' && return 0
	return 1
}

is_ignorable_tool_warning_line() {
	local line="$1"
	[[ "${line}" =~ ^CMake[[:space:]]Warning[[:space:]]\(dev\) ]]
}

repo_command_failure_summary() {
	local title="$1"
	local output_file="$2"

	if [[ "${title}" != clang-tidy\ 严格检查:* ]]; then
		return 0
	fi

	local unused_lines
	unused_lines="$(grep 'clang-diagnostic-unused-command-line-argument' "${output_file}" | head -n 6 || true)"
	if [[ -n "${unused_lines}" ]]; then
		printf '原因：clang-tidy 看到的是 MSVC compile_commands 参数，其中一部分 clang 自身不会消费。这更像工具链调用问题，不一定是源码诊断。相关输出：\n%s' "${unused_lines}"
		return 0
	fi

	local relevant_lines
	relevant_lines="$(grep -E '^error:|^[0-9]+ warnings and [0-9]+ errors generated\.$|^Error while processing |^Found compiler error\(s\)\.$' "${output_file}" | head -n 8 || true)"
	if [[ -n "${relevant_lines}" ]]; then
		printf '原因：clang-tidy 找到了真实诊断或编译错误。相关输出：\n%s' "${relevant_lines}"
	fi
}

invoke_repo_command() {
	local title="$1"
	local fail_on_warnings="$2"
	shift 2

	write_section "${title}"
	printf '命令:'
	printf ' %q' "$@"
	printf '\n'

	local output_file
	output_file="$(mktemp)"
	local line exit_code
	set +e
	"$@" >"${output_file}" 2>&1
	exit_code=$?
	set -e

	while IFS= read -r line || [[ -n "${line}" ]]; do
		if ! is_showincludes_line "${line}"; then
			printf '%s\n' "${line}"
		fi
	done < "${output_file}"

	if [[ ${exit_code} -ne 0 ]]; then
		local failure_message summary
		failure_message="${title} 失败，退出码 ${exit_code}"
		summary="$(repo_command_failure_summary "${title}" "${output_file}" || true)"
		if [[ -n "${summary}" ]]; then
			failure_message+=$'\n'"${summary}"
		fi
		add_result "FAIL" "${title}" "${failure_message}"
		rm -f "${output_file}"
		return 1
	fi

	if [[ "${fail_on_warnings}" == "1" ]]; then
		local warning_lines=()
		while IFS= read -r line || [[ -n "${line}" ]]; do
			if is_strict_warning_line "${line}" && ! is_ignorable_tool_warning_line "${line}"; then
				warning_lines+=("${line}")
			fi
		done < "${output_file}"
		if [[ ${#warning_lines[@]} -gt 0 ]]; then
			local warning_message
			warning_message="${title} 输出了 warning 文本:"
			warning_message+=$'\n'"$(printf '%s\n' "${warning_lines[@]}")"
			add_result "FAIL" "${title}" "${warning_message}"
			rm -f "${output_file}"
			return 1
		fi
	fi

	add_result "PASS" "${title}" "执行通过"
	rm -f "${output_file}"
}

invoke_configure_and_build() {
	local title="$1"
	local build_dir="$2"
	local fail_on_warnings="$3"
	shift 3

	invoke_repo_command "${title} 配置" "${fail_on_warnings}" "$@"
	if [[ ${SKIP_BUILD} -eq 0 ]]; then
		if [[ "${CM_CMD}" == "cmd.exe" ]]; then
			local cmake_script
			cmake_script="$(to_windows_path "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd")"
			invoke_repo_command "${title} 构建" "${fail_on_warnings}" cmd.exe /c "${cmake_script}" --build "${build_dir}" --target game-client -j 10
		else
			invoke_repo_command "${title} 构建" "${fail_on_warnings}" cmake --build "${build_dir}" --target game-client -j 10
		fi
	else
		add_result "WARN" "${title} 构建" "已显式传入 -SkipBuild，仅执行配置阶段"
	fi
}

test_asan_supported() {
	local cargo_config="${REPO_ROOT}/.cargo/config.toml"
	[[ ! -f "${cargo_config}" ]] && return 0
	! grep -q 'target-feature=+crt-static' "${cargo_config}"
}

resolve_cmake_command() {
	# On Windows, prefer cmake-windows.cmd which sets up MSVC environment.
	# Direct cmake in PATH may pick up clang (from choco install llvm)
	# instead of MSVC, which can't compile WinRT/C++ code.
	if is_windows_host_bash && [[ -f "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" ]]; then
		printf '%s\n' "cmd.exe"
		return 0
	fi
	if command -v cmake >/dev/null 2>&1; then
		printf '%s\n' "cmake"
		return 0
	fi
	return 1
}

configure_git_command() {
	if is_windows_host_bash && command -v git.exe >/dev/null 2>&1; then
		GIT_CMD=("git.exe")
		REPO_ROOT_GIT="$(to_windows_path "${REPO_ROOT}")"
	elif command -v git >/dev/null 2>&1; then
		GIT_CMD=("git")
		REPO_ROOT_GIT="${REPO_ROOT}"
	else
		add_result "FAIL" "入口前置检查" "PATH 中未找到 git/git.exe"
		return 1
	fi
}

write_json_string_array() {
	local first=1
	local item
	printf '['
	for item in "$@"; do
		[[ ${first} -eq 1 ]] || printf ','
		printf '"%s"' "$(json_escape "${item}")"
		first=0
	done
	printf ']'
}

write_json_report() {
	[[ -z "${REPORT_JSON_PATH}" ]] && return 0
	mkdir -p "$(dirname "${REPORT_JSON_PATH}")"

	{
		printf '{\n'
		printf '  "DebugBuildDir": "%s",\n' "$(json_escape "${DEBUG_BUILD_DIR}")"
		printf '  "AnalyzeBuildDir": "%s",\n' "$(json_escape "${ANALYZE_BUILD_DIR}")"
		printf '  "AsanBuildDir": "%s",\n' "$(json_escape "${ASAN_BUILD_DIR}")"
		printf '  "BaseRef": "%s",\n' "$(json_escape "${BASE_REF}")"
		printf '  "Degraded": %s,\n' "$( [[ ${DEGRADED} -eq 1 ]] && printf true || printf false )"
		printf '  "InputScope": {\n'
		printf '    "ExplicitFiles": %s,\n' "$( [[ ${INPUT_FILES_EXPLICIT} -eq 1 ]] && printf true || printf false )"
		printf '    "Files": '; write_json_string_array "${INPUT_SCOPE_FILES[@]}"; printf '\n'
		printf '  },\n'
		printf '  "DefaultScope": {\n'
		printf '    "Branch": '; write_json_string_array "${DEFAULT_SCOPE_BRANCH[@]}"; printf ',\n'
		printf '    "Unstaged": '; write_json_string_array "${DEFAULT_SCOPE_UNSTAGED[@]}"; printf ',\n'
		printf '    "Staged": '; write_json_string_array "${DEFAULT_SCOPE_STAGED[@]}"; printf ',\n'
		printf '    "Untracked": '; write_json_string_array "${DEFAULT_SCOPE_UNTRACKED[@]}"; printf ',\n'
		printf '    "Scoped": '; write_json_string_array "${DEFAULT_SCOPE_SCOPED[@]}"; printf ',\n'
		printf '    "Excluded": ['
		local excluded_idx=0 excluded_item excluded_path excluded_reason
		for excluded_item in "${DEFAULT_SCOPE_EXCLUDED[@]}"; do
			excluded_path="${excluded_item%% \[*}"
			excluded_reason="${excluded_item##*\[}"
			excluded_reason="${excluded_reason%]}"
			[[ ${excluded_idx} -eq 0 ]] || printf ','
			printf '{"Path":"%s","Reason":"%s"}' \
				"$(json_escape "${excluded_path}")" \
				"$(json_escape "${excluded_reason}")"
			excluded_idx=$((excluded_idx + 1))
		done
		printf ']\n'
		printf '  },\n'
		printf '  "EffectiveScope": {\n'
		printf '    "Files": '; write_json_string_array "${EFFECTIVE_FILES[@]}"; printf ',\n'
		printf '    "AnalyzeSourceFiles": '; write_json_string_array "${ANALYZE_SOURCE_FILES[@]}"; printf '\n'
		printf '  },\n'
		printf '  "Summary": {\n'
		printf '    "Pass": %s,\n' "${RESULT_PASS}"
		printf '    "Warn": %s,\n' "${RESULT_WARN}"
		printf '    "Fail": %s\n' "${RESULT_FAIL}"
		printf '  },\n'
		printf '  "Items": [\n'
		local idx=0 item level title detail
		for item in "${RESULT_ITEMS[@]}"; do
			level="${item%%$'\t'*}"
			title="${item#*$'\t'}"; title="${title%%$'\t'*}"
			detail="${item#*$'\t'*$'\t'}"
			[[ ${idx} -eq 0 ]] || printf ',\n'
			printf '    {"Level":"%s","Title":"%s","Detail":"%s"}' \
				"$(json_escape "${level}")" \
				"$(json_escape "${title}")" \
				"$(json_escape "${detail}")"
			idx=$((idx + 1))
		done
		printf '\n  ]\n'
		printf '}\n'
	} > "${REPORT_JSON_PATH}"

	add_result "INFO" "JSON 报告" "已写入: ${REPORT_JSON_PATH}"
}

write_summary() {
	write_section "检查汇总"
	write_result_line "INFO" "DebugBuildDir: ${DEBUG_BUILD_DIR}"
	write_result_line "INFO" "AnalyzeBuildDir: ${ANALYZE_BUILD_DIR}"
	write_result_line "INFO" "BaseRef: ${BASE_REF}"
	write_result_line "INFO" "降级运行: $( [[ ${DEGRADED} -eq 1 ]] && printf '是' || printf '否' )"
	write_result_line "INFO" "通过: ${RESULT_PASS}"
	write_result_line "INFO" "警告: ${RESULT_WARN}"
	write_result_line "INFO" "失败: ${RESULT_FAIL}"

	if [[ ${RESULT_WARN} -gt 0 ]]; then
		printf '\n警告清单：\n'
		local item level title detail
		for item in "${RESULT_ITEMS[@]}"; do
			level="${item%%$'\t'*}"
			[[ "${level}" == "WARN" ]] || continue
			title="${item#*$'\t'}"; title="${title%%$'\t'*}"
			detail="${item#*$'\t'*$'\t'}"
			write_result_line "WARN" "${title}: ${detail}"
		done
	fi

	if [[ ${RESULT_FAIL} -gt 0 ]]; then
		printf '\n失败清单：\n'
		local item level title detail
		for item in "${RESULT_ITEMS[@]}"; do
			level="${item%%$'\t'*}"
			[[ "${level}" == "FAIL" ]] || continue
			title="${item#*$'\t'}"; title="${title%%$'\t'*}"
			detail="${item#*$'\t'*$'\t'}"
			write_result_line "FAIL" "${title}: ${detail}"
		done
	fi
}

usage() {
	cat <<'EOF'
用法：
  strict-debug-check.sh [options]

选项：
  --debug-build-dir DIR
  --analyze-build-dir DIR
  --asan-build-dir DIR
  --base-ref REF
  --files FILE [FILE ...]
  --analyze-all
  --require-asan
  --skip-build
  --skip-tidy
  --skip-analyze
  --skip-asan
  --print-file-scope
  --report-json-path PATH
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--debug-build-dir) DEBUG_BUILD_DIR="$2"; shift 2 ;;
		--analyze-build-dir) ANALYZE_BUILD_DIR="$2"; shift 2 ;;
		--base-ref) BASE_REF="$2"; shift 2 ;;
		--files)
			INPUT_FILES_EXPLICIT=1
			shift
			while [[ $# -gt 0 && "$1" != --* ]]; do
				RAW_INPUT_FILES+=("$1")
				INPUT_FILES+=("$1")
				shift
			done
			;;
		--analyze-all) ANALYZE_ALL=1; shift ;;
		--skip-build) SKIP_BUILD=1; shift ;;
		--skip-tidy) SKIP_TIDY=1; shift ;;
		--skip-analyze) SKIP_ANALYZE=1; shift ;;
		--print-file-scope) PRINT_FILE_SCOPE=1; shift ;;
		--report-json-path) REPORT_JSON_PATH="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*)
			printf '未知参数: %s\n' "$1" >&2
			usage >&2
			exit 2
			;;
	esac
done

trap 'write_json_report; write_summary' EXIT

if [[ ! -f "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd" ]]; then
	add_result "FAIL" "入口前置检查" "缺少 CMake 包装脚本: ${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd"
	exit 1
fi
add_result "PASS" "入口前置检查" "已找到 qmclient_scripts/cmake-windows.cmd"

configure_git_command || exit 1

if ! CM_CMD="$(resolve_cmake_command)"; then
	add_result "FAIL" "入口前置检查" "PATH 中未找到 cmake"
	exit 1
fi

if [[ ${INPUT_FILES_EXPLICIT} -eq 0 ]]; then
	collect_default_scope
	INPUT_FILES=("${DEFAULT_SCOPE_SCOPED[@]}")
	INPUT_SCOPE_FILES=("${INPUT_FILES[@]}")
else
	add_result "INFO" "文件范围来源" "已显式传入 --files，共 ${#INPUT_FILES[@]} 个文件"
	INPUT_SCOPE_FILES=("${RAW_INPUT_FILES[@]}")
	for i in "${!INPUT_FILES[@]}"; do
		INPUT_FILES[$i]="$(normalize_input_file_arg "${INPUT_FILES[$i]}")"
	done
fi
EFFECTIVE_FILES=("${INPUT_FILES[@]}")

pushd "${REPO_ROOT}" >/dev/null

if [[ "${CM_CMD}" == "cmd.exe" ]]; then
	CM_SCRIPT_WIN="$(to_windows_path "${REPO_ROOT}/qmclient_scripts/cmake-windows.cmd")"
	invoke_configure_and_build "Debug CRT" "${DEBUG_BUILD_DIR}" 1 \
		"${CM_CMD}" /c "${CM_SCRIPT_WIN}" -G Ninja -S . -B "${DEBUG_BUILD_DIR}" \
		-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DQM_STRICT_WARNINGS=ON
else
	invoke_configure_and_build "Debug CRT" "${DEBUG_BUILD_DIR}" 1 \
		"${CM_CMD}" -G Ninja -S . -B "${DEBUG_BUILD_DIR}" \
		-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DQM_STRICT_WARNINGS=ON
fi

if [[ ${SKIP_ANALYZE} -eq 0 ]]; then
	if [[ "${CM_CMD}" == "cmd.exe" ]]; then
		if [[ ${ANALYZE_ALL} -eq 0 ]]; then
			mapfile -t ANALYZE_SOURCE_FILES < <(get_analyze_source_files "${INPUT_FILES[@]}")
			if [[ ${#ANALYZE_SOURCE_FILES[@]} -eq 0 ]]; then
				mark_degraded "MSVC /analyze 范围" "当前改动只有头文件或无可分析编译单元，/analyze 阶段已跳过"
			else
				add_result "INFO" "MSVC /analyze 范围" "仅扫描 ${#ANALYZE_SOURCE_FILES[@]} 个改动编译单元"
			fi
		else
			add_result "INFO" "MSVC /analyze 范围" "已显式传入 --analyze-all，将执行全量首方源码分析"
		fi

		if [[ ${ANALYZE_ALL} -eq 1 || ${#ANALYZE_SOURCE_FILES[@]} -gt 0 ]]; then
			ANALYZE_ARGS=(
				"${CM_CMD}" /c "${CM_SCRIPT_WIN}" -G Ninja -S . -B "${ANALYZE_BUILD_DIR}"
				-DCMAKE_BUILD_TYPE=Debug
				-DQM_MSVC_ANALYZE=ON -DQM_STRICT_WARNINGS=ON
			)
			invoke_configure_and_build "MSVC /analyze" "${ANALYZE_BUILD_DIR}" 1 "${ANALYZE_ARGS[@]}"
		fi
	else
		mark_degraded "MSVC /analyze" "当前不是 Windows/MSVC 环境，bash 版本不会伪造 /analyze；该阶段已按设计降级跳过"
	fi
else
	add_result "WARN" "MSVC /analyze" "已显式传入 --skip-analyze，跳过 /analyze 阶段"
fi

if [[ ${SKIP_TIDY} -eq 1 ]]; then
	add_result "WARN" "clang-tidy" "已显式传入 --skip-tidy，跳过 tidy 阶段"
	popd >/dev/null
	exit 0
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
	add_result "FAIL" "clang-tidy 前置检查" "PATH 中未找到 clang-tidy，无法执行严格 tidy 检查。"
	popd >/dev/null
	exit 1
fi
add_result "PASS" "clang-tidy 前置检查" "已找到 clang-tidy"

COMPILE_COMMANDS="${DEBUG_BUILD_DIR}/compile_commands.json"
if [[ ! -f "${COMPILE_COMMANDS}" ]]; then
	if [[ "${CM_CMD}" == "cmd.exe" ]]; then
		invoke_repo_command "Debug CRT 重新导出 compile_commands" 1 \
			"${CM_CMD}" /c "${CM_SCRIPT_WIN}" -G Ninja -S . -B "${DEBUG_BUILD_DIR}" \
			-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	else
		invoke_repo_command "Debug CRT 重新导出 compile_commands" 1 \
			"${CM_CMD}" -G Ninja -S . -B "${DEBUG_BUILD_DIR}" \
			-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	fi
fi

if [[ ! -f "${COMPILE_COMMANDS}" ]]; then
	add_result "FAIL" "compile_commands 检查" "重新配置后仍未生成 compile_commands.json: ${COMPILE_COMMANDS}"
	popd >/dev/null
	exit 1
fi
add_result "PASS" "compile_commands 检查" "compile_commands.json 已就绪"

CLANG_TIDY_FILES=()
for file in "${INPUT_FILES[@]}"; do
	[[ -z "${file}" ]] && continue
	file="$(normalize_input_file_arg "${file}")"
	if [[ -f "${file}" ]]; then
		CLANG_TIDY_FILES+=("${file}")
	elif [[ -f "${REPO_ROOT}/${file}" ]]; then
		CLANG_TIDY_FILES+=("${REPO_ROOT}/${file}")
	fi
done
mapfile -t EFFECTIVE_FILES < <(printf '%s\n' "${CLANG_TIDY_FILES[@]}" | unique_lines)

if [[ ${#EFFECTIVE_FILES[@]} -eq 0 ]]; then
	write_section "clang-tidy"
	printf '没有可供 clang-tidy 检查的 C/C++ 文件。\n'
	add_result "WARN" "clang-tidy" "当前没有可检查的 C/C++ 文件，tidy 阶段已跳过"
	popd >/dev/null
	exit 0
fi

	TIDY_CHECKS="-*,bugprone-assignment-in-if-condition,bugprone-dangling-handle,bugprone-inaccurate-erase,bugprone-misplaced-widening-cast,bugprone-stringview-nullptr,bugprone-suspicious-enum-usage,bugprone-unchecked-optional-access,bugprone-use-after-move,clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-nullability.*,modernize-use-override,performance-for-range-copy,performance-unnecessary-copy-initialization"
add_result "INFO" "clang-tidy 范围" "将对 ${#EFFECTIVE_FILES[@]} 个文件执行严格 tidy 检查"

for file in "${EFFECTIVE_FILES[@]}"; do
	invoke_repo_command "clang-tidy 严格检查: ${file}" 1 \
		clang-tidy "${file}" "-p=${DEBUG_BUILD_DIR}" \
		"--checks=${TIDY_CHECKS}" \
		"--extra-arg=-Qunused-arguments" "--quiet"
done

popd >/dev/null

if [[ ${RESULT_FAIL} -gt 0 ]]; then
	exit 1
fi

printf '\n严格调试检查完成。\n'
