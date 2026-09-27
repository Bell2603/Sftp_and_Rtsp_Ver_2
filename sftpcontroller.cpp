#include "sftpcontroller.h"
#include "appconfig.h"

#include <QFile>
#include <QTimer>

SftpController::SftpController(QObject *parent)
    : QObject(parent)
{
    m_session = new SftpSession(this);
    m_fileModel = new SftpFileModel(this);

    connect(m_session, &SftpSession::entryParsed, this, &SftpController::handleEntryParsed);
    connect(m_session, &SftpSession::fetchingStarted, this, &SftpController::handleFetchingStarted);
    connect(m_session, &SftpSession::startFailed, this, &SftpController::connectionFailed);
}

void SftpController::toggleConnection()
{
    const auto &config = AppConfig::instance();

    if (m_session->isRunning()) {
        // Neu co Process chay truoc, hoac khi ban disconnect
        m_session->disconnectFromServer();
        m_fileModel->clear();
        emit connectionStateChanged(false);
        return;
    }

    m_session->connectToServer(config.host(), config.port(), config.user(), config.password());

    if (m_session->isRunning()) {
        emit connectionStateChanged(true);
        m_session->sendCommand("cd " + config.remoteDefaultPath());
        m_session->sendCommand("ls -l");
    }
}

void SftpController::refresh()
{
    const auto &config = AppConfig::instance();
    m_session->sendCommand(QString("cd \"%1\"").arg(config.remoteDefaultPath()));

    QTimer::singleShot(300, this, [this]() {
        m_fileModel->clear();
        m_session->sendCommand("ls -l");
    });
}

void SftpController::openEntry(const QString &name)
{
    const auto &config = AppConfig::instance();
    QString localPath = config.localPath() + "/" + name;
    QString tmpPath = config.tmpDir() + "/" + name;

    // Neu file da ton tai phat luon, khong tai lai
    if (QFile::exists(localPath)) {
        emit videoReadyToPlay(localPath);
        return;
    }
    if (QFile::exists(tmpPath)) {
        emit videoReadyToPlay(tmpPath);
        return;
    }

    // Chua co, tai ve thu muc tam roi phat
    m_pendingVideoPath = tmpPath;
    QString remotePath = config.remoteDefaultPath() + "/" + name;
    m_session->sendCommand(QString("get \"%1\" \"%2\"").arg(remotePath, tmpPath));
}

void SftpController::downloadToLocal(const QString &fileName)
{
    const auto &config = AppConfig::instance();
    QString localPath = config.localPath() + "/" + fileName;
    QString remotePath = config.remoteDefaultPath() + fileName;
    m_session->sendCommand(QString("get \"%1\" \"%2\"").arg(remotePath, localPath));
}

void SftpController::renameRemote(const QString &oldName, const QString &newName)
{
    const auto &config = AppConfig::instance();
    QString oldPath = config.remoteDefaultPath() + "/" + oldName;
    QString newPath = config.remoteDefaultPath() + "/" + newName + ".mp4";
    m_session->sendCommand("rename " + oldPath + " " + newPath);
}

void SftpController::handleEntryParsed(const QString &name, bool isDir)
{
    m_fileModel->addEntry(name, isDir);
}

void SftpController::handleFetchingStarted()
{
    // Doi sau khi tai video xong moi phat video
    if (!m_pendingVideoPath.isEmpty()) {
        QString path = m_pendingVideoPath;
        m_pendingVideoPath.clear();
        QTimer::singleShot(800, this, [this, path]() {
            emit videoReadyToPlay(path);
        });
    }
}
