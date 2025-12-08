#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QVersionNumber>

/**
 * UpdateChecker - Checks for application updates from GitHub Releases
 * 
 * Usage:
 *   UpdateChecker* checker = new UpdateChecker(this);
 *   checker->checkForUpdates("username/repo", "1.0.0");
 *   connect(checker, &UpdateChecker::updateAvailable, ...);
 */
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();

    /**
     * Check for updates on GitHub
     * @param repoPath - GitHub repository path (e.g., "username/repository")
     * @param currentVersion - Current application version (e.g., "1.0.0")
     * @param silent - If true, only show message if update is available
     */
    void checkForUpdates(const QString& repoPath, const QString& currentVersion, bool silent = false);

signals:
    /**
     * Emitted when a new version is available
     * @param newVersion - The new version number
     * @param downloadUrl - URL to download the new version
     * @param releaseNotes - Release notes/changelog
     */
    void updateAvailable(const QString& newVersion, const QString& downloadUrl, const QString& releaseNotes);

    /**
     * Emitted when no update is available (only if not silent)
     */
    void noUpdateAvailable();

    /**
     * Emitted when an error occurs during the update check
     * @param error - Error message
     */
    void checkFailed(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_currentVersion;
    bool m_silent;

    bool isNewerVersion(const QString& latestVersion, const QString& currentVersion);
    QString extractDownloadUrl(const QJsonObject& releaseData);
};

#endif // UPDATECHECKER_H

