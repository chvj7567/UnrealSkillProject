# PostToolUse Write|Edit — .cpp/.h 파일 저장 시 clang-format 자동 실행
$d = $input | ConvertFrom-Json
$f = $d.tool_input.file_path

if ($f -match '\.(cpp|h)$') {
    & "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" -i $f 2>$null
}
