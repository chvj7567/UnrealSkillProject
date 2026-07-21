@echo off
setlocal

:: 엔진 경로는 머신마다 다르므로 레지스트리에서 UE 5.7 설치 위치를 조회한다.
set "UE_ROOT="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.7" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"

:: 조회 실패 시 알려진 설치 경로로 폴백
if not defined UE_ROOT set "UE_ROOT=D:\UE_5.7"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.7"

if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo [Launch.bat] UE 5.7 엔진을 찾지 못했습니다. UE_ROOT 를 직접 지정하세요.
    exit /b 1
)

echo [Launch.bat] Engine: %UE_ROOT%

call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ^
-projectfiles -project="%~dp0\SkillProject.uproject" -game -rocket -progress

::call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ^
::SkillProject Win64 Development -Project="%~dp0\SkillProject.uproject" -WaitMutex -FromMsBuild -architecture=x64

::call "%~dp0\SkillProject.uproject"
