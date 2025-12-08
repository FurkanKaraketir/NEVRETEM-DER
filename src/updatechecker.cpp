#include "updatechecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QNetworkRequest>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_silent(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReplyFinished);
}

UpdateChecker::~UpdateChecker()
{
}

void UpdateChecker::checkForUpdates(const QString& repoPath, const QString& currentVersion, bool silent)
{
    m_currentVersion = currentVersion;
    m_silent = silent;

    // GitHub API endpoint for latest release
    QString apiUrl = QString("https://api.github.com/repos/%1/releases/latest").arg(repoPath);
    
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "NEVRETEM-DER-MBS-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_networkManager->get(request);
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        // Special handling for 404 - likely no releases exist yet
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            if (!m_silent) {
                emit checkFailed("Henüz yayınlanmış bir sürüm bulunamadı.\n\nİlk sürüm için GitHub'da release oluşturmanız gerekiyor.");
            }
        } else {
            emit checkFailed(QString("Ağ hatası: %1").arg(reply->errorString()));
        }
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (!jsonDoc.isObject()) {
        emit checkFailed("Invalid response from GitHub API");
        return;
    }

    QJsonObject releaseData = jsonDoc.object();
    
    // Extract version from tag_name (usually in format "v1.0.0" or "1.0.0")
    QString tagName = releaseData.value("tag_name").toString();
    if (tagName.isEmpty()) {
        emit checkFailed("No release tag found");
        return;
    }

    // Remove 'v' prefix if present
    QString latestVersion = tagName;
    if (latestVersion.startsWith("v", Qt::CaseInsensitive)) {
        latestVersion = latestVersion.mid(1);
    }

    // Check if the latest version is newer
    if (isNewerVersion(latestVersion, m_currentVersion)) {
        QString downloadUrl = extractDownloadUrl(releaseData);
        QString releaseNotes = releaseData.value("body").toString();
        
        if (downloadUrl.isEmpty()) {
            // If no Windows installer found, use the release page URL
            downloadUrl = releaseData.value("html_url").toString();
        }

        emit updateAvailable(latestVersion, downloadUrl, releaseNotes);
    } else {
        if (!m_silent) {
            emit noUpdateAvailable();
        }
    }
}

bool UpdateChecker::isNewerVersion(const QString& latestVersion, const QString& currentVersion)
{
    QVersionNumber latest = QVersionNumber::fromString(latestVersion);
    QVersionNumber current = QVersionNumber::fromString(currentVersion);

    return latest > current;
}

QString UpdateChecker::extractDownloadUrl(const QJsonObject& releaseData)
{
    // Look for Windows installer in assets
    QJsonArray assets = releaseData.value("assets").toArray();
    
    // Priority order for Windows assets
    QStringList patterns = {
        "Setup.exe",
        "Installer.exe", 
        "setup.exe",
        "installer.exe",
        ".exe",
        ".zip"
    };

    // Try to find a Windows installer/executable
    for (const QString& pattern : patterns) {
        for (const QJsonValue& assetValue : assets) {
            QJsonObject asset = assetValue.toObject();
            QString assetName = asset.value("name").toString();
            
            if (assetName.contains(pattern, Qt::CaseInsensitive)) {
                return asset.value("browser_download_url").toString();
            }
        }
    }

    // If no specific asset found, return the first asset (if any)
    if (!assets.isEmpty()) {
        return assets.first().toObject().value("browser_download_url").toString();
    }

    return QString(); // Return empty string if no download found
}

