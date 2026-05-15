param(
	[string]$BuildDir = "build-ninja",
	[string]$BaseRef = "main",
	[ValidateSet("quick", "default", "full")]
	[string]$Mode = "default",
	[switch]$AnalyzeAll,
	[switch]$SkipPreflight,
	[switch]$SkipConfigChecks,
	[switch]$SkipHeaderChecks,
	[switch]$SkipStyleCheck,
	[switch]$SkipStrictDebug,
	[switch]$SkipCxxTests,
	[switch]$RunRustTests,
	[switch]$RunAllTests,
	[switch]$IncludeIdentifierCheck,
	[switch]$IncludeUnusedHeaderCheck,
	[switch]$EnableClangFormatCheck,
	[switch]$EnableFullClangTidyWarn,
	[switch]$StrictEnvironment,
	[switch]$DryRun,
	[switch]$ExplainScope,
	[string]$ReportJsonPath = "",
	[string]$ScopeReportPath = ""
)

$ErrorActionPreference = "Stop"

# -----------------------------------------------------------------------------
# QmClient 仓库级检查总入口
#
# 设计目标：
# 1. 不替换仓库里现有的可信检查脚本，而是把它们按层串起来。
# 2. 默认尊重当前工作树 reality，避免因为历史脚本假设过硬而误杀。
# 3. 输出统一中文，便于直接作为人工检查报告阅读。
# 4. Windows 构建目录语义固定为：build-ninja=Release 运行目录，build-debug=调试目录，build-analyze=/analyze 专用目录，build-asan=实验目录。
#
# 分层模式：
# - quick   : 只做源码卫生层，适合快速自查
# - default : 源码卫生 + 严格构建静态分析 + C++ 测试，适合日常提交前
# - full    : default 基础上打开更重的可选检查，适合集中收口
# -----------------------------------------------------------------------------

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$CmakeWrapper = Join-Path $RepoRoot "qmclient_scripts\cmake-windows.cmd"
$StrictDebugCheck = Join-Path $RepoRoot "qmclient_scripts\strict-debug-check.ps1"
$ConfigCheck = Join-Path $RepoRoot "qmclient_scripts\check_config_variables.py"
$WorkflowDocCheck = Join-Path $RepoRoot "qmclient_scripts\check_workflow_docs.py"
$BaselineDebtAllowlistPath = Join-Path $RepoRoot "qmclient_scripts\baseline_debt_allowlist.json"
$HeaderGuardCheck = Join-Path $RepoRoot "scripts\check_header_guards.py"
$StandardHeaderCheck = Join-Path $RepoRoot "scripts\check_standard_headers.py"
$StyleCheck = Join-Path $RepoRoot "scripts\fix_style.py"
$IdentifierExtract = Join-Path $RepoRoot "scripts\extract_identifiers.py"
$IdentifierCheck = Join-Path $RepoRoot "scripts\check_identifiers.py"
$UnusedHeaderCheck = Join-Path $RepoRoot "scripts\check_unused_header_files.py"
$ClangFormatConfig = Join-Path $RepoRoot ".clang-format"
$ClangTidyConfig = Join-Path $RepoRoot ".clang-tidy"

$Script:ResultPass = 0
$Script:ResultWarn = 0
$Script:ResultFail = 0
$Script:ResultItems = New-Object System.Collections.Generic.List[object]
$Script:ModeSpec = $null
$Script:ScopedFilesCache = $null
$Script:ScopedSourceFilesCache = $null
$Script:ScopeDiagnosticsCache = $null
$Script:BaseRefAvailable = $true
$Script:BaseRefFailureReason = ""
$Script:PythonExecutable = ""
$Script:PythonBaseArguments = @()
$Script:PythonCommandText = ""
$Script:PythonResolutionAttempted = $false
$Script:PythonResolutionError = ""
$Script:BaselineDebtAllowlist = @()
$Script:PowerShellExecutable = ""

function Resolve-PowerShellExecutable {
	if(-not [string]::IsNullOrWhiteSpace($Script:PowerShellExecutable)) {
		return $Script:PowerShellExecutable
	}

	foreach($CandidateName in @("pwsh", "powershell")) {
		$Command = @(Get-Command $CandidateName -CommandType Application -All -ErrorAction SilentlyContinue | Select-Object -First 1)
		if($Command.Count -gt 0 -and $null -ne $Command[0] -and -not [string]::IsNullOrWhiteSpace($Command[0].Source)) {
			$Script:PowerShellExecutable = $Command[0].Source
			return $Script:PowerShellExecutable
		}
	}

	throw "未找到可用的 PowerShell 可执行文件（pwsh/powershell）"
}

function Get-NormalizedDetailHash {
	param([string]$Detail)

	$NormalizedText = (($Detail -replace "`r`n", "`n") -replace "`r", "`n")
	$TailMarker = "`n--- 原始尾部输出 ---`n"
	$TailIndex = $NormalizedText.IndexOf($TailMarker, [System.StringComparison]::Ordinal)
	if($TailIndex -ge 0) {
		$NormalizedText = $NormalizedText.Substring(0, $TailIndex)
	}

	$NormalizedLines = @(
		$NormalizedText.Split("`n") |
			ForEach-Object { $_.Trim() } |
			Where-Object { $_ } |
			Sort-Object -Unique
	)
	$Normalized = ($NormalizedLines -join "`n").Trim()
	$Sha256 = [System.Security.Cryptography.SHA256]::Create()
	try {
		$Bytes = [System.Text.Encoding]::UTF8.GetBytes($Normalized)
		$HashBytes = $Sha256.ComputeHash($Bytes)
		return ([System.BitConverter]::ToString($HashBytes)).Replace("-", "").ToLowerInvariant()
	} finally {
		$Sha256.Dispose()
	}
}

function Load-BaselineDebtAllowlist {
	if(!(Test-Path $BaselineDebtAllowlistPath)) {
		$Script:BaselineDebtAllowlist = @()
		return
	}

	try {
		$Json = Get-Content -LiteralPath $BaselineDebtAllowlistPath -Raw -Encoding UTF8 | ConvertFrom-Json
		$Entries = @()
		if($null -ne $Json.entries) {
			$Entries = @($Json.entries)
		}
		$Script:BaselineDebtAllowlist = $Entries
	} catch {
		Add-Result "WARN" "Baseline debt allowlist" ("读取失败，当前按空 allowlist 处理: {0}" -f $_.Exception.Message)
		$Script:BaselineDebtAllowlist = @()
	}
}

function Test-IsAllowlistedBaselineDebt {
	param(
		[string]$Title,
		[string]$Detail
	)

	$DetailHash = Get-NormalizedDetailHash $Detail
	foreach($Entry in $Script:BaselineDebtAllowlist) {
		if($Entry.title -eq $Title -and $Entry.detail_hash -eq $DetailHash) {
			return [pscustomobject]@{
				Matched = $true
				Reason = if($null -ne $Entry.reason -and $Entry.reason) { $Entry.reason } else { "known_baseline_debt" }
				DetailHash = $DetailHash
			}
		}
	}

	return [pscustomobject]@{
		Matched = $false
		Reason = ""
		DetailHash = $DetailHash
	}
}

function Get-ResultCategoryInfo {
	param(
		[string]$Level,
		[string]$Title,
		[string]$Detail
	)

	$Text = ("{0}`n{1}" -f $Title, $Detail)

	if($Text -match '环境前置检查|Python 前置检查|PATH 中未找到|未找到可用的 Python|WindowsApps|缺少必需路径|缺少 CMake 包装脚本|ddnet-libs|当前 worktree 的 DDNet\.exe 仍在运行|差异基线不可用|Found no clang-format|WinError 206|not a directory') {
		return [pscustomobject]@{
			Id = "environment"
			Label = "环境/工具"
		}
	}

	if($Title -match '配置变量使用检查|工作流文档一致性检查|头文件 guard 检查|标准头文件检查|代码格式干跑检查|标识符命名检查|未使用头文件检查|clang-format 附加检查' -or
		$Detail -match 'clang-format-violations|未使用配置项|头文件保护宏不正确|缺少头文件保护宏') {
		return [pscustomobject]@{
			Id = "baseline_debt"
			Label = "仓库基线债务"
		}
	}

	if($Title -match '严格构建与静态分析入口|Debug CRT|MSVC /analyze|AddressSanitizer|clang-tidy|CMake run_|测试目标|JSON 报告' -or
		$Detail -match 'build-debug|build-analyze|run_cxx_tests|run_rust_tests|run_tests|compile_commands|仓库级检查存在失败项') {
		return [pscustomobject]@{
			Id = "active_blocker"
			Label = "当前改动/构建阻断"
		}
	}

	return [pscustomobject]@{
		Id = "general"
		Label = "一般项"
	}
}

function Add-Result {
	param(
		[ValidateSet("PASS", "WARN", "FAIL", "INFO")]
		[string]$Level,
		[string]$Title,
		[string]$Detail
	)

	$Category = Get-ResultCategoryInfo $Level $Title $Detail
	$StoredLevel = $Level
	$StoredDetail = $Detail
	$AllowlistReason = ""
	$DetailHash = ""

	if($Level -eq "FAIL" -and $Category.Id -eq "baseline_debt") {
		$AllowlistMatch = Test-IsAllowlistedBaselineDebt -Title $Title -Detail $Detail
		$DetailHash = $AllowlistMatch.DetailHash
		if($AllowlistMatch.Matched) {
			$StoredLevel = "WARN"
			$StoredDetail = ("已按 baseline allowlist 降级为 WARN，reason={0}, detail_hash={1}`n{2}" -f $AllowlistMatch.Reason, $AllowlistMatch.DetailHash, $Detail)
			$AllowlistReason = $AllowlistMatch.Reason
		}
	} elseif($Category.Id -eq "baseline_debt") {
		$DetailHash = Get-NormalizedDetailHash $Detail
	}

	switch($StoredLevel) {
		"PASS" { $Script:ResultPass++ }
		"WARN" { $Script:ResultWarn++ }
		"FAIL" { $Script:ResultFail++ }
	}

	$ResultItem = [pscustomobject]@{
		Level = $StoredLevel
		OriginalLevel = $Level
		Title = $Title
		Detail = $StoredDetail
		CategoryId = $Category.Id
		CategoryLabel = $Category.Label
		AllowlistReason = $AllowlistReason
		DetailHash = $DetailHash
	}
	$Script:ResultItems.Add($ResultItem) | Out-Null
}

function Get-LatestResultItem {
	if($Script:ResultItems.Count -le 0) {
		return $null
	}

	return $Script:ResultItems[$Script:ResultItems.Count - 1]
}

function Write-Section {
	param([string]$Title)
	Write-Host ""
	Write-Host "==> $Title"
}

function Write-ResultLine {
	param(
		[ValidateSet("PASS", "WARN", "FAIL", "INFO")]
		[string]$Level,
		[string]$Message
	)

	Write-Host ("[{0}] {1}" -f $Level, $Message)
}

function Get-CommandFailureSummary {
	param(
		[string]$Title,
		[string[]]$OutputLines,
		[int]$ExitCode
	)

	$TailLines = @(
		$OutputLines |
			Where-Object { $_ } |
			Select-Object -Last 12
	)
	$RelevantLines = @(
		$OutputLines |
			Where-Object { $_ } |
			Where-Object {
				$_ -match '错误|error|Error|fatal|Traceback|WinError|Found no clang-format|not a directory|未找到|失败|FAILED|Exception'
			} |
			Select-Object -Last 12
	)
	if($RelevantLines.Count -eq 0) {
		$RelevantLines = @(
			$OutputLines |
				Where-Object { $_ } |
				Select-Object -Last 12
		)
	}

	$Summary = "$Title 失败，退出码 $ExitCode"
	if($RelevantLines.Count -gt 0) {
		$Summary = "$Summary`n$($RelevantLines -join "`n")"
	}
	if($TailLines.Count -gt 0 -and (($RelevantLines -join "`n") -ne ($TailLines -join "`n"))) {
		$Summary = "$Summary`n--- 原始尾部输出 ---`n$($TailLines -join "`n")"
	}
	return $Summary
}

function Write-CommandOutputLines {
	param(
		[string[]]$OutputLines,
		[int]$MaxLines = 120,
		[int]$HeadLines = 40,
		[int]$TailLines = 40
	)

	$Lines = @($OutputLines | Where-Object { $null -ne $_ })
	if($Lines.Count -le $MaxLines) {
		foreach($Line in $Lines) {
			Write-Host $Line
		}
		return
	}

	$Head = @($Lines | Select-Object -First $HeadLines)
	$Tail = @($Lines | Select-Object -Last $TailLines)
	foreach($Line in $Head) {
		Write-Host $Line
	}

	$OmittedCount = [Math]::Max(0, $Lines.Count - $Head.Count - $Tail.Count)
	Write-Host ("... 已省略 {0} 行中间输出，完整细节请看失败摘要或 JSON 报告 ..." -f $OmittedCount)

	foreach($Line in $Tail) {
		Write-Host $Line
	}
}

function Invoke-ProcessWithCapturedOutput {
	param(
		[string]$FilePath,
		[string[]]$Arguments = @(),
		[string]$WorkingDirectory = $RepoRoot
	)

	$PowerShellExecutable = Resolve-PowerShellExecutable
	$QuotedArguments = @($Arguments | ForEach-Object { Convert-ToPowerShellSingleQuotedLiteral $_ }) -join ', '
	$WrappedCommand = @(
		'$ErrorActionPreference = ''Continue'''
		('Set-Location {0}' -f (Convert-ToPowerShellSingleQuotedLiteral $WorkingDirectory))
		('$argsList = @({0})' -f $QuotedArguments)
		('& {0} @argsList 2>&1 | ForEach-Object {{ $_.ToString() }}' -f (Convert-ToPowerShellSingleQuotedLiteral $FilePath))
		'exit $LASTEXITCODE'
	) -join '; '

	$StdOutPath = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), [System.IO.Path]::GetRandomFileName())
	$StdErrPath = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), [System.IO.Path]::GetRandomFileName())
	try {
		$Process = Start-Process -FilePath $PowerShellExecutable `
			-ArgumentList @("-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", $WrappedCommand) `
			-WorkingDirectory $RepoRoot `
			-NoNewWindow `
			-Wait `
			-PassThru `
			-RedirectStandardOutput $StdOutPath `
			-RedirectStandardError $StdErrPath

		$OutputLines = New-Object System.Collections.Generic.List[string]
		foreach($CapturedPath in @($StdOutPath, $StdErrPath)) {
			if(Test-Path $CapturedPath) {
				$ReadCapturedLines = {
					param([System.Text.Encoding]$Encoding)

					$Lines = New-Object System.Collections.Generic.List[string]
					$Reader = $null
					try {
						$Reader = New-Object System.IO.StreamReader($CapturedPath, $Encoding, $true)
						while(-not $Reader.EndOfStream) {
							$Lines.Add($Reader.ReadLine()) | Out-Null
						}
					} finally {
						if($null -ne $Reader) {
							$Reader.Dispose()
						}
					}
					return @($Lines.ToArray())
				}

				try {
					$Utf8Strict = [System.Text.UTF8Encoding]::new($false, $true)
					foreach($Line in (& $ReadCapturedLines $Utf8Strict)) {
						$OutputLines.Add($Line) | Out-Null
					}
				} catch [System.Text.DecoderFallbackException] {
					foreach($Line in (& $ReadCapturedLines ([System.Text.Encoding]::Default))) {
						$OutputLines.Add($Line) | Out-Null
					}
				}
			}
		}

		return [pscustomobject]@{
			ExitCode = $Process.ExitCode
			OutputLines = @($OutputLines.ToArray())
		}
	} finally {
		foreach($CapturedPath in @($StdOutPath, $StdErrPath)) {
			if(Test-Path $CapturedPath) {
				Remove-Item -LiteralPath $CapturedPath -Force -ErrorAction SilentlyContinue
			}
		}
	}
}

function Test-IsWindowsAppsPythonPath {
	param([string]$PathValue)

	if([string]::IsNullOrWhiteSpace($PathValue)) {
		return $false
	}

	$Normalized = $PathValue -replace '/', '\'
	return $Normalized -like '*\AppData\Local\Microsoft\WindowsApps\python.exe'
}

function Resolve-PythonCommand {
	if($Script:PythonResolutionAttempted) {
		if(-not [string]::IsNullOrWhiteSpace($Script:PythonExecutable)) {
			return [pscustomobject]@{
				Executable = $Script:PythonExecutable
				BaseArguments = @($Script:PythonBaseArguments)
				CommandText = $Script:PythonCommandText
			}
		}
		throw $Script:PythonResolutionError
	}

	$Script:PythonResolutionAttempted = $true
	if(-not [string]::IsNullOrWhiteSpace($Script:PythonExecutable)) {
		return [pscustomobject]@{
			Executable = $Script:PythonExecutable
			BaseArguments = @($Script:PythonBaseArguments)
			CommandText = $Script:PythonCommandText
		}
	}

	$Candidates = New-Object System.Collections.Generic.List[object]
	$SeenKeys = New-Object System.Collections.Generic.HashSet[string]

	$AddCandidate = {
		param(
			[string]$Executable,
			[string[]]$BaseArguments,
			[string]$CommandText
		)

		if([string]::IsNullOrWhiteSpace($Executable)) {
			return
		}
		if(!(Test-Path $Executable)) {
			return
		}
		if(Test-IsWindowsAppsPythonPath $Executable) {
			return
		}

		$Key = ("{0}|{1}" -f $Executable.ToLowerInvariant(), ($BaseArguments -join ' ')).Trim()
		if($SeenKeys.Add($Key)) {
			$Candidates.Add([pscustomobject]@{
				Executable = $Executable
				BaseArguments = @($BaseArguments)
				CommandText = $CommandText
			}) | Out-Null
		}
	}

	@(Get-Command py -CommandType Application -All -ErrorAction SilentlyContinue) | ForEach-Object {
		& $AddCandidate $_.Source @("-3") "py -3"
	}

	@(Get-Command python -CommandType Application -All -ErrorAction SilentlyContinue) | ForEach-Object {
		& $AddCandidate $_.Source @() "python"
	}

	foreach($Candidate in $Candidates.ToArray()) {
		try {
			& $Candidate.Executable @($Candidate.BaseArguments + @("--version")) *> $null
			if($LASTEXITCODE -eq 0) {
				$Script:PythonExecutable = $Candidate.Executable
				$Script:PythonBaseArguments = @($Candidate.BaseArguments)
				$Script:PythonCommandText = $Candidate.CommandText
				$Script:PythonResolutionError = ""
				return $Candidate
			}
		} catch {
			continue
		}
	}

	$Detail = "未找到可用的 Python 解释器。已跳过 WindowsApps 别名，请确认 py -3 或真实 python.exe 可从当前无 profile 会话访问。"
	$Script:PythonResolutionError = $Detail
	throw $Detail
}

function Get-PythonInvocationText {
	$Python = Resolve-PythonCommand
	return $Python.CommandText
}

function Convert-ToPowerShellSingleQuotedLiteral {
	param([string]$Value)

	return "'" + ($Value -replace "'", "''") + "'"
}

function Invoke-PythonRepoCommand {
	param(
		[string]$Title,
		[string[]]$Arguments = @(),
		[string]$WorkingDirectory = $RepoRoot
	)

	$Python = Resolve-PythonCommand
	Invoke-RepoCommand $Title $Python.Executable @($Python.BaseArguments + $Arguments) $WorkingDirectory
}

function Test-PythonReadyForStage {
	param([string]$Title)

	if($Script:PythonResolutionAttempted -and [string]::IsNullOrWhiteSpace($Script:PythonExecutable) -and -not [string]::IsNullOrWhiteSpace($Script:PythonResolutionError)) {
		Add-Result "WARN" $Title ("Python 前置检查失败，当前阶段已跳过：{0}" -f $Script:PythonResolutionError)
		return $false
	}

	return $true
}

function Get-ModeSpecification {
	switch($Mode) {
		"quick" {
			return [pscustomobject]@{
				Name = "quick"
				Target = "开发期快速自查"
				Expectation = "通常应在数分钟内完成，只扫源码卫生层。"
				BlockingRule = "只阻断明显的脚本/规范问题，不做真实构建与测试。"
			}
		}
		"default" {
			return [pscustomobject]@{
				Name = "default"
				Target = "日常提交前严格门"
				Expectation = "需要真实构建、严格静态分析和 C++ 测试。"
				BlockingRule = "构建、静态分析、测试任一失败都应阻断。"
			}
		}
		"full" {
			return [pscustomobject]@{
				Name = "full"
				Target = "集中收口 / 准发布门"
				Expectation = "在 default 基础上增加更重的附加检查与 Rust 测试。"
				BlockingRule = "默认阻断 default 层和 full 的硬失败项；高噪音附加检查先以 WARN 方式试跑。"
			}
		}
	}
}

function Resolve-ModeToggles {
	$Script:ModeSpec = Get-ModeSpecification
	switch($Mode) {
		"quick" {
			$Script:ModeRunStrictDebug = $false
			$Script:ModeRunCxxTests = $false
			$Script:ModeRunRustTests = $false
			$Script:ModeRunIdentifierCheck = $false
			$Script:ModeRunUnusedHeaderCheck = $false
		}
		"default" {
			$Script:ModeRunStrictDebug = $true
			$Script:ModeRunCxxTests = $true
			$Script:ModeRunRustTests = $false
			$Script:ModeRunIdentifierCheck = $false
			$Script:ModeRunUnusedHeaderCheck = $false
		}
		"full" {
			$Script:ModeRunStrictDebug = $true
			$Script:ModeRunCxxTests = $true
			$Script:ModeRunRustTests = $true
			$Script:ModeRunIdentifierCheck = $true
			$Script:ModeRunUnusedHeaderCheck = $false
		}
	}

	if($RunRustTests) {
		$Script:ModeRunRustTests = $true
	}
	if($IncludeIdentifierCheck) {
		$Script:ModeRunIdentifierCheck = $true
	}
	if($IncludeUnusedHeaderCheck) {
		$Script:ModeRunUnusedHeaderCheck = $true
	}
	$Script:ModeRunClangFormatCheck = $EnableClangFormatCheck
	$Script:ModeRunFullClangTidyWarn = $EnableFullClangTidyWarn
	if($RunAllTests) {
		$Script:ModeRunCxxTests = $false
		$Script:ModeRunRustTests = $false
	}
}

function Invoke-RepoCommand {
	param(
		[string]$Title,
		[string]$FilePath,
		[string[]]$Arguments = @(),
		[string]$WorkingDirectory = $RepoRoot
	)

	Write-Section $Title
	$CommandText = "$FilePath $($Arguments -join ' ')".Trim()
	Write-Host "命令: $CommandText"
	if($DryRun) {
		Add-Result "INFO" $Title "DryRun，仅展示命令"
		return
	}

	Push-Location $WorkingDirectory
	try {
		$CommandResult = Invoke-ProcessWithCapturedOutput -FilePath $FilePath -Arguments $Arguments -WorkingDirectory $WorkingDirectory
		$OutputLines = @($CommandResult.OutputLines)
		Write-CommandOutputLines -OutputLines $OutputLines
		if($CommandResult.ExitCode -ne 0) {
			$Message = Get-CommandFailureSummary $Title $OutputLines $CommandResult.ExitCode
			Add-Result "FAIL" $Title $Message
			$ResultItem = Get-LatestResultItem
			if($ResultItem.Level -eq "FAIL") {
				throw $Message
			}
			return
		}
		Add-Result "PASS" $Title "执行通过"
	} finally {
		Pop-Location
	}
}

function Invoke-RepoCommandAsWarning {
	param(
		[string]$Title,
		[string]$FilePath,
		[string[]]$Arguments = @(),
		[string]$WorkingDirectory = $RepoRoot
	)

	Write-Section $Title
	$CommandText = "$FilePath $($Arguments -join ' ')".Trim()
	Write-Host "命令: $CommandText"
	if($DryRun) {
		Add-Result "INFO" $Title "DryRun，仅展示命令"
		return
	}

	Push-Location $WorkingDirectory
	try {
		$CommandResult = Invoke-ProcessWithCapturedOutput -FilePath $FilePath -Arguments $Arguments -WorkingDirectory $WorkingDirectory
		$OutputLines = @($CommandResult.OutputLines)
		Write-CommandOutputLines -OutputLines $OutputLines
		if($CommandResult.ExitCode -ne 0) {
			Write-Warning "$Title 失败，当前按 WARN 记录"
			$Message = Get-CommandFailureSummary $Title $OutputLines $CommandResult.ExitCode
			Add-Result "WARN" $Title $Message
			return
		}
		Add-Result "PASS" $Title "执行通过"
	} finally {
		Pop-Location
	}
}

function Invoke-GateStep {
	param(
		[string]$Title,
		[scriptblock]$Action
	)

	$FailCountBefore = @($Script:ResultItems | Where-Object { $_.Level -eq "FAIL" }).Count
	try {
		& $Action
	} catch {
		$FailCountAfter = @($Script:ResultItems | Where-Object { $_.Level -eq "FAIL" }).Count
		if($FailCountAfter -le $FailCountBefore) {
			$Message = $_.Exception.Message
			if([string]::IsNullOrWhiteSpace($Message)) {
				$Message = "$Title 执行失败"
			}
			Add-Result "FAIL" $Title $Message
		}
		Write-Warning "$Title 失败，已记录并继续后续检查"
	}
}

function Test-RequiredPath {
	param([string]$PathValue)
	if(!(Test-Path $PathValue)) {
		throw "缺少必需路径: $PathValue"
	}
}

function Report-EnvironmentMessage {
	param(
		[string]$Message,
		[switch]$AsError
	)

	if($AsError) {
		Add-Result "FAIL" "环境前置检查" $Message
		throw $Message
	}

	Write-Warning $Message
	Add-Result "WARN" "环境前置检查" $Message
}

function Test-RequiresBuildEnvironment {
	if($RunAllTests) {
		return $true
	}

	if($Script:ModeRunStrictDebug -and -not $SkipStrictDebug) {
		return $true
	}

	if($Script:ModeRunCxxTests -and -not $SkipCxxTests) {
		return $true
	}

	if($Script:ModeRunRustTests) {
		return $true
	}

	return $false
}

function Get-BranchDiffFiles {
	$MergeBaseOutput = @(& git -C $RepoRoot merge-base $BaseRef HEAD 2>&1)
	$MergeBaseExitCode = $LASTEXITCODE
	$MergeBase = ($MergeBaseOutput | Select-Object -First 1).ToString().Trim()
	if($MergeBaseExitCode -ne 0) {
		$Script:BaseRefAvailable = $false
		$Script:BaseRefFailureReason = (($MergeBaseOutput | ForEach-Object { $_.ToString().Trim() } | Where-Object { $_ }) -join " | ")
		return @()
	}
	if([string]::IsNullOrWhiteSpace($MergeBase)) {
		$Script:BaseRefAvailable = $false
		$Script:BaseRefFailureReason = "git merge-base 返回空结果"
		return @()
	}

	$Script:BaseRefAvailable = $true
	$Script:BaseRefFailureReason = ""
	$Files = @(
		& git -C $RepoRoot -c core.safecrlf=false diff --name-only --diff-filter=ACMR "$MergeBase...HEAD" -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>$null |
			ForEach-Object { $_.ToString().Trim() } |
			Where-Object { $_ }
	)
	return @($Files | Sort-Object -Unique)
}

function Get-WorkingTreeFiles {
	$Buckets = Get-WorkingTreeFileBuckets
	return @($Buckets.Unstaged + $Buckets.Staged + $Buckets.Untracked | Sort-Object -Unique)
}

function Get-WorkingTreeFileBuckets {
	$UnstagedFiles = @(
		& git -C $RepoRoot -c core.safecrlf=false diff --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>$null |
			ForEach-Object { $_.ToString().Trim() } |
			Where-Object { $_ }
	)
	$StagedFiles = @(
		& git -C $RepoRoot -c core.safecrlf=false diff --cached --name-only --diff-filter=ACMR -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>$null |
			ForEach-Object { $_.ToString().Trim() } |
			Where-Object { $_ }
	)
	$UntrackedFiles = @(
		& git -C $RepoRoot ls-files --others --exclude-standard -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>$null |
			ForEach-Object { $_.ToString().Trim() } |
			Where-Object { $_ }
	)
	return [pscustomobject]@{
		Unstaged = @($UnstagedFiles | Sort-Object -Unique)
		Staged = @($StagedFiles | Sort-Object -Unique)
		Untracked = @($UntrackedFiles | Sort-Object -Unique)
	}
}

function Test-IsScopedFirstPartyFile {
	param([string]$RelativePath)

	$Normalized = $RelativePath -replace '\\', '/'
	if($Normalized -notmatch '^src/.+\.(c|cc|cpp|h|hpp)$') {
		return $false
	}

	$ExcludedPrefixes = @(
		"src/engine/external/",
		"src/game/generated/",
		"src/rust-bridge/base/"
	)
	foreach($Prefix in $ExcludedPrefixes) {
		if($Normalized.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
			return $false
		}
	}

	return $true
}

function Get-ScopedFirstPartyFiles {
	if($null -ne $Script:ScopedFilesCache) {
		return $Script:ScopedFilesCache
	}

	$BranchFiles = @(Get-BranchDiffFiles)
	$WorkingTreeFiles = @(Get-WorkingTreeFiles)
	$AllFiles = @($BranchFiles + $WorkingTreeFiles | Sort-Object -Unique)
	$ScopedFiles = @(
		$AllFiles |
			Where-Object { Test-IsScopedFirstPartyFile $_ } |
			ForEach-Object { $_ -replace '\\', '/' } |
			Sort-Object -Unique
	)
	$Script:ScopedFilesCache = $ScopedFiles
	return $ScopedFiles
}

function Get-ScopeDiagnostics {
	if($null -ne $Script:ScopeDiagnosticsCache) {
		return $Script:ScopeDiagnosticsCache
	}

	$BranchFiles = @(Get-BranchDiffFiles)
	$WorkingTreeBuckets = Get-WorkingTreeFileBuckets
	$AllFiles = @($BranchFiles + $WorkingTreeBuckets.Unstaged + $WorkingTreeBuckets.Staged + $WorkingTreeBuckets.Untracked | Sort-Object -Unique)
	$IncludedFiles = @(Get-ScopedFirstPartyFiles)
	$ExcludedFiles = New-Object System.Collections.Generic.List[object]
	foreach($File in @($AllFiles | Sort-Object -Unique)) {
		if($File -in $IncludedFiles) {
			continue
		}

		$Normalized = $File -replace '\\', '/'
		$Reason = "not-in-src"
		if($Normalized -match '^src/engine/external/') {
			$Reason = "third-party-external"
		} elseif($Normalized -match '^src/game/generated/') {
			$Reason = "generated"
		} elseif($Normalized -match '^src/rust-bridge/base/') {
			$Reason = "rust-bridge-base"
		} elseif($Normalized -notmatch '^src/') {
			$Reason = "not-in-src"
		} elseif($Normalized -notmatch '\.(c|cc|cpp|h|hpp)$') {
			$Reason = "extension-not-supported"
		}

		$ExcludedFiles.Add([pscustomobject]@{
			Path = $Normalized
			Reason = $Reason
		}) | Out-Null
	}

	$ExcludedFileItems = @($ExcludedFiles | ForEach-Object { $_ })
	$BranchFileItems = @($BranchFiles | ForEach-Object { $_ })
	$IncludedFileItems = @($IncludedFiles | ForEach-Object { $_ })

	$Script:ScopeDiagnosticsCache = [pscustomobject]@{
		BaseRef = $BaseRef
		BaseRefAvailable = $Script:BaseRefAvailable
		BaseRefFailureReason = $Script:BaseRefFailureReason
		BranchFiles = $BranchFileItems
		UnstagedFiles = @($WorkingTreeBuckets.Unstaged)
		StagedFiles = @($WorkingTreeBuckets.Staged)
		UntrackedFiles = @($WorkingTreeBuckets.Untracked)
		IncludedFiles = $IncludedFileItems
		ExcludedFiles = $ExcludedFileItems
	}
	return $Script:ScopeDiagnosticsCache
}

function Write-ScopeDiagnostics {
	$Diagnostics = Get-ScopeDiagnostics
	Add-Result "INFO" "差异范围统计" ("branch={0}, unstaged={1}, staged={2}, untracked={3}, included={4}, excluded={5}" -f
		$Diagnostics.BranchFiles.Count,
		$Diagnostics.UnstagedFiles.Count,
		$Diagnostics.StagedFiles.Count,
		$Diagnostics.UntrackedFiles.Count,
		$Diagnostics.IncludedFiles.Count,
		$Diagnostics.ExcludedFiles.Count)
	if(-not $Diagnostics.BaseRefAvailable) {
		$BaseRefMessage = "差异基线不可用: $BaseRef"
		if(-not [string]::IsNullOrWhiteSpace($Diagnostics.BaseRefFailureReason)) {
			$BaseRefMessage = "{0} ({1})" -f $BaseRefMessage, $Diagnostics.BaseRefFailureReason
		}
		if((Test-RequiresBuildEnvironment) -and -not $DryRun) {
			Add-Result "FAIL" "差异基线检查" $BaseRefMessage
			throw $BaseRefMessage
		}
		Add-Result "WARN" "差异基线检查" $BaseRefMessage
	}

	if($ExplainScope) {
		Write-Section "差异范围说明"
		Write-Host ("BaseRef: {0}" -f $Diagnostics.BaseRef)
		Write-Host ("BaseRef 可用: {0}" -f $Diagnostics.BaseRefAvailable)
		if(-not $Diagnostics.BaseRefAvailable -and -not [string]::IsNullOrWhiteSpace($Diagnostics.BaseRefFailureReason)) {
			Write-Host ("BaseRef 失败原因: {0}" -f $Diagnostics.BaseRefFailureReason)
		}
		Write-Host ("分支差异文件数: {0}" -f $Diagnostics.BranchFiles.Count)
		Write-Host ("未暂存文件数: {0}" -f $Diagnostics.UnstagedFiles.Count)
		Write-Host ("已暂存文件数: {0}" -f $Diagnostics.StagedFiles.Count)
		Write-Host ("未跟踪文件数: {0}" -f $Diagnostics.UntrackedFiles.Count)
		Write-Host ("纳入首方范围文件数: {0}" -f $Diagnostics.IncludedFiles.Count)
		Write-Host ("排除文件数: {0}" -f $Diagnostics.ExcludedFiles.Count)
	}

	if(-not [string]::IsNullOrWhiteSpace($ScopeReportPath)) {
		$ParentDir = Split-Path -Parent $ScopeReportPath
		if(-not [string]::IsNullOrWhiteSpace($ParentDir) -and !(Test-Path $ParentDir)) {
			New-Item -ItemType Directory -Path $ParentDir -Force | Out-Null
		}
		$Diagnostics | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ScopeReportPath -Encoding UTF8
		Add-Result "INFO" "差异范围报告" ("已写入: {0}" -f $ScopeReportPath)
	}
}

function Get-ScopedFirstPartySourceFiles {
	if($null -ne $Script:ScopedSourceFilesCache) {
		return $Script:ScopedSourceFilesCache
	}

	$SourceFiles = @(
		Get-ScopedFirstPartyFiles |
			Where-Object { $_ -match '\.(c|cc|cpp)$' } |
			Sort-Object -Unique
	)
	$Script:ScopedSourceFilesCache = $SourceFiles
	return $SourceFiles
}

function Get-StrictDebugFiles {
	return @(
		Get-ScopedFirstPartyFiles |
			Where-Object { Test-Path (Join-Path $RepoRoot $_) } |
			Sort-Object -Unique
	)
}

function Assert-WorkingTreePreflight {
	Write-Section "环境前置检查"

	Test-RequiredPath $CmakeWrapper
	Test-RequiredPath $StrictDebugCheck
	Test-RequiredPath $ConfigCheck
	Test-RequiredPath $WorkflowDocCheck
	Test-RequiredPath $HeaderGuardCheck
	Test-RequiredPath $StandardHeaderCheck
	Test-RequiredPath $StyleCheck
	Add-Result "PASS" "脚本入口存在性" "核心检查脚本均已找到"

	if(Test-Path $BaselineDebtAllowlistPath) {
		Add-Result "PASS" "Baseline debt allowlist" "已找到 baseline allowlist 文件"
	} else {
		Add-Result "WARN" "Baseline debt allowlist" ("未找到 {0}，当前按空 allowlist 继续；如需初始化，请先生成 report 再运行 refresh_baseline_debt_allowlist.py" -f $BaselineDebtAllowlistPath)
	}

	$Python = Resolve-PythonCommand
	Add-Result "PASS" "Python 前置检查" ("已解析到 {0} ({1})" -f $Python.CommandText, $Python.Executable)

	$DdnetLibs = Join-Path $RepoRoot "ddnet-libs"
	if(!(Test-Path $DdnetLibs)) {
		$Message = "未找到 ddnet-libs/。正式构建前必须先确认子模块或依赖目录是否已准备好。"
		if((Test-RequiresBuildEnvironment) -and -not $DryRun) {
			Report-EnvironmentMessage $Message -AsError
		} elseif($StrictEnvironment -and -not $DryRun) {
			Report-EnvironmentMessage $Message -AsError
		} else {
			Report-EnvironmentMessage $Message
		}
	} else {
		Add-Result "PASS" "依赖目录检查" "ddnet-libs/ 已存在"
	}

	$CurrentBuildExe = Join-Path $RepoRoot "$BuildDir\DDNet.exe"
	$RunningCurrentBuild = @(
		Get-Process -Name "DDNet" -ErrorAction SilentlyContinue | Where-Object {
			try {
				$_.Path -and ([System.IO.Path]::GetFullPath($_.Path) -eq [System.IO.Path]::GetFullPath($CurrentBuildExe))
			} catch {
				$false
			}
		}
	)
	if($RunningCurrentBuild.Count -gt 0) {
		$Message = "当前 worktree 的 DDNet.exe 仍在运行：$CurrentBuildExe"
		if((Test-RequiresBuildEnvironment) -and -not $DryRun) {
			Report-EnvironmentMessage $Message -AsError
		} elseif($StrictEnvironment -and -not $DryRun) {
			Report-EnvironmentMessage $Message -AsError
		} else {
			Report-EnvironmentMessage $Message
		}
	} else {
		Add-Result "PASS" "运行中进程检查" "当前 worktree 的 DDNet.exe 未占用构建输出"
	}

	$Branch = (& git -C $RepoRoot branch --show-current).Trim()
	$Status = (& git -C $RepoRoot status --short --branch)
	Write-Host "当前分支: $Branch"
	$Status | ForEach-Object { Write-Host $_ }
	Add-Result "INFO" "工作树状态" ("当前分支: {0}" -f $Branch)
	Add-Result "INFO" "模式定义" ("{0} | {1} | {2}" -f $Script:ModeSpec.Target, $Script:ModeSpec.Expectation, $Script:ModeSpec.BlockingRule)
	Add-Result "INFO" "差异基线" ("当前使用的分支比较基线: {0}" -f $BaseRef)
}

function Invoke-ConfigChecks {
	Invoke-PythonRepoCommand "配置变量使用检查（Qm/Tc/栖梦）" @($ConfigCheck, "--qm")
}

function Invoke-WorkflowDocChecks {
	if(!(Test-PythonReadyForStage "工作流文档一致性检查")) {
		return
	}
	Invoke-PythonRepoCommand "工作流文档一致性检查" @($WorkflowDocCheck)
}

function Invoke-HeaderChecks {
	if(!(Test-PythonReadyForStage "头文件检查")) {
		return
	}
	Invoke-GateStep "头文件 guard 检查" {
		Invoke-PythonRepoCommand "头文件 guard 检查" @($HeaderGuardCheck)
	}
	Invoke-GateStep "标准头文件检查" {
		Invoke-PythonRepoCommand "标准头文件检查" @($StandardHeaderCheck)
	}
}

function Invoke-StyleChecks {
	if(!(Test-PythonReadyForStage "代码格式检查")) {
		return
	}
	$ScopedFiles = @(
		Get-ScopedFirstPartyFiles |
			Where-Object { Test-Path (Join-Path $RepoRoot $_) } |
			Sort-Object -Unique
	)
	if($ScopedFiles.Count -eq 0) {
		Add-Result "WARN" "代码格式检查" "未收集到改动范围内可供 fix_style.py 检查的首方 C/C++ 文件"
		return
	}

	Add-Result "INFO" "代码格式检查范围" ("按收敛后的首方源码范围传入 {0} 个文件" -f $ScopedFiles.Count)
	$StyleArguments = @($StyleCheck, "-n") + $ScopedFiles
	Invoke-PythonRepoCommand "代码格式干跑检查" $StyleArguments
}

function Invoke-IdentifierChecks {
	if(!(Test-PythonReadyForStage "标识符命名检查")) {
		return
	}
	Write-Section "标识符命名检查"
	$PythonCommandText = Get-PythonInvocationText
	$PipelineText = "$PythonCommandText <extract_identifiers.py> <源文件列表> | $PythonCommandText <check_identifiers.py>"
	Write-Host "命令: $PipelineText"
	if($DryRun) {
		Add-Result "INFO" "标识符命名检查" "DryRun，仅展示命令"
		return
	}

	Push-Location $RepoRoot
	try {
		$SourceFiles = @(
			Get-ScopedFirstPartySourceFiles |
				ForEach-Object { Join-Path $RepoRoot $_ }
		)
		if($SourceFiles.Count -eq 0) {
			Add-Result "WARN" "标识符命名检查" "未找到改动范围内可供 extract_identifiers.py 分析的首方源文件"
			return
		}

		$ExtractArgs = @($IdentifierExtract) + $SourceFiles
		$Python = Resolve-PythonCommand
		$ExtractOutput = & $Python.Executable @($Python.BaseArguments + $ExtractArgs)
		if($LASTEXITCODE -ne 0) {
			throw "extract_identifiers.py 执行失败，退出码 $LASTEXITCODE"
		}

		$TmpFile = [System.IO.Path]::GetTempFileName()
		try {
			Set-Content -LiteralPath $TmpFile -Value $ExtractOutput -Encoding UTF8
			Get-Content -LiteralPath $TmpFile | & $Python.Executable @($Python.BaseArguments + @($IdentifierCheck))
			if($LASTEXITCODE -ne 0) {
				throw "check_identifiers.py 执行失败，退出码 $LASTEXITCODE"
			}
			Add-Result "PASS" "标识符命名检查" "命名风格检查通过"
		} finally {
			Remove-Item -LiteralPath $TmpFile -ErrorAction SilentlyContinue
		}
	} finally {
		Pop-Location
	}
}

function Invoke-UnusedHeaderChecks {
	if(!(Test-PythonReadyForStage "未使用头文件检查")) {
		return
	}
	Invoke-PythonRepoCommand "未使用头文件检查" @($UnusedHeaderCheck)
}

function Invoke-ClangFormatChecks {
	$ClangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
	if($null -eq $ClangFormat) {
		Add-Result "WARN" "clang-format 附加检查" "PATH 中未找到 clang-format，已跳过"
		return
	}
	if(!(Test-Path $ClangFormatConfig)) {
		Add-Result "WARN" "clang-format 附加检查" "未找到 .clang-format，已跳过"
		return
	}

	$ScopedFiles = @(Get-ScopedFirstPartyFiles)
	if($ScopedFiles.Count -eq 0) {
		Add-Result "WARN" "clang-format 附加检查" "未收集到改动范围内的首方 C/C++ 文件"
		return
	}

	foreach($RelativePath in $ScopedFiles) {
		Invoke-RepoCommandAsWarning "clang-format 附加检查: $RelativePath" $ClangFormat.Source @(
			"--dry-run",
			"--Werror",
			(Join-Path $RepoRoot $RelativePath)
		)
	}
}

function Invoke-FullClangTidyWarnChecks {
	$ClangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
	if($null -eq $ClangTidy) {
		Add-Result "WARN" "全量 .clang-tidy 附加检查" "PATH 中未找到 clang-tidy，已跳过"
		return
	}
	if(!(Test-Path $ClangTidyConfig)) {
		Add-Result "WARN" "全量 .clang-tidy 附加检查" "未找到 .clang-tidy，已跳过"
		return
	}

	$CompileCommands = Join-Path $RepoRoot "build-debug\compile_commands.json"
	if(!(Test-Path $CompileCommands)) {
		Add-Result "WARN" "全量 .clang-tidy 附加检查" "缺少 build-debug/compile_commands.json，请先跑 strict-debug-check 或 default/full 构建层"
		return
	}

	$ScopedFiles = @(Get-StrictDebugFiles)
	if($ScopedFiles.Count -eq 0) {
		Add-Result "WARN" "全量 .clang-tidy 附加检查" "未收集到改动范围内的首方 C/C++ 文件"
		return
	}

	foreach($FilePath in $ScopedFiles) {
		Invoke-RepoCommandAsWarning "全量 .clang-tidy 附加检查: $FilePath" $ClangTidy.Source @(
			$FilePath,
			"-p=build-debug",
			"--config-file=$ClangTidyConfig",
			"--extra-arg=-Qunused-arguments",
			"--quiet"
		)
	}
}

function Invoke-StrictDebugGate {
	$CommandBody = @(
		'$ErrorActionPreference = ''Stop'''
	)
	$BaseRefLiteral = Convert-ToPowerShellSingleQuotedLiteral $BaseRef
	if($AnalyzeAll) {
		$CommandBody += "& $(Convert-ToPowerShellSingleQuotedLiteral $StrictDebugCheck) -BaseRef $BaseRefLiteral -AnalyzeAll"
	} else {
		$ScopedFiles = @(Get-StrictDebugFiles)
		if($ScopedFiles.Count -gt 0) {
			Add-Result "INFO" "严格构建与静态分析范围" ("按首方源码差异传入 {0} 个文件（已排除 external/generated 等目录）" -f $ScopedFiles.Count)
			$FileListLiteral = @($ScopedFiles | ForEach-Object { Convert-ToPowerShellSingleQuotedLiteral $_ }) -join ', '
			$CommandBody += ('$files = @({0})' -f $FileListLiteral)
			$CommandBody += "& $(Convert-ToPowerShellSingleQuotedLiteral $StrictDebugCheck) -BaseRef $BaseRefLiteral -Files `$files"
		} else {
			Add-Result "WARN" "严格构建与静态分析范围" "未收集到首方 src 差异文件；strict-debug-check 将退回自身默认范围"
			$CommandBody += "& $(Convert-ToPowerShellSingleQuotedLiteral $StrictDebugCheck) -BaseRef $BaseRefLiteral"
		}
	}
	$CommandText = $CommandBody -join '; '
	Invoke-RepoCommand "严格构建与静态分析入口" "powershell" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", $CommandText)
}

function Invoke-TestTargets {
	if($RunAllTests) {
		Invoke-RepoCommand "CMake run_tests" $CmakeWrapper @("--build", $BuildDir, "--target", "run_tests", "-j", "10")
		return
	}

	if($Script:ModeRunCxxTests -and -not $SkipCxxTests) {
		Invoke-GateStep "CMake run_cxx_tests" {
			Invoke-RepoCommand "CMake run_cxx_tests" $CmakeWrapper @("--build", $BuildDir, "--target", "run_cxx_tests", "-j", "10")
		}
	}

	if($Script:ModeRunRustTests) {
		Invoke-GateStep "CMake run_rust_tests" {
			Invoke-RepoCommand "CMake run_rust_tests" $CmakeWrapper @("--build", $BuildDir, "--target", "run_rust_tests", "-j", "10")
		}
	}
}

function Write-Summary {
	$FailItems = @($Script:ResultItems | Where-Object { $_.Level -eq "FAIL" })
	$WarnItems = @($Script:ResultItems | Where-Object { $_.Level -eq "WARN" })

	Write-Section "检查汇总"
	Write-ResultLine "INFO" ("模式: {0}" -f $Mode)
	Write-ResultLine "INFO" ("模式目标: {0}" -f $Script:ModeSpec.Target)
	Write-ResultLine "INFO" ("模式预期: {0}" -f $Script:ModeSpec.Expectation)
	Write-ResultLine "INFO" ("阻断规则: {0}" -f $Script:ModeSpec.BlockingRule)
	Write-ResultLine "INFO" ("通过: {0}" -f $Script:ResultPass)
	Write-ResultLine "INFO" ("警告: {0}" -f $Script:ResultWarn)
	Write-ResultLine "INFO" ("失败: {0}" -f $Script:ResultFail)

	if($FailItems.Count -gt 0) {
		Write-Host ""
		Write-Host "失败分类："
		$FailItems |
			Group-Object CategoryLabel |
			Sort-Object Name |
			ForEach-Object {
				Write-ResultLine "FAIL" ("{0}: {1}" -f $_.Name, $_.Count)
			}
	}

	if($WarnItems.Count -gt 0) {
		Write-Host ""
		Write-Host "警告清单："
		$WarnItems | ForEach-Object {
			Write-ResultLine "WARN" ("[{0}] {1}: {2}" -f $_.CategoryLabel, $_.Title, $_.Detail)
		}
	}

	if($FailItems.Count -gt 0) {
		Write-Host ""
		Write-Host "失败清单："
		$FailItems | ForEach-Object {
			Write-ResultLine "FAIL" ("[{0}] {1}: {2}" -f $_.CategoryLabel, $_.Title, $_.Detail)
		}
	}
}

function Write-JsonReport {
	if([string]::IsNullOrWhiteSpace($ReportJsonPath)) {
		return
	}

	$ParentDir = Split-Path -Parent $ReportJsonPath
	if(-not [string]::IsNullOrWhiteSpace($ParentDir) -and !(Test-Path $ParentDir)) {
		New-Item -ItemType Directory -Path $ParentDir -Force | Out-Null
	}

	$ReportScopedFiles = @(Get-ScopedFirstPartyFiles)
	$ReportItems = @($Script:ResultItems | ForEach-Object { $_ })
	$FailureSummaryByCategory = @(
		$ReportItems |
			Where-Object { $_.Level -eq "FAIL" } |
			Group-Object CategoryId, CategoryLabel |
			ForEach-Object {
				[pscustomobject]@{
					CategoryId = $_.Group[0].CategoryId
					CategoryLabel = $_.Group[0].CategoryLabel
					Count = $_.Count
				}
			}
	)
	$Report = [pscustomobject]@{
		Mode = $Mode
		BaseRef = $BaseRef
		GeneratedAt = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
		ModeSpec = $Script:ModeSpec
		Summary = [pscustomobject]@{
			Pass = $Script:ResultPass
			Warn = $Script:ResultWarn
			Fail = $Script:ResultFail
		}
		FailureSummaryByCategory = $FailureSummaryByCategory
		ScopedFiles = $ReportScopedFiles
		Items = $ReportItems
	}

	$Report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ReportJsonPath -Encoding UTF8
}

Resolve-ModeToggles
Load-BaselineDebtAllowlist

Push-Location $RepoRoot
try {
	if(!$SkipPreflight) {
		Invoke-GateStep "环境前置检查" {
			Assert-WorkingTreePreflight
		}
	}
	Invoke-GateStep "差异范围诊断" {
		Write-ScopeDiagnostics
	}

	# 第一层：源码卫生层。默认所有模式都会跑。
	if(!$SkipConfigChecks) {
		Invoke-GateStep "配置变量使用检查（Qm/Tc/栖梦）" {
			Invoke-ConfigChecks
		}
	}
	Invoke-GateStep "工作流文档一致性检查" {
		Invoke-WorkflowDocChecks
	}
	if(!$SkipHeaderChecks) {
		Invoke-GateStep "头文件检查" {
			Invoke-HeaderChecks
		}
	}
	if(!$SkipStyleCheck) {
		Invoke-GateStep "代码格式检查" {
			Invoke-StyleChecks
		}
	}

	# 第二层：可选源码规则层。默认只在 full 模式打开，避免当前仓库误报过多。
	if($Script:ModeRunIdentifierCheck) {
		Invoke-GateStep "标识符命名检查" {
			Invoke-IdentifierChecks
		}
	}
	if($Script:ModeRunUnusedHeaderCheck) {
		Invoke-GateStep "未使用头文件检查" {
			Invoke-UnusedHeaderChecks
		}
	}
	if($Script:ModeRunClangFormatCheck) {
		Invoke-GateStep "clang-format 附加检查" {
			Invoke-ClangFormatChecks
		}
	}

	# 第三层：严格构建 / 静态分析层。default/full 模式默认开启。
	if($Script:ModeRunStrictDebug -and -not $SkipStrictDebug) {
		Invoke-GateStep "严格构建与静态分析入口" {
			Invoke-StrictDebugGate
		}
	}
	if($Script:ModeRunFullClangTidyWarn) {
		Invoke-GateStep "全量 .clang-tidy 附加检查" {
			Invoke-FullClangTidyWarnChecks
		}
	}

	# 第四层：测试层。default/full 模式默认跑 C++，full 可附加 Rust。
	Invoke-GateStep "测试目标" {
		Invoke-TestTargets
	}

	Invoke-GateStep "JSON 报告" {
		Write-JsonReport
	}
	Write-Summary

	if($Script:ResultFail -gt 0) {
		exit 1
	}

	Write-Host ""
	Write-Host "仓库级检查完成。"
	exit 0
} finally {
	Pop-Location
}
