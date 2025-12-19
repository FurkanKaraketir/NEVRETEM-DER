#include "updateinstaller.h"
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QProcess>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

UpdateInstaller::UpdateInstaller(QObject *parent)
    : QObject(parent)
{
}

UpdateInstaller::~UpdateInstaller()
{
}

bool UpdateInstaller::extractUpdate(const QString& zipPath, const QString& extractPath)
{
    emit extractionProgress(10);

#ifdef Q_OS_WIN
    return extractZipWindows(zipPath, extractPath);
#else
    emit extractionFailed("Platform not supported");
    return false;
#endif
}

bool UpdateInstaller::extractZipWindows(const QString& zipPath, const QString& extractPath)
{
#ifdef Q_OS_WIN
    // Create extract directory
    QDir dir;
    if (!dir.mkpath(extractPath)) {
        emit extractionFailed("Cannot create extraction directory");
        return false;
    }

    emit extractionProgress(30);

    // Use PowerShell to extract ZIP
    QProcess process;
    QString command = QString("powershell -NoProfile -ExecutionPolicy Bypass -Command "
                             "\"Expand-Archive -Path '%1' -DestinationPath '%2' -Force\"")
                        .arg(zipPath, extractPath);

    emit extractionProgress(50);

    process.start("cmd.exe", QStringList() << "/c" << command);
    
    if (!process.waitForStarted(5000)) {
        emit extractionFailed("Failed to start extraction process");
        return false;
    }

    emit extractionProgress(70);

    if (!process.waitForFinished(60000)) { // 60 second timeout
        emit extractionFailed("Extraction timeout");
        return false;
    }

    emit extractionProgress(90);

    if (process.exitCode() != 0) {
        emit extractionFailed(QString("Extraction failed: %1").arg(QString(process.readAllStandardError())));
        return false;
    }

    emit extractionProgress(100);
    emit extractionFinished();
    return true;
#else
    return false;
#endif
}

void UpdateInstaller::installUpdate(const QString& updatePath, const QString& currentAppPath, const QString& executableName)
{
#ifdef Q_OS_WIN
    // Check if the update files are in a subdirectory (common when extracting ZIP files)
    QString actualUpdatePath = updatePath;
    QDir updateDir(updatePath);
    QStringList entries = updateDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    // If there's only one subdirectory and no files, the content is likely in that subdirectory
    if (entries.size() == 1 && updateDir.entryList(QDir::Files).isEmpty()) {
        QString subDir = updateDir.filePath(entries.first());
        QDir subDirObj(subDir);
        // Check if the subdirectory contains files (the actual update)
        if (!subDirObj.entryList(QDir::Files).isEmpty()) {
            actualUpdatePath = subDir;
        }
    }
    
    // Create a batch file to handle the update
    QString batchPath = QDir::temp().filePath("nevretem_updater.bat");
    QFile batchFile(batchPath);
    
    if (!batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&batchFile);
    out << "@echo off\r\n";
    out << "title NEVRETEM-DER MBS Updater\r\n";
    out << "color 0A\r\n";
    out << "echo ========================================\r\n";
    out << "echo    NEVRETEM-DER MBS Updater\r\n";
    out << "echo ========================================\r\n";
    out << "echo.\r\n";
    out << "echo Waiting for application to close...\r\n";
    out << "\r\n";
    
    // Wait for the executable to be fully closed (check for up to 15 seconds)
    out << "set COUNTER=0\r\n";
    out << ":WAIT_LOOP\r\n";
    out << "tasklist /FI \"IMAGENAME eq " << executableName << "\" 2>NUL | find /I /N \"" << executableName << "\">NUL\r\n";
    out << "if \"%ERRORLEVEL%\"==\"0\" (\r\n";
    out << "    if %COUNTER% GEQ 30 (\r\n";
    out << "        echo WARNING: Application still running after 15 seconds. Proceeding anyway...\r\n";
    out << "        goto INSTALL\r\n";
    out << "    )\r\n";
    out << "    timeout /t 1 /nobreak >nul\r\n";
    out << "    set /a COUNTER+=1\r\n";
    out << "    goto WAIT_LOOP\r\n";
    out << ")\r\n";
    out << "\r\n";
    out << ":INSTALL\r\n";
    out << "echo Application closed. Installing update...\r\n";
    out << "echo.\r\n";
    out << "echo Source: " << QDir::toNativeSeparators(actualUpdatePath) << "\r\n";
    out << "echo Target: " << QDir::toNativeSeparators(currentAppPath) << "\r\n";
    out << "echo.\r\n";
    out << "\r\n";
    
    // Use robocopy which is more reliable than xcopy for this purpose
    out << "robocopy \"" << QDir::toNativeSeparators(actualUpdatePath) << "\" \"" 
        << QDir::toNativeSeparators(currentAppPath) << "\" /E /IS /IT /XO\r\n";
    out << "\r\n";
    
    // Robocopy exit codes: 0-7 are success, 8+ are errors
    out << "if %ERRORLEVEL% LEQ 7 (\r\n";
    out << "    echo.\r\n";
    out << "    echo ========================================\r\n";
    out << "    echo    Update installed successfully!\r\n";
    out << "    echo ========================================\r\n";
    out << "    echo.\r\n";
    out << "    echo Starting application...\r\n";
    out << "    timeout /t 2 /nobreak >nul\r\n";
    out << "    cd /d \"" << QDir::toNativeSeparators(currentAppPath) << "\"\r\n";
    out << "    start \"\" \"" << executableName << "\"\r\n";
    out << "    timeout /t 1 /nobreak >nul\r\n";
    out << ") else (\r\n";
    out << "    echo.\r\n";
    out << "    echo ========================================\r\n";
    out << "    echo    Update failed! Error: %ERRORLEVEL%\r\n";
    out << "    echo ========================================\r\n";
    out << "    echo.\r\n";
    out << "    echo Please try again or download manually.\r\n";
    out << "    echo.\r\n";
    out << "    pause\r\n";
    out << "    goto END\r\n";
    out << ")\r\n";
    out << "\r\n";
    out << ":END\r\n";
    out << "REM Clean up temporary files\r\n";
    out << "timeout /t 2 /nobreak >nul\r\n";
    out << "cd /d \"%TEMP%\"\r\n";
    out << "if exist \"nevretem_update.zip\" del /F /Q \"nevretem_update.zip\" 2>nul\r\n";
    out << "if exist \"nevretem_update_extracted\" rd /S /Q \"nevretem_update_extracted\" 2>nul\r\n";
    out << "(goto) 2>nul & del \"%~f0\"\r\n";
    
    batchFile.close();

    // Launch the batch file in a visible window so user can see what's happening
    QProcess::startDetached("cmd.exe", QStringList() << "/c" << "start" << "cmd.exe" << "/k" << batchPath);

    // Give it a moment to start
    QThread::msleep(500);

    // Exit the application
    QCoreApplication::quit();
#endif
}

