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
            emit checkFailed("Bu sürüm için indirilebilir bir Windows paketi bulunamadı.");
            return;
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
    // Look for a downloadable Windows update asset in release assets.
    QJsonArray assets = releaseData.value("assets").toArray();

    QString exeFallbackUrl;

    for (const QJsonValue& assetValue : assets) {
        QJsonObject asset = assetValue.toObject();
        const QString assetName = asset.value("name").toString();
        const QString assetUrl = asset.value("browser_download_url").toString();

        if (assetName.isEmpty() || assetUrl.isEmpty()) {
            continue;
        }

        // Prefer ZIP package for in-app extract/install flow.
        if (assetName.endsWith(".zip", Qt::CaseInsensitive)) {
            return assetUrl;
        }

        // Keep EXE as fallback for installer-based updates.
        if (exeFallbackUrl.isEmpty() && assetName.endsWith(".exe", Qt::CaseInsensitive)) {
            exeFallbackUrl = assetUrl;
        }
    }

    return exeFallbackUrl;
}

