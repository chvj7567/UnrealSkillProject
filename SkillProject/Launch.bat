@echo off
chcp 65001 >nul
setlocal

:: 엔진 경로는 머신마다 다르므로 레지스트리에서 UE 5.7 설치 위치를 조회한다.
set "UE_ROOT="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.7" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"

:: 조회 실패 시 알려진 설치 경로로 폴백
if not defined UE_ROOT set "UE_ROOT=D:\UE_5.7"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.7"

if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo [Launch.bat] UE 5.7 엔진을 찾지 못했습니다. UE_ROOT 를 직접 지정하세요.
    pause
    exit /b 1
)

echo [Launch.bat] Engine: %UE_ROOT%

call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ^
-projectfiles -project="%~dp0\SkillProject.uproject" -game -rocket -progress
set "RC=%ERRORLEVEL%"
chcp 65001 >nul

echo.
if "%RC%"=="0" (
    echo ==========================================================
    echo  [성공] 프로젝트 파일 재생성 완료   종료 코드 = %RC%
    echo ==========================================================
    echo  새로 추가/삭제한 소스가 Visual Studio 에 반영됩니다.
    echo.
    echo  참고: 이 엔진은 런처 배포판이라 Server 구성은 빌드되지 않습니다.
    echo        데디 검증은 SpyProject\RunDediFast.bat 을 쓰세요.
) else (
    echo ==========================================================
    echo  [실패] 프로젝트 파일 재생성 실패   종료 코드 = %RC%
    echo ==========================================================
    echo  위 출력에서 error 로 시작하는 줄을 확인하세요.
)
echo.
pause

::call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ^
::SkillProject Win64 Development -Project="%~dp0\SkillProject.uproject" -WaitMutex -FromMsBuild -architecture=x64

::call "%~dp0\SkillProject.uproject"
