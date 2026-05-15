param(
	[string]$DebugBuildDir = "build-debug",
	[string]$AnalyzeBuildDir = "build-analyze",
	[string]$AsanBuildDir = "build-asan",
	[string]$BaseRef = "main",
	[string[]]$Files = @(),
	[switch]$AnalyzeAll,
	[switch]$RequireAsan,
	[switch]$SkipBuild,
	[switch]$SkipTidy,
	[switch]$SkipAnalyze,
	[switch]$SkipAsan,
	[switch]$PrintFileScope,
	[string]$ReportJsonPath = ""
)

$ErrorActionPreference = "Stop"

# -----------------------------------------------------------------------------
# QmClient Windows 严格调试检查入口
#
# 设计目标：
# 1. 统一串起 build-debug、MSVC /analyze、ASan、clang-tidy。
# 2. 所有阶段输出中文，并在结尾给出统一汇总。
# 3. 默认只扫当前工作树真实改动范围；如需全量 /analyze，显式传 -AnalyzeAll。
# 4. Windows 目录语义固定为：build-debug=正式调试目录，build-analyze=/analyze 专用目录，build-asan=实验目录。
# -----------------------------------------------------------------------------

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$CmakeWrapper = Join-Path $RepoRoot "qmclient_scripts\cmake-windows.cmd"

$Script:ResultPass = 0
$Script:ResultWarn = 0
$Script:ResultFail = 0
$Script:ResultItems = New-Object System.Collections.ArrayList
$Script:DefaultScopeCache = $null
$Script:EffectiveFiles = @()
$Script:AnalyzeSourceFiles = @()
$Script:Degraded = $false
$Script:InputFilesExplicit = $false
$Script:InputScopeFiles = @()

function Add-Result {
	param(
		[ValidateSet("PASS", "WARN", "FAIL", "INFO")]
		[string]$Level,
		[string]$Title,
		[string]$Detail
	)

	switch($Level) {
		"PASS" { $Script:ResultPass++ }
		"WARN" { $Script:ResultWarn++ }
		"FAIL" { $Script:ResultFail++ }
	}

	[void]$Script:ResultItems.Add([pscustomobject]@{
		Level = $Level
		Title = $Title
		Detail = $Detail
	})
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

function Mark-Degraded {
	param(
		[string]$Title,
		[string]$Detail
	)

	$Script:Degraded = $true
	Add-Result "WARN" $Title $Detail
}

function Invoke-RepoCommand {
	param(
		[string]$Title,
		[string]$FilePath,
		[string[]]$Arguments,
		[switch]$FailOnWarnings
	)

	Write-Section $Title
	Write-Host ("命令: {0} {1}" -f $FilePath, ($Arguments -join ' '))

	$OutputLines = @()
	& $FilePath @Arguments 2>&1 | ForEach-Object {
		$Line = $_.ToString()
		$OutputLines += $Line
		Write-Host $Line
	}

	$ExitCode = $LASTEXITCODE
	if($ExitCode -ne 0) {
		$FailureSummary = Get-RepoCommandFailureSummary -Title $Title -OutputLines $OutputLines
		$FailureMessage = "$Title 失败，退出码 $ExitCode"
		if($null -ne $FailureSummary) {
			$FailureMessage = "$FailureMessage`n$FailureSummary"
		}
		Add-Result "FAIL" $Title $FailureMessage
		throw $FailureMessage
	}

	if($FailOnWarnings) {
		$WarningLines = @($OutputLines | Where-Object { Test-StrictWarningLine $_ })
		if($WarningLines.Count -gt 0) {
			$WarningMessage = "$Title 输出了 warning 文本:`n$($WarningLines -join "`n")"
			Add-Result "FAIL" $Title $WarningMessage
			throw $WarningMessage
		}
	}

	Add-Result "PASS" $Title "执行通过"
}

function Get-RepoCommandFailureSummary {
	param(
		[string]$Title,
		[string[]]$OutputLines
	)

	if($Title -notlike "clang-tidy 严格检查:*") {
		return $null
	}

	$UnusedArgumentLines = @($OutputLines | Where-Object { $_ -match 'clang-diagnostic-unused-command-line-argument' })
	if($UnusedArgumentLines.Count -gt 0) {
		$RelevantLines = ($UnusedArgumentLines | Select-Object -First 6) -join "`n"
		return "原因：clang-tidy 看到的是 MSVC compile_commands 参数，其中一部分 clang 自身不会消费。这更像工具链调用问题，不一定是源码诊断。相关输出：`n$RelevantLines"
	}

	$RelevantErrorLines = @($OutputLines | Where-Object {
		$_ -match '^error:' -or
		$_ -match '^\d+ warnings and \d+ errors generated\.$' -or
		$_ -match '^Error while processing ' -or
		$_ -match '^Found compiler error\(s\)\.$'
	})
	if($RelevantErrorLines.Count -gt 0) {
		$SummaryLines = ($RelevantErrorLines | Select-Object -First 8) -join "`n"
		return "原因：clang-tidy 找到了真实诊断或编译错误。相关输出：`n$SummaryLines"
	}

	return $null
}

function Test-StrictWarningLine {
	param([string]$Line)

	if([string]::IsNullOrWhiteSpace($Line)) {
		return $false
	}

	if($Line -match '^CMake Warning') {
		return $true
	}

	if($Line -match '(^|: |\s)warning C\d+\b') {
		return $true
	}

	if($Line -match ':\s*warning\b') {
		return $true
	}

	if($Line -match '\bwarning:\b') {
		return $true
	}

	if($Line -match '^(cl|clang-cl|link|LINK) : warning\b') {
		return $true
	}

	if($Line -match '^\d+ warnings generated\.$') {
		return $true
	}

	if($Line -match '^Suppressed \d+ warnings') {
		return $true
	}

	return $false
}

function Get-BranchDiffFiles {
	$MergeBaseOutput = @(& git -C $RepoRoot merge-base $BaseRef HEAD 2>&1)
	$MergeBaseExitCode = $LASTEXITCODE
	$MergeBase = ($MergeBaseOutput | Select-Object -First 1).ToString().Trim()
	if($MergeBaseExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($MergeBase)) {
		$FailureReason = (($MergeBaseOutput | ForEach-Object { $_.ToString().Trim() } | Where-Object { $_ }) -join " | ")
		if([string]::IsNullOrWhiteSpace($FailureReason)) {
			$FailureReason = "git merge-base 返回空结果"
		}
		Mark-Degraded "分支差异基线" ("无法解析 BaseRef={0}，将退回仅使用工作树差异。原因：{1}" -f $BaseRef, $FailureReason)
		return @()
	}

	$Files = @(
		& git -C $RepoRoot -c core.safecrlf=false diff --name-only --diff-filter=ACMR "$MergeBase...HEAD" -- "*.c" "*.cc" "*.cpp" "*.h" "*.hpp" 2>$null |
			ForEach-Object { $_.ToString().Trim() } |
			Where-Object { $_ }
	)
	return @($Files | Sort-Object -Unique)
}

function Get-DefaultCheckFiles {
	if($null -ne $Script:DefaultScopeCache) {
		return $Script:DefaultScopeCache
	}

	$BranchFiles = @(Get-BranchDiffFiles)
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

	$ExcludedFiles = New-Object System.Collections.ArrayList
	$ScopedFiles = @(
		$BranchFiles + $UnstagedFiles + $StagedFiles + $UntrackedFiles |
			Where-Object { $_ } |
			ForEach-Object { $_ -replace '\\', '/' } |
			ForEach-Object {
				$Normalized = $_
				if(Test-IsScopedFirstPartyFile $Normalized) {
					$Normalized
				} else {
					$Reason = "not-in-src"
					if($Normalized -match '^src/engine/external/') {
						$Reason = "third-party-external"
					} elseif($Normalized -match '^src/game/generated/') {
						$Reason = "generated"
					} elseif($Normalized -match '^src/rust-bridge/base/') {
						$Reason = "rust-bridge-base"
					} elseif($Normalized -match '^src/' -and $Normalized -notmatch '\.(c|cc|cpp|h|hpp)$') {
						$Reason = "extension-not-supported"
					}
					[void]$ExcludedFiles.Add([pscustomobject]@{
						Path = $Normalized
						Reason = $Reason
					})
				}
			} |
			Where-Object { $_ } |
			Sort-Object -Unique
	)

	$Script:DefaultScopeCache = [pscustomobject]@{
		BaseRef = $BaseRef
		Branch = @($BranchFiles | Sort-Object -Unique)
		Unstaged = @($UnstagedFiles | Sort-Object -Unique)
		Staged = @($StagedFiles | Sort-Object -Unique)
		Untracked = @($UntrackedFiles | Sort-Object -Unique)
		Scoped = $ScopedFiles
		Excluded = @($ExcludedFiles | ForEach-Object { $_ })
	}
	return $Script:DefaultScopeCache
}

function Write-DefaultScopeDiagnostics {
	$Scope = Get-DefaultCheckFiles
	Add-Result "INFO" "默认文件范围统计" ("branch={0}, unstaged={1}, staged={2}, untracked={3}, scoped={4}, excluded={5}" -f
		$Scope.Branch.Count,
		$Scope.Unstaged.Count,
		$Scope.Staged.Count,
		$Scope.Untracked.Count,
		$Scope.Scoped.Count,
		$Scope.Excluded.Count)

	if($PrintFileScope) {
		Write-Section "默认文件范围说明"
		Write-Host ("BaseRef: {0}" -f $Scope.BaseRef)
		Write-Host ("分支差异文件数: {0}" -f $Scope.Branch.Count)
		Write-Host ("未暂存文件数: {0}" -f $Scope.Unstaged.Count)
		Write-Host ("已暂存文件数: {0}" -f $Scope.Staged.Count)
		Write-Host ("未跟踪文件数: {0}" -f $Scope.Untracked.Count)
		Write-Host ("纳入严格检查的首方文件数: {0}" -f $Scope.Scoped.Count)
		Write-Host ("排除文件数: {0}" -f $Scope.Excluded.Count)
		if($Scope.Scoped.Count -gt 0) {
			Write-Host "纳入文件："
			$Scope.Scoped | ForEach-Object { Write-Host ("  - {0}" -f $_) }
		}
		if($Scope.Excluded.Count -gt 0) {
			Write-Host "排除文件："
			$Scope.Excluded | ForEach-Object { Write-Host ("  - {0} [{1}]" -f $_.Path, $_.Reason) }
		}
	}
}

function Get-AnalyzeSourceFiles {
	param([string[]]$InputFiles)

	@($InputFiles |
		Where-Object { $_ } |
		ForEach-Object {
			if([System.IO.Path]::IsPathRooted($_)) {
				[System.IO.Path]::GetRelativePath($RepoRoot, $_)
			} else {
				$_
			}
		} |
		Where-Object {
			$Normalized = $_ -replace '\\', '/'
			$Normalized -like "src/*" -and $Normalized -match '\.(c|cc|cpp)$'
		} |
		ForEach-Object { $_ -replace '\\', '/' } |
		Sort-Object -Unique)
}

function Test-AsanSupported {
	$CargoConfig = Join-Path $RepoRoot ".cargo\config.toml"
	if(!(Test-Path $CargoConfig)) {
		return $true
	}

	$CargoConfigText = Get-Content $CargoConfig -Raw
	return $CargoConfigText -notmatch 'target-feature=\+crt-static'
}

function Invoke-ConfigureAndBuild {
	param(
		[string]$Title,
		[string]$BuildDir,
		[string[]]$ConfigureArgs,
		[switch]$FailOnWarnings
	)

	Invoke-RepoCommand "$Title 配置" $CmakeWrapper $ConfigureArgs -FailOnWarnings:$FailOnWarnings
	if(!$SkipBuild) {
		Invoke-RepoCommand "$Title 构建" $CmakeWrapper @("--build", $BuildDir, "--target", "game-client", "-j", "10") -FailOnWarnings:$FailOnWarnings
	} else {
		Add-Result "WARN" "$Title 构建" "已显式传入 -SkipBuild，仅执行配置阶段"
	}
}

function Write-Summary {
	Write-Section "检查汇总"
	Write-ResultLine "INFO" ("DebugBuildDir: {0}" -f $DebugBuildDir)
	Write-ResultLine "INFO" ("AnalyzeBuildDir: {0}" -f $AnalyzeBuildDir)
	Write-ResultLine "INFO" ("AsanBuildDir: {0}" -f $AsanBuildDir)
	Write-ResultLine "INFO" ("BaseRef: {0}" -f $BaseRef)
	Write-ResultLine "INFO" ("降级运行: {0}" -f $(if($Script:Degraded) { "是" } else { "否" }))
	Write-ResultLine "INFO" ("通过: {0}" -f $Script:ResultPass)
	Write-ResultLine "INFO" ("警告: {0}" -f $Script:ResultWarn)
	Write-ResultLine "INFO" ("失败: {0}" -f $Script:ResultFail)

	if($Script:ResultWarn -gt 0) {
		Write-Host ""
		Write-Host "警告清单："
		$Script:ResultItems | Where-Object { $_.Level -eq "WARN" } | ForEach-Object {
			Write-ResultLine "WARN" ("{0}: {1}" -f $_.Title, $_.Detail)
		}
	}

	if($Script:ResultFail -gt 0) {
		Write-Host ""
		Write-Host "失败清单："
		$Script:ResultItems | Where-Object { $_.Level -eq "FAIL" } | ForEach-Object {
			Write-ResultLine "FAIL" ("{0}: {1}" -f $_.Title, $_.Detail)
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

	$Scope = Get-DefaultCheckFiles
	$ReportItems = @($Script:ResultItems | ForEach-Object { $_ })
	$Report = [pscustomobject]@{
		DebugBuildDir = $DebugBuildDir
		AnalyzeBuildDir = $AnalyzeBuildDir
		AsanBuildDir = $AsanBuildDir
		BaseRef = $BaseRef
		Degraded = $Script:Degraded
		InputScope = [pscustomobject]@{
			ExplicitFiles = $Script:InputFilesExplicit
			Files = @($Script:InputScopeFiles | ForEach-Object { $_ })
		}
		DefaultScope = [pscustomobject]@{
			Branch = @($Scope.Branch)
			Unstaged = @($Scope.Unstaged)
			Staged = @($Scope.Staged)
			Untracked = @($Scope.Untracked)
			Scoped = @($Scope.Scoped)
			Excluded = @($Scope.Excluded | ForEach-Object {
				[pscustomobject]@{
					Path = $_.Path
					Reason = $_.Reason
				}
			})
		}
		EffectiveScope = [pscustomobject]@{
			Files = @($Script:EffectiveFiles | ForEach-Object { $_ })
			AnalyzeSourceFiles = @($Script:AnalyzeSourceFiles | ForEach-Object { $_ })
		}
		Summary = [pscustomobject]@{
			Pass = $Script:ResultPass
			Warn = $Script:ResultWarn
			Fail = $Script:ResultFail
		}
		Items = $ReportItems
	}

	$Report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ReportJsonPath -Encoding UTF8
	Add-Result "INFO" "JSON 报告" ("已写入: {0}" -f $ReportJsonPath)
}

try {
	if(!(Test-Path $CmakeWrapper)) {
		$Message = "缺少 CMake 包装脚本: $CmakeWrapper"
		Add-Result "FAIL" "入口前置检查" $Message
		throw $Message
	}
	Add-Result "PASS" "入口前置检查" "已找到 qmclient_scripts/cmake-windows.cmd"

	if($Files.Count -eq 0) {
		$Script:InputFilesExplicit = $false
		Write-DefaultScopeDiagnostics
		$Files = @((Get-DefaultCheckFiles).Scoped)
	} else {
		$Script:InputFilesExplicit = $true
		Add-Result "INFO" "文件范围来源" ("已显式传入 -Files，共 {0} 个文件" -f $Files.Count)
	}
	$Script:InputScopeFiles = @($Files | ForEach-Object { $_ })

	$Script:EffectiveFiles = @($Files | ForEach-Object { $_ })

	Invoke-ConfigureAndBuild "Debug CRT" $DebugBuildDir @(
		"-G", "Ninja",
		"-S", ".",
		"-B", $DebugBuildDir,
		"-DCMAKE_BUILD_TYPE=Debug",
		"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
		"-DQM_STRICT_WARNINGS=ON",
		"-DQM_ENABLE_WINDOWS_CRT_ASSERT_LOGGER=ON"
	) -FailOnWarnings:$true

	if(!$SkipAnalyze) {
		$AnalyzeSourceFiles = @()
		if(!$AnalyzeAll) {
			$AnalyzeSourceFiles = @(Get-AnalyzeSourceFiles $Files)
			if($AnalyzeSourceFiles.Count -eq 0) {
				Mark-Degraded "MSVC /analyze 范围" "当前改动只有头文件或无可分析编译单元，/analyze 阶段已跳过"
			}
			if($AnalyzeSourceFiles.Count -gt 0) {
				$Script:AnalyzeSourceFiles = @($AnalyzeSourceFiles | ForEach-Object { $_ })
				Add-Result "INFO" "MSVC /analyze 范围" ("仅扫描 {0} 个改动编译单元" -f $AnalyzeSourceFiles.Count)
			}
		} else {
			Add-Result "INFO" "MSVC /analyze 范围" "已显式传入 -AnalyzeAll，将执行全量首方源码分析"
		}

		if($AnalyzeAll -or $AnalyzeSourceFiles.Count -gt 0) {
			$AnalyzeConfigureArgs = @(
				"-G", "Ninja",
				"-S", ".",
				"-B", $AnalyzeBuildDir,
				"-DCMAKE_BUILD_TYPE=Debug",
				"-DQM_STRICT_WARNINGS=ON",
				"-DQM_MSVC_ANALYZE=ON",
				"-DQM_ENABLE_WINDOWS_CRT_ASSERT_LOGGER=ON"
			)
			if(!$AnalyzeAll) {
				$AnalyzeConfigureArgs += "-DQM_MSVC_ANALYZE_ONLY=$($AnalyzeSourceFiles -join ';')"
			}

			Invoke-ConfigureAndBuild "MSVC /analyze" $AnalyzeBuildDir $AnalyzeConfigureArgs -FailOnWarnings:$true
		}
	} else {
		Add-Result "WARN" "MSVC /analyze" "已显式传入 -SkipAnalyze，跳过 /analyze 阶段"
	}

	if(!$SkipAsan) {
		if(!(Test-AsanSupported)) {
			$AsanUnsupportedMessage = "AddressSanitizer 已跳过：当前仓库的 .cargo/config.toml 在 MSVC 下强制 Rust +crt-static。"
			if($RequireAsan) {
				Add-Result "FAIL" "AddressSanitizer" $AsanUnsupportedMessage
				throw $AsanUnsupportedMessage
			}
			Write-Section "AddressSanitizer"
			Write-Host $AsanUnsupportedMessage
			Add-Result "WARN" "AddressSanitizer" $AsanUnsupportedMessage
		} else {
			Invoke-ConfigureAndBuild "AddressSanitizer" $AsanBuildDir @(
				"-G", "Ninja",
				"-S", ".",
				"-B", $AsanBuildDir,
				"-DCMAKE_BUILD_TYPE=Debug",
				"-DQM_STRICT_WARNINGS=ON",
				"-DQM_ENABLE_ASAN=ON",
				"-DQM_ENABLE_WINDOWS_CRT_ASSERT_LOGGER=ON"
			) -FailOnWarnings:$true
		}
	} else {
		Add-Result "WARN" "AddressSanitizer" "已显式传入 -SkipAsan，跳过 ASan 阶段"
	}

	if($SkipTidy) {
		Add-Result "WARN" "clang-tidy" "已显式传入 -SkipTidy，跳过 tidy 阶段"
		Write-JsonReport
		Write-Summary
		exit 0
	}

	$ClangTidy = (Get-Command clang-tidy -ErrorAction SilentlyContinue)
	if($null -eq $ClangTidy) {
		$Message = "PATH 中未找到 clang-tidy，无法执行严格 tidy 检查。"
		Add-Result "FAIL" "clang-tidy 前置检查" $Message
		throw $Message
	}
	Add-Result "PASS" "clang-tidy 前置检查" "已找到 clang-tidy"

	$CompileCommands = Join-Path $DebugBuildDir "compile_commands.json"
	if(!(Test-Path $CompileCommands)) {
		Invoke-RepoCommand "Debug CRT 重新导出 compile_commands" $CmakeWrapper @(
			"-G", "Ninja",
			"-S", ".",
			"-B", $DebugBuildDir,
			"-DCMAKE_BUILD_TYPE=Debug",
			"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
			"-DQM_STRICT_WARNINGS=ON",
			"-DQM_ENABLE_WINDOWS_CRT_ASSERT_LOGGER=ON"
		) -FailOnWarnings:$true
	}

	if(!(Test-Path $CompileCommands)) {
		$Message = "重新配置后仍未生成 compile_commands.json: $CompileCommands"
		Add-Result "FAIL" "compile_commands 检查" $Message
		throw $Message
	}
	Add-Result "PASS" "compile_commands 检查" "compile_commands.json 已就绪"

	$Files = @($Files |
		Where-Object { $_ } |
		ForEach-Object {
			if([System.IO.Path]::IsPathRooted($_)) { $_ } else { Join-Path $RepoRoot $_ }
		} |
		Where-Object { Test-Path $_ } |
		Sort-Object -Unique)
	$Script:EffectiveFiles = @($Files | ForEach-Object { $_ })

	if($Files.Count -eq 0) {
		Write-Section "clang-tidy"
		Write-Host "没有可供 clang-tidy 检查的 C/C++ 文件。"
		Add-Result "WARN" "clang-tidy" "当前没有可检查的 C/C++ 文件，tidy 阶段已跳过"
		Write-JsonReport
		Write-Summary
		exit 0
	}

	$Checks = "-*,bugprone-unchecked-optional-access,clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-nullability.*,clang-analyzer-optin.cplusplus.UninitializedObject"
	$WarningsAsErrors = "bugprone-unchecked-optional-access,clang-analyzer-*"
	Add-Result "INFO" "clang-tidy 范围" ("将对 {0} 个文件执行严格 tidy 检查" -f $Files.Count)

	foreach($File in $Files) {
		Invoke-RepoCommand "clang-tidy 严格检查: $File" $ClangTidy.Source @(
			$File,
			"-p=$DebugBuildDir",
			"--checks=$Checks",
			"--warnings-as-errors=$WarningsAsErrors",
			"--extra-arg=-Qunused-arguments",
			"--quiet"
		) -FailOnWarnings:$true
	}

	Write-JsonReport
	Write-Summary

	if($Script:ResultFail -gt 0) {
		throw "严格调试检查存在失败项。"
	}

	Write-Host ""
	Write-Host "严格调试检查完成。"
} catch {
	Write-JsonReport
	Write-Summary
	throw
}
