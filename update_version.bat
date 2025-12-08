@echo off
REM Script to update version number across all project files
REM Usage: update_version.bat NEW_VERSION
REM Example: update_version.bat 1.0.1

if "%~1"=="" (
    echo Usage: update_version.bat NEW_VERSION
    echo Example: update_version.bat 1.0.1
    pause
    exit /b 1
)

set NEW_VERSION=%~1
echo Updating version to %NEW_VERSION%...
echo.

REM Use PowerShell for text replacement
powershell -Command "(Get-Content CMakeLists.txt) -replace 'project\(StudentManager VERSION [0-9]+\.[0-9]+\.[0-9]+\)', 'project(StudentManager VERSION %NEW_VERSION%)' | Set-Content CMakeLists.txt"
echo Updated CMakeLists.txt

powershell -Command "(Get-Content src/main.cpp) -replace 'qCInfo\(configLog\) << \"Application version:\" << \"[0-9]+\.[0-9]+\.[0-9]+\"', 'qCInfo(configLog) << \"Application version:\" << \"%NEW_VERSION%\"' | Set-Content src/main.cpp"
powershell -Command "(Get-Content src/main.cpp) -replace 'app\.setApplicationVersion\(\"[0-9]+\.[0-9]+\.[0-9]+\"\)', 'app.setApplicationVersion(\"%NEW_VERSION%\")' | Set-Content src/main.cpp"
echo Updated src/main.cpp

powershell -Command "(Get-Content create_installer.nsi) -replace '!define APP_VERSION \"[0-9]+\.[0-9]+\.[0-9]+\"', '!define APP_VERSION \"%NEW_VERSION%\"' | Set-Content create_installer.nsi"
powershell -Command "(Get-Content create_installer.nsi) -replace 'VIProductVersion \"[0-9]+\.[0-9]+\.[0-9]+\.0\"', 'VIProductVersion \"%NEW_VERSION%.0\"' | Set-Content create_installer.nsi"
echo Updated create_installer.nsi

echo.
echo Version updated to %NEW_VERSION% in all files!
echo.
echo Next steps:
echo 1. Review the changes
echo 2. Commit: git add . ^&^& git commit -m "Bump version to %NEW_VERSION%"
echo 3. Tag: git tag v%NEW_VERSION%
echo 4. Push: git push origin main ^&^& git push origin v%NEW_VERSION%
echo.
pause

