@echo off
setlocal enabledelayedexpansion

set mirael_dir=%~dp0
set lint_dir=%mirael_dir%out\lint

echo ==== Preparing to Lint ====
echo mirael root: %mirael_dir%
echo   lint root: %lint_dir%

if exist out\linting rd /s /q "%lint_dir%" || (
    echo ERROR: Failed to clear lint dir: "%lint_dir%"
    exit /b 1
)

echo ==== Configure ====
cmake --preset lint --log-level=STATUS || (
    echo ERROR: Failed to configure -- cmake returned exit code: !errorlevel!
    exit /b 2
)

echo ==== Target: mirael ====
cmake --build --preset lint --target mirael || (
    echo ERROR: Failed to build target: mirael -- cmake returned exit code: !errorlevel!
    exit /b 3
)

echo ==== Target: mirael_tests ====
cmake --build --preset lint --target mirael_tests || (
    echo ERROR: Failed to build target: mirael_tests -- cmake returned exit code: !errorlevel!
    exit /b 4
)

echo ==== Target: clang_tidy ====
cmake --build --preset lint --target clang_tidy -v || (
    echo ERROR: Failed to build target: clang_tidy -- cmake returned exit code: !errorlevel!
    exit /b 5
)

echo ==== Lint Succeeded ====
exit /b 0
