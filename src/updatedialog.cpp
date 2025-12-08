#include "updatedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QStyle>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCoreApplication>

UpdateDialog::UpdateDialog(const QString& currentVersion,
                           const QString& newVersion,
                           const QString& downloadUrl,
                           const QString& releaseNotes,
                           QWidget *parent)
    : QDialog(parent)
    , m_downloadUrl(downloadUrl)
    , m_skipVersion(false)
    , m_isInstalling(false)
    , m_downloader(new UpdateDownloader(this))
    , m_installer(new UpdateInstaller(this))
{
    setWindowTitle("Güncelleme Mevcut");
    setMinimumSize(500, 450);
    
    setupUI(currentVersion, newVersion, releaseNotes);
    
    // Connect downloader signals
    connect(m_downloader, &UpdateDownloader::downloadProgress,
            this, &UpdateDialog::onDownloadProgress);
    connect(m_downloader, &UpdateDownloader::downloadFinished,
            this, &UpdateDialog::onDownloadFinished);
    connect(m_downloader, &UpdateDownloader::downloadFailed,
            this, &UpdateDialog::onDownloadFailed);
    
    // Connect installer signals
    connect(m_installer, &UpdateInstaller::extractionProgress,
            this, &UpdateDialog::onExtractionProgress);
    connect(m_installer, &UpdateInstaller::extractionFinished,
            this, &UpdateDialog::onExtractionFinished);
    connect(m_installer, &UpdateInstaller::extractionFailed,
            this, &UpdateDialog::onExtractionFailed);
}

UpdateDialog::~UpdateDialog()
{
}

void UpdateDialog::setupUI(const QString& currentVersion,
                            const QString& newVersion,
                            const QString& releaseNotes)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title with icon
    QHBoxLayout* titleLayout = new QHBoxLayout();
    
    QLabel* iconLabel = new QLabel(this);
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
    iconLabel->setPixmap(icon.pixmap(48, 48));
    titleLayout->addWidget(iconLabel);
    
    m_titleLabel = new QLabel("<h2>Yeni sürüm mevcut!</h2>", this);
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    
    mainLayout->addLayout(titleLayout);

    // Version information
    m_versionLabel = new QLabel(this);
    m_versionLabel->setText(
        QString("<p><b>Mevcut sürüm:</b> %1<br>"
                "<b>Yeni sürüm:</b> <span style='color: #4CAF50;'>%2</span></p>")
            .arg(currentVersion, newVersion)
    );
    m_versionLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(m_versionLabel);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Release notes
    QLabel* notesLabel = new QLabel("<b>Yenilikler:</b>", this);
    mainLayout->addWidget(notesLabel);

    m_releaseNotesBrowser = new QTextBrowser(this);
    m_releaseNotesBrowser->setOpenExternalLinks(true);
    
    // Convert markdown-style release notes to simple HTML
    QString htmlNotes = releaseNotes;
    htmlNotes.replace("\n", "<br>");
    htmlNotes.replace("## ", "<h3>");
    
    m_releaseNotesBrowser->setHtml(htmlNotes);
    m_releaseNotesBrowser->setMaximumHeight(150);
    mainLayout->addWidget(m_releaseNotesBrowser);

    // Skip version checkbox
    m_skipVersionCheckBox = new QCheckBox("Bu sürümü atla", this);
    connect(m_skipVersionCheckBox, &QCheckBox::checkStateChanged,
            this, &UpdateDialog::onSkipVersionChanged);
    mainLayout->addWidget(m_skipVersionCheckBox);

    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_laterButton = new QPushButton("Daha Sonra", this);
    connect(m_laterButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_laterButton);

    m_downloadButton = new QPushButton("Şimdi Yükle ve Kur", this);
    m_downloadButton->setDefault(true);
    m_downloadButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    padding: 8px 20px;"
        "    border: none;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
    );
    connect(m_downloadButton, &QPushButton::clicked,
            this, &UpdateDialog::onDownloadClicked);
    buttonLayout->addWidget(m_downloadButton);

    mainLayout->addLayout(buttonLayout);
}

void UpdateDialog::onDownloadClicked()
{
    if (m_isInstalling) {
        return; // Already installing
    }

    m_isInstalling = true;
    startDownloadAndInstall();
}

void UpdateDialog::startDownloadAndInstall()
{
    // Disable buttons
    m_downloadButton->setEnabled(false);
    m_laterButton->setEnabled(false);
    m_skipVersionCheckBox->setEnabled(false);

    showDownloadProgress();

    // Download to temp directory
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadedFilePath = QDir(tempDir).filePath("nevretem_update.zip");

    m_downloader->startDownload(m_downloadUrl, m_downloadedFilePath);
}

void UpdateDialog::showDownloadProgress()
{
    m_statusLabel->setText("Güncelleme indiriliyor...");
    m_statusLabel->setVisible(true);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
}

void UpdateDialog::showExtractionProgress()
{
    m_statusLabel->setText("Güncelleme çıkartılıyor...");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
}

void UpdateDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressBar->setValue(percent);
        
        double mbReceived = bytesReceived / (1024.0 * 1024.0);
        double mbTotal = bytesTotal / (1024.0 * 1024.0);
        m_statusLabel->setText(QString("İndiriliyor: %1 MB / %2 MB")
            .arg(mbReceived, 0, 'f', 1)
            .arg(mbTotal, 0, 'f', 1));
    }
}

void UpdateDialog::onDownloadFinished(const QString& filePath)
{
    m_statusLabel->setText("İndirme tamamlandı! Çıkartılıyor...");
    showExtractionProgress();

    // Extract to temp directory
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString extractPath = QDir(tempDir).filePath("nevretem_update_extracted");

    m_installer->extractUpdate(filePath, extractPath);
}

void UpdateDialog::onDownloadFailed(const QString& error)
{
    m_isInstalling = false;
    m_downloadButton->setEnabled(true);
    m_laterButton->setEnabled(true);
    m_skipVersionCheckBox->setEnabled(true);
    m_progressBar->setVisible(false);
    
    QMessageBox::critical(this, "İndirme Hatası",
        QString("Güncelleme indirilemedi:\n\n%1").arg(error));
}

void UpdateDialog::onExtractionProgress(int percent)
{
    m_progressBar->setValue(percent);
}

void UpdateDialog::onExtractionFinished()
{
    m_statusLabel->setText("Güncelleme kuruluyor...");

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString extractPath = QDir(tempDir).filePath("nevretem_update_extracted");
    QString currentAppPath = QCoreApplication::applicationDirPath();
    QString executableName = "StudentManager.exe";

    QMessageBox::information(this, "Güncelleme Hazır",
        "Güncelleme yüklenecek ve uygulama yeniden başlatılacak.\n\n"
        "Devam etmek için Tamam'a basın.");

    // Install and restart
    m_installer->installUpdate(extractPath, currentAppPath, executableName);
    
    // The app will close automatically after launching the updater
}

void UpdateDialog::onExtractionFailed(const QString& error)
{
    m_isInstalling = false;
    m_downloadButton->setEnabled(true);
    m_laterButton->setEnabled(true);
    m_skipVersionCheckBox->setEnabled(true);
    m_progressBar->setVisible(false);
    
    QMessageBox::critical(this, "Çıkartma Hatası",
        QString("Güncelleme çıkartılamadı:\n\n%1").arg(error));
}

void UpdateDialog::onSkipVersionChanged(Qt::CheckState state)
{
    m_skipVersion = (state == Qt::Checked);
}

