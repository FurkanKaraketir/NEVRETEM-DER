#ifndef UPDATEDOWNLOADER_H
#define UPDATEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QFile>

/**
 * UpdateDownloader - Downloads update files from URLs
 */
class UpdateDownloader : public QObject
{
    Q_OBJECT

public:
    explicit UpdateDownloader(QObject *parent = nullptr);
    ~UpdateDownloader();

    /**
     * Start downloading update from URL
     * @param url - Download URL
     * @param destinationPath - Where to save the file
     */
    void startDownload(const QString& url, const QString& destinationPath);

    /**
     * Cancel current download
     */
    void cancelDownload();

signals:
    /**
     * Emitted when download progress changes
     * @param bytesReceived - Bytes downloaded so far
     * @param bytesTotal - Total bytes to download
     */
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /**
     * Emitted when download completes successfully
     * @param filePath - Path to downloaded file
     */
    void downloadFinished(const QString& filePath);

    /**
     * Emitted when download fails
     * @param error - Error message
     */
    void downloadFailed(const QString& error);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_reply;
    QFile* m_outputFile;
    QString m_destinationPath;
};

#endif // UPDATEDOWNLOADER_H

