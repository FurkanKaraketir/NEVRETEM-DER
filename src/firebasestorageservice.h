#ifndef FIREBASESTORAGESERVICE_H
#define FIREBASESTORAGESERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFileInfo>
#include <QMimeDatabase>

Q_DECLARE_LOGGING_CATEGORY(storageLog)

class FirebaseStorageService : public QObject
{
    Q_OBJECT

public:
    explicit FirebaseStorageService(QObject *parent = nullptr);
    
    void setProjectId(const QString& projectId);
    void setApiKey(const QString& apiKey);
    void setAuthToken(const QString& authToken);
    
    // Storage operations
    void uploadFile(const QString& localFilePath, const QString& storagePath);
    void deleteFile(const QString& storagePath);
    void getDownloadUrl(const QString& storagePath);
    void loadImage(const QString& imageUrl);

signals:
    void fileUploaded(const QString& storagePath, const QString& downloadUrl);
    void fileDeleted(const QString& storagePath);
    void downloadUrlReceived(const QString& storagePath, const QString& downloadUrl);
    void imageLoaded(const QString& imageUrl, const QByteArray& imageData);
    void imageLoadFailed(const QString& imageUrl, const QString& error);
    void uploadProgress(const QString& storagePath, qint64 bytesSent, qint64 bytesTotal);
    void errorOccurred(const QString& error);

private slots:
    void onNetworkReply(QNetworkReply* reply);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QString buildUploadUrl(const QString& storagePath) const;
    QString buildMetadataUrl(const QString& storagePath) const;
    QString buildDownloadUrl(const QString& storagePath, const QString& token) const;
    QNetworkRequest createUploadRequest(const QString& url, const QString& contentType) const;
    QNetworkRequest createMetadataRequest(const QString& url) const;
    void handleUploadReply(QNetworkReply* reply, const QString& storagePath);
    void handleDeleteReply(QNetworkReply* reply, const QString& storagePath);
    void handleDownloadUrlReply(QNetworkReply* reply, const QString& storagePath);
    void handleImageLoadReply(QNetworkReply* reply, const QString& imageUrl);
    QString generateUniqueFileName(const QString& originalFileName) const;
    QString fixMalformedUrl(const QString& url) const;
    
    QNetworkAccessManager* m_networkManager;
    QString m_projectId;
    QString m_apiKey;
    QString m_authToken;
    QString m_baseUrl;
    
    enum RequestType {
        UploadFile,
        DeleteFile,
        GetDownloadUrl,
        LoadImage
    };
    
    QHash<QNetworkReply*, RequestType> m_pendingRequests;
    QHash<QNetworkReply*, QString> m_requestPaths; // For tracking storage paths
};

#endif // FIREBASESTORAGESERVICE_H
