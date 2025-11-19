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
    qCInfo(configLog) << "Application version:" << "1.0.0";
    qCInfo(configLog) << "Qt version:" << QT_VERSION_STR;
    
    // Set application properties
    app.setApplicationName("NEVRETEM-DER MBS");
    app.setApplicationVersion("1.0.0");
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
    
    // Apply elegant theme inspired by the logo
    QPalette elegantPalette;
    elegantPalette.setColor(QPalette::Window, QColor(30, 58, 95)); // Deep blue from logo
    elegantPalette.setColor(QPalette::WindowText, QColor(212, 175, 55)); // Gold from logo
    elegantPalette.setColor(QPalette::Base, QColor(20, 40, 70)); // Darker blue
    elegantPalette.setColor(QPalette::AlternateBase, QColor(35, 65, 105)); // Lighter blue
    elegantPalette.setColor(QPalette::ToolTipBase, QColor(212, 175, 55));
    elegantPalette.setColor(QPalette::ToolTipText, QColor(30, 58, 95));
    elegantPalette.setColor(QPalette::Text, QColor(212, 175, 55)); // Gold text
    elegantPalette.setColor(QPalette::Button, QColor(44, 90, 160)); // Medium blue
    elegantPalette.setColor(QPalette::ButtonText, QColor(212, 175, 55)); // Gold text
    elegantPalette.setColor(QPalette::BrightText, QColor(255, 215, 0)); // Bright gold
    elegantPalette.setColor(QPalette::Link, QColor(184, 134, 11)); // Darker gold
    elegantPalette.setColor(QPalette::Highlight, QColor(212, 175, 55)); // Gold highlight
    elegantPalette.setColor(QPalette::HighlightedText, QColor(30, 58, 95)); // Blue text on gold
    app.setPalette(elegantPalette);
    
    // Apply elegant stylesheet inspired by the logo
    app.setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            border: 2px solid #D4AF37;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 12px;
            color: #D4AF37;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 15px;
            padding: 0 8px 0 8px;
            color: #D4AF37;
            font-weight: bold;
        }
        
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #2C5AA0, stop:1 #1E3A5F);
            border: 2px solid #D4AF37;
            border-radius: 6px;
            padding: 8px 16px;
            min-width: 90px;
            color: #D4AF37;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #3A6BB0, stop:1 #2A4A7F);
            border-color: #FFD700;
            color: #FFD700;
        }
        
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1E3A5F, stop:1 #142A4F);
        }
        
        QPushButton:disabled {
            background-color: #2A2A2A;
            color: #666666;
            border-color: #444444;
        }
        
        QLineEdit, QTextEdit, QSpinBox, QComboBox {
            background-color: #142A4F;
            border: 2px solid #2C5AA0;
            border-radius: 5px;
            padding: 6px;
            color: #D4AF37;
            selection-background-color: #D4AF37;
            selection-color: #1E3A5F;
        }
        
        QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QComboBox:focus {
            border-color: #D4AF37;
            background-color: #1E3A5F;
        }
        
        QTableWidget {
            gridline-color: #2C5AA0;
            selection-background-color: #D4AF37;
            selection-color: #1E3A5F;
            alternate-background-color: #233A65;
        }
        
        QTableWidget::item {
            padding: 6px;
            color: #D4AF37;
        }
        
        QTableWidget::item:selected {
            background-color: #D4AF37;
            color: #1E3A5F;
        }
        
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #2C5AA0, stop:1 #1E3A5F);
            padding: 8px;
            border: 1px solid #D4AF37;
            font-weight: bold;
            color: #D4AF37;
        }
        
        QProgressBar {
            border: 2px solid #D4AF37;
            border-radius: 5px;
            text-align: center;
            color: #1E3A5F;
            font-weight: bold;
        }
        
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                stop:0 #D4AF37, stop:1 #FFD700);
            border-radius: 3px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
        }
        
        QCheckBox::indicator:unchecked {
            background-color: #142A4F;
            border: 2px solid #2C5AA0;
            border-radius: 3px;
        }
        
        QCheckBox::indicator:checked {
            background-color: #D4AF37;
            border: 2px solid #D4AF37;
            border-radius: 3px;
        }
        
        QSplitter::handle {
            background-color: #D4AF37;
        }
        
        QSplitter::handle:horizontal {
            width: 3px;
        }
        
        QSplitter::handle:vertical {
            height: 3px;
        }
        
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #1E3A5F, stop:1 #2C5AA0);
        }
    )");
    
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
