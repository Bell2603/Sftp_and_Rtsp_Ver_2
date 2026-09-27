#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

// Model: đọc config.ini MỘT LẦN DUY NHẤT lúc khởi động app.
// Trước đây QSettings + đường dẫn config bị hard-code lặp lại ở 3 nơi
// khác nhau trong MainWindow (connectSftp(), constructor, on_btnPlayStream_clicked()),
// với on_btnPlayStream_clicked() còn dùng một đường dẫn KHÁC hẳn ("/home/hung/...")
// nên khoá "Stream/rtsp" không đọc từ cùng 1 file config với phần còn lại.
// AppConfig gom việc này về một chỗ duy nhất.
class AppConfig
{
public:
    static AppConfig &instance();

    // Gọi 1 lần trong main.cpp hoặc constructor của MainWindow
    void load(const QString &configPath);

    QString host() const { return m_host; }
    QString port() const { return m_port; }
    QString user() const { return m_user; }
    QString password() const { return m_password; }
    QString keyPath() const { return m_keyPath; }
    QString remoteDefaultPath() const { return m_remoteDefaultPath; }
    QString localPath() const { return m_localPath; }
    QString tmpDir() const { return m_tmpDir; }
    QString rtspLink() const { return m_rtspLink; }

    bool isLoaded() const { return m_loaded; }

private:
    AppConfig() = default;

    QString m_host;
    QString m_port;
    QString m_user;
    QString m_password;
    QString m_keyPath;
    QString m_remoteDefaultPath;
    QString m_localPath;
    QString m_tmpDir;
    QString m_rtspLink;
    bool m_loaded = false;
};

#endif // APPCONFIG_H
