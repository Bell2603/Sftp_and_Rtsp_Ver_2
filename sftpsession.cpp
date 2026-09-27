#include "sftpsession.h"

#include <QRegularExpression>

SftpSession::SftpSession(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SftpSession::onReadyRead);
}

bool SftpSession::isRunning() const
{
    return m_process->state() == QProcess::Running;
}

void SftpSession::connectToServer(const QString &host, const QString &port,
                                   const QString &user, const QString &password)
{
    QStringList args;
    args << "-p" << password              // Truyen mat khau cho sshpass
         << "sftp"                        // sshpass goi chuong trinh sftp
         << "-P" << port                  // Cong ket noi
         << QString("%1@%2").arg(user, host);

    m_process->start("/opt/homebrew/bin/sshpass", args);

    if (!m_process->waitForStarted()) {
        emit startFailed("Not found sshpass");
        return;
    }

    emit started();
}

void SftpSession::disconnectFromServer()
{
    if (isRunning()) {
        m_process->terminate();
    }
}

void SftpSession::sendCommand(const QString &cmd)
{
    if (isRunning()) {
        m_process->write((cmd + "\n").toUtf8());
    }
}

void SftpSession::onReadyRead()
{
    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromUtf8(data);
    QStringList lines = output.split("\n");

    for (const QString &line : lines) {
        QString clean = line.trimmed();
        QString name = clean.split(QRegularExpression("\\s+")).last().trimmed();

        if (clean.isEmpty())               continue;
        if (clean.contains("sftp>"))       continue;
        if (clean.contains("Connected to")) continue;
        if (clean.contains("Fetching"))    continue;
        if (name.isEmpty())                continue;

        bool isDir = clean.startsWith("d");
        emit entryParsed(name, isDir);
    }

    if (output.contains("Fetching")) {
        emit fetchingStarted();
    }
}
