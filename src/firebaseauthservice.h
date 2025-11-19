#ifndef FIREBASEAUTHSERVICE_H
#define FIREBASEAUTHSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

class FirebaseAuthService : public QObject
{
    Q_OBJECT

public:
    explicit FirebaseAuthService(QObject *parent = nullptr);
    
    void setApiKey(const QString& apiKey);
    void setProjectId(const QString& projectId);
    
    // Authentication methods
    void signInWithEmailAndPassword(const QString& email, const QString& password);
    void createUserWithEmailAndPassword(const QString& email, const QString& password);
    void signOut();
    void refreshToken();
    
    // Token management
    QString getIdToken() const { return m_idToken; }
    QString getRefreshToken() const { return m_refreshToken; }
    QString getUserId() const { return m_userId; }
    QString getUserEmail() const { return m_userEmail; }
    bool isAuthenticated() const { return !m_idToken.isEmpty(); }
    
    // Auto-refresh setup
    void startTokenRefresh();
    void stopTokenRefresh();

signals:
    void authenticationSucceeded(const QString& userId, const QString& email);
    void authenticationFailed(const QString& error);
    void userCreated(const QString& userId, const QString& email);
    void userCreationFailed(const QString& error);
    void tokenRefreshed();
    void tokenRefreshFailed(const QString& error);
    void signedOut();

private slots:
    void onNetworkReply(QNetworkReply* reply);
    void onTokenRefreshTimer();
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

private:
    QString buildAuthUrl(const QString& endpoint) const;
    QNetworkRequest createAuthRequest(const QString& url) const;
    void handleSignInReply(QNetworkReply* reply);
    void handleSignUpReply(QNetworkReply* reply);
    void handleRefreshTokenReply(QNetworkReply* reply);
    void parseAuthResponse(const QJsonObject& response);
    void clearAuthData();
    bool checkNetworkAccessibility() const;
    
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_projectId;
    QString m_idToken;
    QString m_refreshToken;
    QString m_userId;
    QString m_userEmail;
    QTimer* m_refreshTimer;
    
    enum RequestType {
        SignIn,
        SignUp,
        RefreshToken
    };
    
    QHash<QNetworkReply*, RequestType> m_pendingRequests;
};

#endif // FIREBASEAUTHSERVICE_H
