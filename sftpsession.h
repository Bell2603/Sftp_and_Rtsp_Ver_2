#ifndef SFTPSESSION_H
#define SFTPSESSION_H

#include <QObject>
#include <QProcess>

// Model: bọc QProcess chạy sftp qua sshpass, và parse output thô (text)
// thành dữ liệu có cấu trúc (tên file + có phải thư mục hay không).
// Đây chính là phần logic của onReadyRead() cũ trong MainWindow - việc parse
// dữ liệu là việc của Model, không phải của View.
class SftpSession : public QObject
{
    Q_OBJECT
public:
    explicit SftpSession(QObject *parent = nullptr);

    bool isRunning() const;
    void connectToServer(const QString &host, const QString &port,
                          const QString &user, const QString &password);
    void disconnectFromServer();
    void sendCommand(const QString &cmd);

signals:
    void entryParsed(const QString &name, bool isDir);
    void startFailed(const QString &reason);
    void started();
    void fetchingStarted(); // sftp báo "Fetching" -> dùng để biết lúc nào file tải xong

private slots:
    void onReadyRead();

private:
    QProcess *m_process;
};

#endif // SFTPSESSION_H
