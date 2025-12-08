#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QCheckBox>
#include <QProgressBar>
#include "updatedownloader.h"
#include "updateinstaller.h"

/**
 * UpdateDialog - Displays information about available updates and handles installation
 */
class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(const QString& currentVersion,
                          const QString& newVersion,
                          const QString& downloadUrl,
                          const QString& releaseNotes,
                          QWidget *parent = nullptr);
    ~UpdateDialog();

    bool skipThisVersion() const { return m_skipVersion; }

private slots:
    void onDownloadClicked();
    void onSkipVersionChanged(Qt::CheckState state);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString& filePath);
    void onDownloadFailed(const QString& error);
    void onExtractionProgress(int percent);
    void onExtractionFinished();
    void onExtractionFailed(const QString& error);

private:
    void setupUI(const QString& currentVersion,
                 const QString& newVersion,
                 const QString& releaseNotes);
    void startDownloadAndInstall();
    void showDownloadProgress();
    void showExtractionProgress();

    QString m_downloadUrl;
    bool m_skipVersion;
    bool m_isInstalling;
    
    QLabel* m_titleLabel;
    QLabel* m_versionLabel;
    QLabel* m_statusLabel;
    QTextBrowser* m_releaseNotesBrowser;
    QCheckBox* m_skipVersionCheckBox;
    QProgressBar* m_progressBar;
    QPushButton* m_downloadButton;
    QPushButton* m_laterButton;
    
    UpdateDownloader* m_downloader;
    UpdateInstaller* m_installer;
    QString m_downloadedFilePath;
};

#endif // UPDATEDIALOG_H

