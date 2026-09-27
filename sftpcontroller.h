#ifndef SFTPCONTROLLER_H
#define SFTPCONTROLLER_H

#include <QObject>

#include "sftpsession.h"
#include "sftpfilemodel.h"

// Controller: nơi DUY NHẤT "biết" cả SftpSession/SftpFileModel (Model)
// lẫn phát tín hiệu cho MainWindow (View) cập nhật giao diện.
// Chứa toàn bộ business logic đã nằm rải rác trong MainWindow trước đây:
// - quyết định double-click vào file thì play luôn hay phải tải về trước
//   (logic cũ trong on_treeView_doubleClicked)
// - rename / download / refresh
class SftpController : public QObject
{
    Q_OBJECT
public:
    explicit SftpController(QObject *parent = nullptr);

    QAbstractItemModel *fileModel() const { return m_fileModel->proxyModel(); }
    bool isConnected() const { return m_session->isRunning(); }

    void toggleConnection();
    void refresh();

    // name: tên file được double-click trong treeView remote.
    // Nếu đã có ở local hoặc thư mục tạm -> videoReadyToPlay() phát ngay.
    // Nếu chưa có -> gửi lệnh "get" rồi chờ fetchingStarted() để phát videoReadyToPlay().
    void openEntry(const QString &name);

    void downloadToLocal(const QString &fileName);
    void renameRemote(const QString &oldName, const QString &newName);

signals:
    void connectionStateChanged(bool connected);
    void connectionFailed(const QString &reason);
    void videoReadyToPlay(const QString &localPath);

private slots:
    void handleEntryParsed(const QString &name, bool isDir);
    void handleFetchingStarted();

private:
    SftpSession *m_session;
    SftpFileModel *m_fileModel;
    QString m_pendingVideoPath; // đường dẫn video đang chờ tải xong để play
};

#endif // SFTPCONTROLLER_H
