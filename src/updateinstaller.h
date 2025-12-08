#ifndef UPDATEINSTALLER_H
#define UPDATEINSTALLER_H

#include <QObject>
#include <QString>
#include <QProcess>

/**
 * UpdateInstaller - Handles extraction and installation of updates
 */
class UpdateInstaller : public QObject
{
    Q_OBJECT

public:
    explicit UpdateInstaller(QObject *parent = nullptr);
    ~UpdateInstaller();

    /**
     * Extract ZIP file and prepare for installation
     * @param zipPath - Path to downloaded ZIP file
     * @param extractPath - Where to extract (temp directory)
     * @return true if extraction successful
     */
    bool extractUpdate(const QString& zipPath, const QString& extractPath);

    /**
     * Install update by launching updater helper
     * This will close the current application and replace files
     * @param updatePath - Path containing extracted update files
     * @param currentAppPath - Current application directory
     * @param executableName - Name of the main executable
     */
    void installUpdate(const QString& updatePath, const QString& currentAppPath, const QString& executableName);

signals:
    /**
     * Emitted during extraction progress
     * @param percent - Progress percentage (0-100)
     */
    void extractionProgress(int percent);

    /**
     * Emitted when extraction completes
     */
    void extractionFinished();

    /**
     * Emitted when extraction fails
     * @param error - Error message
     */
    void extractionFailed(const QString& error);

private:
    bool extractZipWindows(const QString& zipPath, const QString& extractPath);
};

#endif // UPDATEINSTALLER_H

