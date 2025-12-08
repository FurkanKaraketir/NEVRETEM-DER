#include "updatedownloader.h"
#include <QNetworkRequest>
#include <QDir>
#include <QFileInfo>

UpdateDownloader::UpdateDownloader(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_outputFile(nullptr)
{
}

UpdateDownloader::~UpdateDownloader()
{
    cancelDownload();
}

void UpdateDownloader::startDownload(const QString& url, const QString& destinationPath)
{
    m_destinationPath = destinationPath;

    // Ensure directory exists
    QFileInfo fileInfo(destinationPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Create output file
    m_outputFile = new QFile(destinationPath, this);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        emit downloadFailed(QString("Cannot create file: %1").arg(destinationPath));
        delete m_outputFile;
        m_outputFile = nullptr;
        return;
    }

    // Start download
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "NEVRETEM-DER-MBS-Updater");

    m_reply = m_networkManager->get(request);
    
    connect(m_reply, &QNetworkReply::downloadProgress,
            this, &UpdateDownloader::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished,
            this, &UpdateDownloader::onDownloadFinished);
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &UpdateDownloader::onDownloadError);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_outputFile) {
            m_outputFile->write(m_reply->readAll());
        }
    });
}

void UpdateDownloader::cancelDownload()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (m_outputFile) {
        m_outputFile->close();
        m_outputFile->remove();
        delete m_outputFile;
        m_outputFile = nullptr;
    }
}

void UpdateDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateDownloader::onDownloadFinished()
{
    if (!m_reply || !m_outputFile) {
        return;
    }

    // Write any remaining data
    m_outputFile->write(m_reply->readAll());
    m_outputFile->close();

    if (m_reply->error() != QNetworkReply::NoError) {
        m_outputFile->remove();
        emit downloadFailed(m_reply->errorString());
    } else {
        emit downloadFinished(m_destinationPath);
    }

    m_reply->deleteLater();
    m_reply = nullptr;
    delete m_outputFile;
    m_outputFile = nullptr;
}

void UpdateDownloader::onDownloadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    
    if (m_reply) {
        emit downloadFailed(m_reply->errorString());
    }
}

