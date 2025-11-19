#include "firebaseauthservice.h"
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QDebug>
#include <QSettings>
#include <QLoggingCategory>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslError>

Q_LOGGING_CATEGORY(authServiceLog, "auth.service")
Q_DECLARE_LOGGING_CATEGORY(networkLog)

FirebaseAuthService::FirebaseAuthService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_refreshTimer(new QTimer(this))
{
    qCInfo(authServiceLog) << "Initializing Firebase Auth Service";
    
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &FirebaseAuthService::onNetworkReply);
    
    // Connect SSL error handler
    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            this, &FirebaseAuthService::onSslErrors);
    
    connect(m_refreshTimer, &QTimer::timeout,
            this, &FirebaseAuthService::onTokenRefreshTimer);
    
    // Set refresh timer to 50 minutes (tokens expire in 1 hour)
    m_refreshTimer->setInterval(50 * 60 * 1000);
    
    // Configure network manager timeouts
    m_networkManager->setTransferTimeout(30000); // 30 seconds timeout
    
    qCInfo(authServiceLog) << "Firebase Auth Service initialized successfully";
}

void FirebaseAuthService::setApiKey(const QString& apiKey)
{
    qCInfo(authServiceLog) << "Setting API key (length:" << apiKey.length() << ")";
    m_apiKey = apiKey;
}

void FirebaseAuthService::setProjectId(const QString& projectId)
{
    qCInfo(authServiceLog) << "Setting project ID:" << projectId;
    m_projectId = projectId;
}

QString FirebaseAuthService::buildAuthUrl(const QString& endpoint) const
{
    QString baseUrl = "https://identitytoolkit.googleapis.com/v1/accounts:";
    QString url = baseUrl + endpoint;
    
    if (!m_apiKey.isEmpty()) {
        QUrl qurl(url);
        QUrlQuery query;
        query.addQueryItem("key", m_apiKey);
        qurl.setQuery(query);
        return qurl.toString();
    }
    
    return url;
}

QNetworkRequest FirebaseAuthService::createAuthRequest(const QString& url) const
{
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Add User-Agent header
    request.setRawHeader("User-Agent", "StudentManager/1.0");
    
    // Set SSL configuration for better compatibility
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    request.setSslConfiguration(sslConfig);
    
    qCDebug(networkLog) << "Created request for URL:" << url;
    qCDebug(networkLog) << "Request headers:";
    const auto headers = request.rawHeaderList();
    for (const auto& header : headers) {
        qCDebug(networkLog) << "  " << header << ":" << request.rawHeader(header);
    }
    
    return request;
}

void FirebaseAuthService::signInWithEmailAndPassword(const QString& email, const QString& password)
{
    qCInfo(authServiceLog) << "Starting sign-in process for email:" << email;
    
    // Check network accessibility first
    if (!checkNetworkAccessibility()) {
        qCCritical(authServiceLog) << "Network is not accessible, cannot perform sign-in";
        emit authenticationFailed("Network is not accessible. Please check your internet connection.");
        return;
    }
    
    QString url = buildAuthUrl("signInWithPassword");
    qCInfo(networkLog) << "Sign-in URL:" << url;
    
    QNetworkRequest request = createAuthRequest(url);
    
    QJsonObject requestData;
    requestData["email"] = email;
    requestData["password"] = password;
    requestData["returnSecureToken"] = true;
    
    QJsonDocument doc(requestData);
    QByteArray data = doc.toJson();
    
    qCInfo(networkLog) << "Sending sign-in request, payload size:" << data.size() << "bytes";
    qCDebug(networkLog) << "Request payload:" << doc.toJson(QJsonDocument::Compact);
    
    QNetworkReply* reply = m_networkManager->post(request, data);
    m_pendingRequests[reply] = SignIn;
    
    qCInfo(authServiceLog) << "Sign-in request sent, waiting for response";
}

void FirebaseAuthService::createUserWithEmailAndPassword(const QString& email, const QString& password)
{
    // Check network accessibility first
    if (!checkNetworkAccessibility()) {
        qCCritical(authServiceLog) << "Network is not accessible, cannot create user";
        emit userCreationFailed("Network is not accessible. Please check your internet connection.");
        return;
    }
    
    QString url = buildAuthUrl("signUp");
    QNetworkRequest request = createAuthRequest(url);
    
    QJsonObject requestData;
    requestData["email"] = email;
    requestData["password"] = password;
    requestData["returnSecureToken"] = true;
    
    QJsonDocument doc(requestData);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = m_networkManager->post(request, data);
    m_pendingRequests[reply] = SignUp;
}

void FirebaseAuthService::refreshToken()
{
    if (m_refreshToken.isEmpty()) {
        emit tokenRefreshFailed("No refresh token available");
        return;
    }
    
    QString url = QString("https://securetoken.googleapis.com/v1/token?key=%1").arg(m_apiKey);
    QNetworkRequest request = createAuthRequest(url);
    
    QJsonObject requestData;
    requestData["grant_type"] = "refresh_token";
    requestData["refresh_token"] = m_refreshToken;
    
    QJsonDocument doc(requestData);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = m_networkManager->post(request, data);
    m_pendingRequests[reply] = RefreshToken;
}

void FirebaseAuthService::signOut()
{
    clearAuthData();
    stopTokenRefresh();
    emit signedOut();
}

void FirebaseAuthService::startTokenRefresh()
{
    if (isAuthenticated()) {
        m_refreshTimer->start();
    }
}

void FirebaseAuthService::stopTokenRefresh()
{
    m_refreshTimer->stop();
}

void FirebaseAuthService::onNetworkReply(QNetworkReply* reply)
{
    if (!reply) {
        qCWarning(networkLog) << "Received null reply in onNetworkReply";
        return;
    }
    
    qCInfo(networkLog) << "Received network reply, status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qCInfo(networkLog) << "Reply error:" << reply->error() << reply->errorString();
    qCInfo(networkLog) << "Reply URL:" << reply->url().toString();
    qCInfo(networkLog) << "Reply headers:";
    
    // Log all response headers for debugging
    const auto headers = reply->rawHeaderPairs();
    for (const auto& header : headers) {
        qCDebug(networkLog) << "  " << header.first << ":" << header.second;
    }
    
    reply->deleteLater();
    
    if (!m_pendingRequests.contains(reply)) {
        qCWarning(networkLog) << "Reply not found in pending requests";
        return;
    }
    
    RequestType requestType = m_pendingRequests.take(reply);
    QString requestTypeStr = (requestType == SignIn) ? "SignIn" : 
                           (requestType == SignUp) ? "SignUp" : "RefreshToken";
    
    qCInfo(networkLog) << "Processing" << requestTypeStr << "response";
    
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        qCWarning(networkLog) << "Network error occurred, response data:" << data;
        
        QString errorMessage = reply->errorString();
        
        // Handle specific network errors
        switch (reply->error()) {
        case QNetworkReply::TimeoutError:
            errorMessage = "Request timed out. Please check your internet connection and try again.";
            qCWarning(networkLog) << "Request timed out";
            break;
        case QNetworkReply::ConnectionRefusedError:
            errorMessage = "Connection refused. Please check your internet connection.";
            qCWarning(networkLog) << "Connection refused";
            break;
        case QNetworkReply::HostNotFoundError:
            errorMessage = "Host not found. Please check your internet connection.";
            qCWarning(networkLog) << "Host not found";
            break;
        case QNetworkReply::SslHandshakeFailedError:
            errorMessage = "SSL handshake failed. Please check your system's SSL configuration.";
            qCWarning(networkLog) << "SSL handshake failed";
            break;
        case QNetworkReply::NetworkSessionFailedError:
            errorMessage = "Network session failed. Please check your internet connection.";
            qCWarning(networkLog) << "Network session failed";
            break;
        default:
            // Try to parse Firebase-specific error from response
            if (!data.isEmpty()) {
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                
                if (error.error == QJsonParseError::NoError) {
                    QJsonObject errorObj = doc.object();
                    qCDebug(networkLog) << "Error response JSON:" << doc.toJson(QJsonDocument::Compact);
                    if (errorObj.contains("error")) {
                        QJsonObject errorDetails = errorObj["error"].toObject();
                        QString firebaseError = errorDetails["message"].toString();
                        if (!firebaseError.isEmpty()) {
                            errorMessage = firebaseError;
                        }
                        qCWarning(authServiceLog) << "Firebase error message:" << firebaseError;
                        if (errorDetails.contains("code")) {
                            qCWarning(authServiceLog) << "Firebase error code:" << errorDetails["code"].toInt();
                        }
                    }
                } else {
                    qCWarning(networkLog) << "Failed to parse error response JSON:" << error.errorString();
                }
            }
            break;
        }
        
        qCCritical(authServiceLog) << requestTypeStr << "failed with error:" << errorMessage;
        
        switch (requestType) {
        case SignIn:
            emit authenticationFailed(errorMessage);
            break;
        case SignUp:
            emit userCreationFailed(errorMessage);
            break;
        case RefreshToken:
            emit tokenRefreshFailed(errorMessage);
            break;
        }
        return;
    }
    
    qCInfo(networkLog) << requestTypeStr << "request completed successfully";
    
    switch (requestType) {
    case SignIn:
        handleSignInReply(reply);
        break;
    case SignUp:
        handleSignUpReply(reply);
        break;
    case RefreshToken:
        handleRefreshTokenReply(reply);
        break;
    }
}

void FirebaseAuthService::onTokenRefreshTimer()
{
    refreshToken();
}

void FirebaseAuthService::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    qCWarning(networkLog) << "SSL errors occurred for URL:" << reply->url().toString();
    for (const QSslError& error : errors) {
        qCWarning(networkLog) << "SSL Error:" << error.errorString();
    }
    
    // For debugging purposes, you might want to ignore SSL errors temporarily
    // CAUTION: This should only be used for debugging, never in production!
    // reply->ignoreSslErrors();
}

void FirebaseAuthService::handleSignInReply(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    qCInfo(networkLog) << "Sign-in response data size:" << data.size() << "bytes";
    qCDebug(networkLog) << "Sign-in response data:" << data;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCCritical(authServiceLog) << "JSON parse error in sign-in response:" << error.errorString();
        emit authenticationFailed(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    QJsonObject response = doc.object();
    qCDebug(authServiceLog) << "Parsed sign-in response JSON:" << doc.toJson(QJsonDocument::Compact);
    
    parseAuthResponse(response);
    
    qCInfo(authServiceLog) << "Sign-in successful for user:" << m_userEmail << "ID:" << m_userId;
    emit authenticationSucceeded(m_userId, m_userEmail);
    startTokenRefresh();
}

void FirebaseAuthService::handleSignUpReply(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit userCreationFailed(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    QJsonObject response = doc.object();
    parseAuthResponse(response);
    
    emit userCreated(m_userId, m_userEmail);
    startTokenRefresh();
}

void FirebaseAuthService::handleRefreshTokenReply(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit tokenRefreshFailed(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    QJsonObject response = doc.object();
    
    // Update tokens
    m_idToken = response["id_token"].toString();
    m_refreshToken = response["refresh_token"].toString();
    m_userId = response["user_id"].toString();
    
    emit tokenRefreshed();
}

void FirebaseAuthService::parseAuthResponse(const QJsonObject& response)
{
    m_idToken = response["idToken"].toString();
    m_refreshToken = response["refreshToken"].toString();
    m_userId = response["localId"].toString();
    m_userEmail = response["email"].toString();
}

void FirebaseAuthService::clearAuthData()
{
    m_idToken.clear();
    m_refreshToken.clear();
    m_userId.clear();
    m_userEmail.clear();
}

bool FirebaseAuthService::checkNetworkAccessibility() const
{
    // For now, we'll assume network is accessible and rely on the actual request to fail
    // if there are network issues. This is more compatible across Qt versions.
    qCInfo(networkLog) << "Assuming network is accessible (will be verified during request)";
    return true;
}
