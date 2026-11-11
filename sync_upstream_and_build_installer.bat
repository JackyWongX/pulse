@echo off
setlocal
cd /d "%~dp0"

set "UPSTREAM_URL=https://github.com/jimmgreen/pulse.git"

git rev-parse --show-toplevel >nul 2>&1
if errorlevel 1 (
    echo This script must run inside the Pulse Git repository.
    pause
    exit /b 1
)

for /f "delims=" %%R in ('git remote get-url upstream 2^>nul') do set "CONFIGURED_UPSTREAM=%%R"
if not defined CONFIGURED_UPSTREAM (
    git remote add upstream "%UPSTREAM_URL%"
    if errorlevel 1 goto remote_error
) else if /i not "%CONFIGURED_UPSTREAM%"=="%UPSTREAM_URL%" (
    echo The upstream remote does not match:
    echo   %UPSTREAM_URL%
    echo Configured upstream:
    echo   %CONFIGURED_UPSTREAM%
    pause
    exit /b 1
)

for /f "delims=" %%R in ('git remote get-url origin 2^>nul') do set "ORIGIN_URL=%%R"
if not defined ORIGIN_URL (
    echo No origin remote is configured. Add your fork as origin before running this script.
    pause
    exit /b 1
)

for /f "delims=" %%B in ('git branch --show-current') do set "BRANCH=%%B"
if not defined BRANCH (
    echo Could not determine the current branch. Check out a branch before running this script.
    pause
    exit /b 1
)

for /f "delims=" %%S in ('git status --porcelain --untracked-files=all') do (
    echo(%%S| findstr /i /r /c:"^.. sync_upstream_and_build_installer\.bat$" >nul
    if errorlevel 1 set "WORKTREE_DIRTY=1"
)
if defined WORKTREE_DIRTY (
    echo The working tree is not clean. Commit or stash your changes, then retry.
    pause
    exit /b 1
)

echo Fetching upstream...
git fetch upstream
if errorlevel 1 goto fetch_error

git rev-parse --verify refs/remotes/upstream/main >nul 2>&1
if errorlevel 1 (
    echo The upstream main branch was not found after fetching.
    pause
    exit /b 1
)

echo Merging upstream/main into %BRANCH%...
git merge --no-edit upstream/main
if errorlevel 1 goto merge_error

echo Building and packing the installer...
call "%~dp0build_installer.bat"
if errorlevel 1 goto build_error

echo Pushing %BRANCH% to origin...
git push origin "%BRANCH%"
if errorlevel 1 goto push_error

echo.
echo Sync, merge, installer build, and push completed successfully.
exit /b 0

:remote_error
echo Failed to configure the upstream remote.
pause
exit /b 1

:fetch_error
echo Failed to fetch upstream. Installer packaging and push were skipped.
pause
exit /b 1

:merge_error
echo.
echo Merge did not complete. Installer packaging and push were skipped.
echo If there are conflicts, resolve them, stage the files, commit the merge, then run this script again.
pause
exit /b 1

:build_error
echo Installer build or packaging failed. Push was skipped.
pause
exit /b 1

:push_error
echo Installer was created, but pushing to origin failed.
pause
exit /b 1
