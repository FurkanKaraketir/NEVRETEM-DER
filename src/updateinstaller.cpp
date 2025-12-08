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
    // Create a batch file to handle the update
    QString batchPath = QDir::temp().filePath("nevretem_updater.bat");
    QFile batchFile(batchPath);
    
    if (!batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&batchFile);
    out << "@echo off\r\n";
    out << "echo NEVRETEM-DER MBS Updater\r\n";
    out << "echo Waiting for application to close...\r\n";
    out << "timeout /t 2 /nobreak >nul\r\n";
    out << "\r\n";
    out << "echo Installing update...\r\n";
    out << "xcopy /E /I /Y \"" << QDir::toNativeSeparators(updatePath) << "\\*\" \"" 
        << QDir::toNativeSeparators(currentAppPath) << "\"\r\n";
    out << "\r\n";
    out << "if %ERRORLEVEL% EQU 0 (\r\n";
    out << "    echo Update installed successfully!\r\n";
    out << "    timeout /t 2 /nobreak >nul\r\n";
    out << "    cd /d \"" << QDir::toNativeSeparators(currentAppPath) << "\"\r\n";
    out << "    start \"\" \"" << executableName << "\"\r\n";
    out << ") else (\r\n";
    out << "    echo Update failed!\r\n";
    out << "    pause\r\n";
    out << ")\r\n";
    out << "\r\n";
    out << "REM Clean up\r\n";
    out << "del \"%~f0\"\r\n";
    
    batchFile.close();

    // Launch the batch file
    QProcess::startDetached("cmd.exe", QStringList() << "/c" << batchPath);

    // Give it a moment to start
    QThread::msleep(500);

    // Exit the application
    QCoreApplication::quit();
#endif
}

