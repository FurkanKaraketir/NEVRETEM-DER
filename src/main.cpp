#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>
#include <QDateTime>

#include "mainwindow.h"
#include "logindialog.h"
#include "firebaseauthservice.h"
#include "thememanager.h"

// Declare logging categories
Q_LOGGING_CATEGORY(authLog, "auth")
Q_LOGGING_CATEGORY(configLog, "config")
Q_LOGGING_CATEGORY(networkLog, "network")
Q_LOGGING_CATEGORY(firestoreLog, "firestore")
Q_LOGGING_CATEGORY(dataLog, "data")

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString typeStr;
    
    switch (type) {
    case QtDebugMsg:    typeStr = "DEBUG"; break;
    case QtInfoMsg:     typeStr = "INFO "; break;
    case QtWarningMsg:  typeStr = "WARN "; break;
    case QtCriticalMsg: typeStr = "ERROR"; break;
    case QtFatalMsg:    typeStr = "FATAL"; break;
    }
    
    QString category = context.category ? QString("[%1]").arg(context.category) : "";
    QString formattedMsg = QString("%1 %2 %3 %4").arg(timestamp, typeStr, category, msg);
    
    // Output to console
    fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Install custom message handler for logging
    qInstallMessageHandler(messageOutput);
    
    qCInfo(configLog) << "=== NEVRETEM-DER MBS Starting ===";
    qCInfo(configLog) << "Application version:" << "1.0.6";
    qCInfo(configLog) << "Qt version:" << QT_VERSION_STR;
    
    // Set application properties
    app.setApplicationName("NEVRETEM-DER MBS");
    app.setApplicationVersion("1.0.6");
    app.setOrganizationName("NEVRETEM-DER");
    app.setOrganizationDomain("nevretem-der.org");
    
    // Set application icon
    QStringList iconPaths = {
        ":/logo.jpg",
        "src/logo.jpg",
        "logo.jpg", 
        QApplication::applicationDirPath() + "/logo.jpg",
        QApplication::applicationDirPath() + "/src/logo.jpg",
        QApplication::applicationDirPath() + "/../../src/logo.jpg"
    };
    
    QIcon appIcon;
    bool iconLoaded = false;
    
    for (const QString& path : iconPaths) {
        QPixmap iconPixmap(path);
        if (!iconPixmap.isNull()) {
            appIcon = QIcon(iconPixmap);
            iconLoaded = true;
            qCInfo(configLog) << "Application icon loaded from:" << path;
            break;
        }
    }
    
    if (iconLoaded) {
        app.setWindowIcon(appIcon);
    } else {
        qCWarning(configLog) << "Could not load application icon from any of the paths";
    }
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Load and apply themed stylesheet using ThemeManager
    qCInfo(configLog) << "Loading application theme...";
    ThemeManager themeManager;
    QString stylesheet = themeManager.loadThemedStylesheet(":/resources/style.qss");
    
    if (stylesheet.isEmpty()) {
        qCWarning(configLog) << "Failed to load stylesheet, using fallback theme";
        // Fallback: minimal styling if QSS fails to load
        app.setStyleSheet("QMainWindow { background-color: #0A2E3C; }");
    } else {
        app.setStyleSheet(stylesheet);
        qCInfo(configLog) << "Theme loaded successfully - Dark Cyan & Gold";
    }
    
    // Load settings from config.ini file
    // Look for config.ini in the application directory first, then current directory
    QString configPath = QApplication::applicationDirPath() + "/../../config.ini";
    qCInfo(configLog) << "Checking for config file at:" << configPath;
    if (!QFile::exists(configPath)) {
        configPath = "config.ini";
        qCInfo(configLog) << "Fallback config path:" << configPath;
    }
    
    qCInfo(configLog) << "Using config file:" << configPath;
    qCInfo(configLog) << "Config file exists:" << QFile::exists(configPath);
    
    QSettings settings(configPath, QSettings::IniFormat);
    QString projectId = settings.value("firestore/projectId", "").toString();
    QString apiKey = settings.value("firestore/apiKey", "").toString();
    
    qCInfo(configLog) << "Project ID loaded:" << (projectId.isEmpty() ? "EMPTY" : "SET");
    qCInfo(configLog) << "API Key loaded:" << (apiKey.isEmpty() ? "EMPTY" : "SET");
    if (!apiKey.isEmpty()) {
        qCInfo(configLog) << "API Key length:" << apiKey.length();
        qCInfo(configLog) << "API Key prefix:" << apiKey.left(10) + "...";
    }
    
    if (projectId.isEmpty()) {
        qCCritical(configLog) << "Firebase Project ID is not configured!";
        QString errorMsg = QString("Firebase Project ID is not configured.\n\n"
                                  "Looking for config file at: %1\n"
                                  "File exists: %2\n\n"
                                  "Please check your config.ini file.")
                                  .arg(configPath)
                                  .arg(QFile::exists(configPath) ? "Evet" : "Hayır");
        QMessageBox::critical(nullptr, "Configuration Error", errorMsg);
        return 1;
    }
    
    if (apiKey.isEmpty()) {
        qCCritical(configLog) << "Firebase API Key is not configured!";
        QMessageBox::critical(nullptr, "Configuration Error", 
                             "Firebase API Key is not configured. Please check your config.ini file.");
        return 1;
    }
    
    // Create authentication service
    qCInfo(authLog) << "Creating Firebase authentication service";
    FirebaseAuthService authService;
    authService.setProjectId(projectId);
    authService.setApiKey(apiKey);
    qCInfo(authLog) << "Authentication service configured successfully";
    
    // Show login dialog
    qCInfo(authLog) << "Showing login dialog";
    LoginDialog loginDialog;
    loginDialog.setAuthService(&authService);
    
    if (loginDialog.exec() != QDialog::Accepted) {
        qCInfo(authLog) << "User cancelled authentication or login failed";
        return 0;
    }
    
    qCInfo(authLog) << "Authentication successful, starting main application";
    
    // Authentication successful, create and show main window
    MainWindow window;
    window.setAuthService(&authService);
    window.show();
    
    return app.exec();
}
