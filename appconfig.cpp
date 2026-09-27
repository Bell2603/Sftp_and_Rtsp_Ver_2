#include "appconfig.h"

#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QFile>

AppConfig &AppConfig::instance()
{
    static AppConfig config;
    return config;
}

void AppConfig::load(const QString &configPath)
{
    QSettings settings(configPath, QSettings::IniFormat);

    m_host = settings.value("Server/host").toString();
    m_port = settings.value("Server/port").toString();
    m_user = settings.value("Server/user").toString();
    m_password = settings.value("Server/password").toString();
    m_keyPath = settings.value("Server/keypath").toString();
    m_remoteDefaultPath = settings.value("Server/remote_defaultPath").toString();

    // Trước đây khoá này được đọc từ MỘT FILE CONFIG KHÁC ("/home/hung/...")
    // trong on_btnPlayStream_clicked(), nay gộp về cùng 1 file config.ini duy nhất.
    // Nhớ thêm mục [Stream] rtsp=... vào config.ini nếu chưa có.
    m_rtspLink = settings.value("Stream/rtsp").toString();

    m_tmpDir = QDir::homePath() + settings.value("Local/tmpDir").toString();
    m_localPath = QDir::homePath() + settings.value("Local/localPath").toString();

    QDir().mkpath(m_tmpDir);

    m_loaded = true;

    qDebug() << "[AppConfig] loaded from:" << configPath;
    qDebug() << "[AppConfig] File exists:" << QFile::exists(configPath);
    qDebug() << "[AppConfig] host:" << m_host << "| port:" << m_port << "| user:" << m_user;
}
