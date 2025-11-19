#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QTabWidget>
#include "firebaseauthservice.h"

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    
    void setAuthService(FirebaseAuthService* authService);
    QString getUserId() const { return m_userId; }
    QString getUserEmail() const { return m_userEmail; }

private slots:
    void onSignInClicked();
    void onAuthenticationSucceeded(const QString& userId, const QString& email);
    void onAuthenticationFailed(const QString& error);

private:
    void setupUI();
    void setLoadingState(bool loading);
    void showError(const QString& error);
    void clearErrors();
    bool validateEmail(const QString& email);
    
    // UI Components
    QLineEdit* m_emailEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_signInButton;
    QLabel* m_errorLabel;
    
    // Common UI
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    
    // Auth service
    FirebaseAuthService* m_authService;
    
    // User data
    QString m_userId;
    QString m_userEmail;
};

#endif // LOGINDIALOG_H
