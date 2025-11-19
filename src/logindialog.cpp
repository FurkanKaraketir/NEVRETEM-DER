#include "logindialog.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QLoggingCategory>
#include <QApplication>
#include <QDebug>

Q_LOGGING_CATEGORY(loginLog, "auth.login")

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_emailEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_signInButton(nullptr)
    , m_errorLabel(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_authService(nullptr)
{
    setupUI();
    setModal(true);
    setWindowTitle("NEVRETEM-DER MBS - Giriş");
    setFixedSize(580, 680);
}

void LoginDialog::setAuthService(FirebaseAuthService* authService)
{
    m_authService = authService;
    
    if (m_authService) {
        connect(m_authService, &FirebaseAuthService::authenticationSucceeded,
                this, &LoginDialog::onAuthenticationSucceeded);
        connect(m_authService, &FirebaseAuthService::authenticationFailed,
                this, &LoginDialog::onAuthenticationFailed);
    }
}

void LoginDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Logo container for better placement
    QWidget* logoContainer = new QWidget();
    QVBoxLayout* logoLayout = new QVBoxLayout(logoContainer);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(0);
    
    QLabel* logoLabel = new QLabel();
    
    // Try different possible paths for the logo
    QStringList logoPaths = {
        "src/logo.jpg",
        "logo.jpg", 
        ":/logo.jpg",
        QApplication::applicationDirPath() + "/logo.jpg",
        QApplication::applicationDirPath() + "/src/logo.jpg",
        QApplication::applicationDirPath() + "/../../src/logo.jpg"
    };
    
    QPixmap logoPixmap;
    bool logoLoaded = false;
    
    for (const QString& path : logoPaths) {
        logoPixmap.load(path);
        if (!logoPixmap.isNull()) {
            logoLoaded = true;
            qDebug() << "Logo loaded successfully from:" << path;
            break;
        }
    }
    
    if (logoLoaded) {
        // Scale the logo to be more prominent
        logoPixmap = logoPixmap.scaled(240, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        logoLabel->setPixmap(logoPixmap);
        logoLabel->setStyleSheet("background-color: transparent; padding: 10px;");
    } else {
        // Fallback to text logo
        logoLabel->setText("NEVŞEHİR 2025");
        logoLabel->setStyleSheet(
            "font-size: 22px; "
            "font-weight: bold; "
            "color: #C9A962; "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E5F6F, stop:1 #2B7A8C); "
            "border-radius: 10px; "
            "padding: 30px; "
            "border: 3px solid #C9A962;"
        );
        qWarning() << "Could not load logo from any of the attempted paths";
    }
    
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setFixedSize(260, 260);
    logoLayout->addWidget(logoLabel, 0, Qt::AlignCenter);
    
    mainLayout->addWidget(logoContainer);
    mainLayout->addSpacing(10);
    
    // Title - subtle, as logo already has the text
    QLabel* titleLabel = new QLabel("Mezun Bilgi Sistemi");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 16px; "
        "font-weight: bold; "
        "color: #C9A962; "
        "letter-spacing: 1px; "
        "margin-bottom: 5px;"
    );
    mainLayout->addWidget(titleLabel);
    
    // Subtitle
    QLabel* subtitleLabel = new QLabel("Giriş Yapın");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "font-size: 13px; "
        "color: #E5D4A4; "
        "margin-bottom: 20px;"
    );
    mainLayout->addWidget(subtitleLabel);
    
    // Form layout with better styling
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setContentsMargins(20, 0, 20, 0);
    
    // Email label styling
    QLabel* emailLabel = new QLabel("E-posta:");
    emailLabel->setStyleSheet("color: #C9A962; font-weight: bold; font-size: 13px;");
    
    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText("E-postanızı girin");
    m_emailEdit->setStyleSheet(
        "QLineEdit {"
        "   background-color: rgba(255, 255, 255, 0.9);"
        "   border: 2px solid #C9A962;"
        "   border-radius: 6px;"
        "   padding: 8px 12px;"
        "   color: #1E3A5F;"
        "   font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "   border: 2px solid #FFD700;"
        "   background-color: white;"
        "}"
    );
    formLayout->addRow(emailLabel, m_emailEdit);
    
    // Password label styling
    QLabel* passwordLabel = new QLabel("Şifre:");
    passwordLabel->setStyleSheet("color: #C9A962; font-weight: bold; font-size: 13px;");
    
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Şifrenizi girin");
    m_passwordEdit->setStyleSheet(
        "QLineEdit {"
        "   background-color: rgba(255, 255, 255, 0.9);"
        "   border: 2px solid #C9A962;"
        "   border-radius: 6px;"
        "   padding: 8px 12px;"
        "   color: #1E3A5F;"
        "   font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "   border: 2px solid #FFD700;"
        "   background-color: white;"
        "}"
    );
    formLayout->addRow(passwordLabel, m_passwordEdit);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    
    // Sign in button with enhanced styling
    m_signInButton = new QPushButton("Giriş Yap");
    m_signInButton->setDefault(true);
    m_signInButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #D4AF37, stop:1 #C9A962);"
        "   color: #1E3A5F;"
        "   border: 2px solid #C9A962;"
        "   border-radius: 8px;"
        "   padding: 10px 24px;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "   min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFD700, stop:1 #D4AF37);"
        "   border: 2px solid #FFD700;"
        "}"
        "QPushButton:pressed {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B8860B, stop:1 #A67C00);"
        "}"
        "QPushButton:disabled {"
        "   background: #808080;"
        "   color: #C0C0C0;"
        "   border: 2px solid #606060;"
        "}"
    );
    connect(m_signInButton, &QPushButton::clicked, this, &LoginDialog::onSignInClicked);
    mainLayout->addWidget(m_signInButton, 0, Qt::AlignCenter);
    
    // Error label with better styling
    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet(
        "color: #FF6B6B; "
        "font-weight: bold; "
        "background-color: rgba(255, 107, 107, 0.15); "
        "border: 1px solid #FF6B6B; "
        "border-radius: 5px; "
        "padding: 8px; "
        "font-size: 12px;"
    );
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);
    
    // Progress bar with themed styling
    m_progressBar = new QProgressBar();
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #C9A962;"
        "   border-radius: 5px;"
        "   background-color: rgba(255, 255, 255, 0.2);"
        "   text-align: center;"
        "   color: #C9A962;"
        "}"
        "QProgressBar::chunk {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #C9A962, stop:1 #FFD700);"
        "   border-radius: 3px;"
        "}"
    );
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Cancel button with subtle styling
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("İptal");
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: #C9A962;"
        "   border: 2px solid #8B7355;"
        "   border-radius: 6px;"
        "   padding: 8px 20px;"
        "   font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(201, 169, 98, 0.2);"
        "   border: 2px solid #C9A962;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Enable Enter key for sign in
    connect(m_emailEdit, &QLineEdit::returnPressed, this, &LoginDialog::onSignInClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onSignInClicked);
}


void LoginDialog::onSignInClicked()
{
    qCInfo(loginLog) << "Sign-in button clicked";
    
    if (!m_authService) {
        qCCritical(loginLog) << "Authentication service not available!";
        showError("Kimlik doğrulama servisi mevcut değil");
        return;
    }
    
    clearErrors();
    
    QString email = m_emailEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    
    qCInfo(loginLog) << "Attempting sign-in for email:" << email;
    qCInfo(loginLog) << "Password length:" << password.length();
    
    if (email.isEmpty()) {
        qCWarning(loginLog) << "Email field is empty";
        showError("Lütfen e-postanızı girin");
        m_emailEdit->setFocus();
        return;
    }
    
    if (!validateEmail(email)) {
        qCWarning(loginLog) << "Invalid email format:" << email;
        showError("Lütfen geçerli bir e-posta adresi girin");
        m_emailEdit->setFocus();
        return;
    }
    
    if (password.isEmpty()) {
        qCWarning(loginLog) << "Password field is empty";
        showError("Lütfen şifrenizi girin");
        m_passwordEdit->setFocus();
        return;
    }
    
    qCInfo(loginLog) << "Input validation passed, starting authentication";
    setLoadingState(true);
    m_authService->signInWithEmailAndPassword(email, password);
}


void LoginDialog::onAuthenticationSucceeded(const QString& userId, const QString& email)
{
    qCInfo(loginLog) << "Authentication succeeded for user:" << email << "ID:" << userId;
    setLoadingState(false);
    m_userId = userId;
    m_userEmail = email;
    accept();
}

void LoginDialog::onAuthenticationFailed(const QString& error)
{
    qCCritical(loginLog) << "Authentication failed with error:" << error;
    setLoadingState(false);
    showError(QString("Giriş başarısız: %1").arg(error));
}


void LoginDialog::setLoadingState(bool loading)
{
    m_progressBar->setVisible(loading);
    if (loading) {
        m_progressBar->setRange(0, 0); // Indeterminate progress
    }
    
    m_signInButton->setEnabled(!loading);
    m_cancelButton->setEnabled(!loading);
    
    // Disable input fields during loading
    m_emailEdit->setEnabled(!loading);
    m_passwordEdit->setEnabled(!loading);
}

void LoginDialog::showError(const QString& error)
{
    m_errorLabel->setText(error);
    m_errorLabel->setVisible(true);
}

void LoginDialog::clearErrors()
{
    m_errorLabel->setVisible(false);
}

bool LoginDialog::validateEmail(const QString& email)
{
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return emailRegex.match(email).hasMatch();
}

