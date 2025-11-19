#include "firebasestorageservice.h"
#include <QUuid>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

Q_LOGGING_CATEGORY(storageLog, "firebase.storage")

FirebaseStorageService::FirebaseStorageService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished, 
            this, &FirebaseStorageService::onNetworkReply);
    
    qCInfo(storageLog) << "Firebase Storage service initialized";
}

void FirebaseStorageService::setProjectId(const QString& projectId)
{
    m_projectId = projectId;
    m_baseUrl = QString("https://firebasestorage.googleapis.com/v0/b/%1.appspot.com/o").arg(projectId);
    qCInfo(storageLog) << "Project ID set:" << projectId;
}

void FirebaseStorageService::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
    qCInfo(storageLog) << "API key configured";
}

void FirebaseStorageService::setAuthToken(const QString& authToken)
{
    m_authToken = authToken;
    qCDebug(storageLog) << "Auth token updated";
}

void FirebaseStorageService::uploadFile(const QString& localFilePath, const QString& storagePath)
{
    qCInfo(storageLog) << "=== Starting file upload ===";
    qCInfo(storageLog) << "Local file:" << localFilePath;
    qCInfo(storageLog) << "Storage path:" << storagePath;
    qCDebug(storageLog) << "Project ID:" << m_projectId;
    qCDebug(storageLog) << "Has API key:" << !m_apiKey.isEmpty();
    qCDebug(storageLog) << "Has auth token:" << !m_authToken.isEmpty();
    if (!m_authToken.isEmpty()) {
        qCDebug(storageLog) << "Auth token (first 20 chars):" << m_authToken.left(20) + "...";
    }
    
    QFileInfo fileInfo(localFilePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        QString error = QString("File does not exist or is not readable: %1").arg(localFilePath);
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
        return;
    }
    
    // Determine content type
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fileInfo);
    QString contentType = mimeType.name();
    
    qCDebug(storageLog) << "File size:" << fileInfo.size() << "bytes";
    qCDebug(storageLog) << "Content type:" << contentType;
    
    // Create unique storage path if needed
    QString finalStoragePath = storagePath;
    if (finalStoragePath.isEmpty()) {
        finalStoragePath = QString("student_photos/%1").arg(generateUniqueFileName(fileInfo.fileName()));
    }
    
    qCInfo(storageLog) << "Final storage path:" << finalStoragePath;
    
    // Build upload URL
    QString uploadUrl = buildUploadUrl(finalStoragePath);
    qCDebug(storageLog) << "Upload URL:" << uploadUrl;
    
    // Create request
    QNetworkRequest request = createUploadRequest(uploadUrl, contentType);
    
    // Read file data
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = QString("Failed to open file for reading: %1").arg(localFilePath);
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
        return;
    }
    
    QByteArray fileData = file.readAll();
    file.close();
    
    qCInfo(storageLog) << "Sending upload request...";
    QNetworkReply* reply = m_networkManager->post(request, fileData);
    
    // Track upload progress
    connect(reply, &QNetworkReply::uploadProgress, 
            this, &FirebaseStorageService::onUploadProgress);
    
    m_pendingRequests[reply] = UploadFile;
    m_requestPaths[reply] = finalStoragePath;
    
    qCDebug(storageLog) << "Upload request sent, waiting for response...";
}

void FirebaseStorageService::deleteFile(const QString& storagePath)
{
    qCInfo(storageLog) << "=== Deleting file ===";
    qCInfo(storageLog) << "Storage path:" << storagePath;
    
    QString deleteUrl = buildMetadataUrl(storagePath);
    QNetworkRequest request = createMetadataRequest(deleteUrl);
    
    qCInfo(storageLog) << "Sending delete request...";
    QNetworkReply* reply = m_networkManager->deleteResource(request);
    
    m_pendingRequests[reply] = DeleteFile;
    m_requestPaths[reply] = storagePath;
}

void FirebaseStorageService::getDownloadUrl(const QString& storagePath)
{
    qCInfo(storageLog) << "=== Getting download URL ===";
    qCInfo(storageLog) << "Storage path:" << storagePath;
    
    QString metadataUrl = buildMetadataUrl(storagePath);
    QNetworkRequest request = createMetadataRequest(metadataUrl);
    
    qCInfo(storageLog) << "Sending metadata request...";
    QNetworkReply* reply = m_networkManager->get(request);
    
    m_pendingRequests[reply] = GetDownloadUrl;
    m_requestPaths[reply] = storagePath;
}

void FirebaseStorageService::loadImage(const QString& imageUrl)
{
    qCInfo(storageLog) << "=== Loading image ===";
    qCInfo(storageLog) << "Original Image URL:" << imageUrl;
    
    // Fix malformed URLs before making the request
    QString fixedUrl = fixMalformedUrl(imageUrl);
    qCInfo(storageLog) << "Fixed Image URL:" << fixedUrl;
    
    QNetworkRequest request(fixedUrl);
    
    // Add authentication header if available
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
        qCDebug(storageLog) << "Added Authorization header for image request";
    } else {
        qCWarning(storageLog) << "No auth token available for image request";
    }
    
    qCInfo(storageLog) << "Sending image request...";
    QNetworkReply* reply = m_networkManager->get(request);
    
    m_pendingRequests[reply] = LoadImage;
    m_requestPaths[reply] = imageUrl; // Store the original URL for reference
}

QString FirebaseStorageService::buildUploadUrl(const QString& storagePath) const
{
    QString encodedPath = QUrl::toPercentEncoding(storagePath);
    QString url = QString("%1/%2").arg(m_baseUrl, encodedPath);
    
    QUrlQuery query;
    if (!m_apiKey.isEmpty()) {
        query.addQueryItem("key", m_apiKey);
    }
    
    if (!query.isEmpty()) {
        url += "?" + query.toString();
    }
    
    return url;
}

QString FirebaseStorageService::buildMetadataUrl(const QString& storagePath) const
{
    QString encodedPath = QUrl::toPercentEncoding(storagePath);
    QString url = QString("%1/%2").arg(m_baseUrl, encodedPath);
    
    QUrlQuery query;
    if (!m_apiKey.isEmpty()) {
        query.addQueryItem("key", m_apiKey);
    }
    
    if (!query.isEmpty()) {
        url += "?" + query.toString();
    }
    
    return url;
}

QString FirebaseStorageService::buildDownloadUrl(const QString& storagePath, const QString& token) const
{
    QString encodedPath = QUrl::toPercentEncoding(storagePath);
    QString url = QString("%1/%2").arg(m_baseUrl, encodedPath);
    
    QUrlQuery query;
    query.addQueryItem("alt", "media");
    query.addQueryItem("token", token);
    
    url += "?" + query.toString();
    
    return url;
}

QNetworkRequest FirebaseStorageService::createUploadRequest(const QString& url, const QString& contentType) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
        qCDebug(storageLog) << "Added Authorization header with token";
    } else {
        qCWarning(storageLog) << "No auth token available for upload request";
    }
    
    qCDebug(storageLog) << "Upload request URL:" << url;
    qCDebug(storageLog) << "Content-Type:" << contentType;
    
    return request;
}

QNetworkRequest FirebaseStorageService::createMetadataRequest(const QString& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }
    
    return request;
}

void FirebaseStorageService::onNetworkReply(QNetworkReply* reply)
{
    reply->deleteLater();
    
    if (!m_pendingRequests.contains(reply)) {
        qCWarning(storageLog) << "Received reply for unknown request";
        return;
    }
    
    RequestType requestType = m_pendingRequests.take(reply);
    QString storagePath = m_requestPaths.take(reply);
    
    qCDebug(storageLog) << "=== Processing network reply ===";
    qCDebug(storageLog) << "Request type:" << requestType;
    qCDebug(storageLog) << "Storage path:" << storagePath;
    qCDebug(storageLog) << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QString error = QString("Network error: %1").arg(reply->errorString());
        qCCritical(storageLog) << error;
        qCCritical(storageLog) << "Response body:" << responseData;
        
        // Try to parse Firebase error message
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            QJsonObject errorObj = doc.object();
            if (errorObj.contains("error")) {
                QJsonObject errorDetails = errorObj["error"].toObject();
                QString firebaseError = errorDetails["message"].toString();
                if (!firebaseError.isEmpty()) {
                    error = QString("Firebase Storage error: %1").arg(firebaseError);
                }
            }
        }
        
        emit errorOccurred(error);
        return;
    }
    
    switch (requestType) {
        case UploadFile:
            handleUploadReply(reply, storagePath);
            break;
        case DeleteFile:
            handleDeleteReply(reply, storagePath);
            break;
        case GetDownloadUrl:
            handleDownloadUrlReply(reply, storagePath);
            break;
        case LoadImage:
            handleImageLoadReply(reply, storagePath);
            break;
    }
}

void FirebaseStorageService::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply && m_requestPaths.contains(reply)) {
        QString storagePath = m_requestPaths[reply];
        emit uploadProgress(storagePath, bytesSent, bytesTotal);
        
        if (bytesTotal > 0) {
            int percentage = (bytesSent * 100) / bytesTotal;
            qCDebug(storageLog) << "Upload progress for" << storagePath << ":" << percentage << "%";
        }
    }
}

void FirebaseStorageService::handleUploadReply(QNetworkReply* reply, const QString& storagePath)
{
    qCInfo(storageLog) << "=== Handling upload reply ===";
    
    QByteArray responseData = reply->readAll();
    qCDebug(storageLog) << "Response data:" << responseData;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("Failed to parse upload response: %1").arg(parseError.errorString());
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
        return;
    }
    
    QJsonObject response = doc.object();
    
    // Extract download URL from response
    QString downloadUrl;
    if (response.contains("downloadTokens")) {
        QString token = response["downloadTokens"].toString();
        downloadUrl = buildDownloadUrl(storagePath, token);
    } else {
        // Fallback: try to get download URL via separate request
        getDownloadUrl(storagePath);
        return;
    }
    
    qCInfo(storageLog) << "File uploaded successfully";
    qCDebug(storageLog) << "Download URL:" << downloadUrl;
    
    emit fileUploaded(storagePath, downloadUrl);
}

void FirebaseStorageService::handleDeleteReply(QNetworkReply* reply, const QString& storagePath)
{
    qCInfo(storageLog) << "=== Handling delete reply ===";
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (statusCode == 204) { // No Content - successful deletion
        qCInfo(storageLog) << "File deleted successfully:" << storagePath;
        emit fileDeleted(storagePath);
    } else {
        QByteArray responseData = reply->readAll();
        QString error = QString("Failed to delete file: HTTP %1 - %2").arg(statusCode).arg(QString(responseData));
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
    }
}

void FirebaseStorageService::handleDownloadUrlReply(QNetworkReply* reply, const QString& storagePath)
{
    qCInfo(storageLog) << "=== Handling download URL reply ===";
    
    QByteArray responseData = reply->readAll();
    qCDebug(storageLog) << "Response data:" << responseData;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("Failed to parse metadata response: %1").arg(parseError.errorString());
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
        return;
    }
    
    QJsonObject response = doc.object();
    
    // Extract download URL
    QString downloadUrl;
    if (response.contains("downloadTokens")) {
        QString token = response["downloadTokens"].toString();
        downloadUrl = buildDownloadUrl(storagePath, token);
    } else if (response.contains("mediaLink")) {
        downloadUrl = response["mediaLink"].toString();
    }
    
    if (downloadUrl.isEmpty()) {
        QString error = "No download URL found in metadata response";
        qCCritical(storageLog) << error;
        emit errorOccurred(error);
        return;
    }
    
    qCInfo(storageLog) << "Download URL retrieved successfully";
    qCDebug(storageLog) << "Download URL:" << downloadUrl;
    
    emit downloadUrlReceived(storagePath, downloadUrl);
}

void FirebaseStorageService::handleImageLoadReply(QNetworkReply* reply, const QString& imageUrl)
{
    qCInfo(storageLog) << "=== Handling image load reply ===";
    
    QByteArray imageData = reply->readAll();
    qCDebug(storageLog) << "Image data size:" << imageData.size() << "bytes";
    
    // Log first 200 characters to see what we actually received
    if (imageData.size() > 0) {
        QString preview = QString::fromUtf8(imageData.left(200));
        qCDebug(storageLog) << "Image data preview:" << preview;
    }
    
    if (imageData.isEmpty()) {
        QString error = "No image data received";
        qCCritical(storageLog) << error;
        emit imageLoadFailed(imageUrl, error);
        return;
    }
    
    // Check if this looks like an error response (HTML/JSON) rather than image data
    if (imageData.startsWith("<!DOCTYPE") || imageData.startsWith("<html") || 
        imageData.startsWith("{\"error\"") || imageData.startsWith("<?xml")) {
        QString error = QString("Received error response instead of image data: %1").arg(QString::fromUtf8(imageData.left(100)));
        qCCritical(storageLog) << error;
        emit imageLoadFailed(imageUrl, error);
        return;
    }
    
    qCInfo(storageLog) << "Image loaded successfully";
    emit imageLoaded(imageUrl, imageData);
}

QString FirebaseStorageService::generateUniqueFileName(const QString& originalFileName) const
{
    QFileInfo fileInfo(originalFileName);
    QString baseName = fileInfo.baseName();
    QString extension = fileInfo.completeSuffix();
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    if (extension.isEmpty()) {
        return QString("%1_%2").arg(baseName, uuid);
    } else {
        return QString("%1_%2.%3").arg(baseName, uuid, extension);
    }
}

QString FirebaseStorageService::fixMalformedUrl(const QString& url) const
{
    if (url.isEmpty()) {
        return url;
    }
    
    // Check for duplicate query separators (e.g., "?key=...?alt=media")
    int firstQuestionMark = url.indexOf('?');
    if (firstQuestionMark == -1) {
        return url; // No query parameters, URL is fine
    }
    
    int secondQuestionMark = url.indexOf('?', firstQuestionMark + 1);
    if (secondQuestionMark == -1) {
        return url; // Only one question mark, URL is fine
    }
    
    // Found duplicate question marks, fix by replacing the second one with &
    QString fixedUrl = url;
    fixedUrl[secondQuestionMark] = '&';
    
    qCInfo(storageLog) << "Fixed malformed URL - replaced second '?' with '&'";
    
    return fixedUrl;
}
