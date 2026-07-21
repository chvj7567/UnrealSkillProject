# PostToolUse Write|Edit — .cpp/.h 저장 시 "변경된 라인만" clang-format 적용
#
# 파일 전체를 포맷하면 편집 2줄짜리 커밋에 수백 줄 재포맷 diff 가 섞인다.
# 그래서 HEAD 대비 실제로 바뀐 라인 범위만 --lines 로 지정해 포맷한다.
# 추적되지 않는 새 파일은 비교 대상이 없으므로 전체 포맷 (churn 없음).
# 포맷 규칙은 저장소 루트 .clang-format 참조.

$clangFormat = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"

try {
    $payload = $input | ConvertFrom-Json
    $file = $payload.tool_input.file_path
}
catch {
    exit 0
}

if ([string]::IsNullOrWhiteSpace($file)) { exit 0 }
if ($file -notmatch '\.(cpp|h|hpp|inl)$') { exit 0 }
if (-not (Test-Path -LiteralPath $file)) { exit 0 }
if (-not (Test-Path -LiteralPath $clangFormat)) { exit 0 }

$resolved = (Resolve-Path -LiteralPath $file).Path
$dir = Split-Path -Parent $resolved

Push-Location $dir
try {
    # 저장소 밖이면 비교 기준이 없으므로 전체 포맷
    $null = git rev-parse --is-inside-work-tree 2>$null
    if ($LASTEXITCODE -ne 0) {
        & $clangFormat -i $resolved
        exit 0
    }

    # 추적되지 않는 새 파일 → 전체 포맷 (기존 코드가 없으니 노이즈 없음)
    $null = git ls-files --error-unmatch -- $resolved 2>$null
    if ($LASTEXITCODE -ne 0) {
        & $clangFormat -i $resolved
        exit 0
    }

    # HEAD 대비 변경된 라인 범위 수집
    $diff = git diff -U0 HEAD -- $resolved 2>$null
    $ranges = @()

    foreach ($line in $diff) {
        if ($line -match '^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@') {
            $start = [int]$Matches[1]
            $count = 1
            if ($Matches[2]) { $count = [int]$Matches[2] }

            # 순수 삭제 hunk 는 포맷할 라인이 없다
            if ($count -eq 0) { continue }

            $end = $start + $count - 1
            $ranges += "--lines=${start}:${end}"
        }
    }

    # 변경 없음 → 아무것도 하지 않는다
    if ($ranges.Count -eq 0) { exit 0 }

    & $clangFormat -i @ranges $resolved
}
finally {
    Pop-Location
}

exit 0
