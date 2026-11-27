@echo off
setlocal

set "PROFILE_DIR=%LOCALAPPDATA%\AXIOM_LiveBrowser"
set "CHROME_PF=%ProgramFiles%\Google\Chrome\Application\chrome.exe"
set "CHROME_LOCAL=%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe"
set "EDGE_X86=%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"
set "EDGE_PF=%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"

set "BROWSER_EXE="
if exist "%CHROME_PF%" (
    set "BROWSER_EXE=%CHROME_PF%"
) else if exist "%CHROME_LOCAL%" (
    set "BROWSER_EXE=%CHROME_LOCAL%"
) else if exist "%EDGE_X86%" (
    set "BROWSER_EXE=%EDGE_X86%"
) else if exist "%EDGE_PF%" (
    set "BROWSER_EXE=%EDGE_PF%"
)

if "%BROWSER_EXE%"=="" (
    echo [ERROR] Neither Google Chrome nor Microsoft Edge was found.
    exit /b 1
)

if not exist "%PROFILE_DIR%" mkdir "%PROFILE_DIR%"

start "" "%BROWSER_EXE%" ^
    --remote-debugging-port=9222 ^
    --remote-debugging-address=127.0.0.1 ^
    --user-data-dir="%PROFILE_DIR%" ^
    --no-first-run ^
    --no-default-browser-check ^
    --window-position=960,0 ^
    --window-size=960,1040

endlocal
