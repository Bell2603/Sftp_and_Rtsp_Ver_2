#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QFileSystemModel>

#include "dialog.h"
#include "sftpcontroller.h"
#include "playercontroller.h"
#include "localfilecontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// View: chỉ setup UI, forward thao tác người dùng sang Controller,
// và cập nhật UI khi Controller phát signal. Không còn QProcess,
// không còn logic parse dữ liệu, không còn quyết định nghiệp vụ nào ở đây.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Buttons - btnConnect nối thủ công (xem constructor), các on_..._clicked
    // còn lại được Qt tự auto-connect nhờ đúng tên theo Designer.
    void onBtnConnectClicked();
    void on_btnPlayStream_clicked();
    void on_pushButton_Play_Pause_clicked();
    void on_horizontalSlider_Duration_sliderMoved(int position);
    void on_horizontalSlider_Duration_sliderReleased();
    void on_horizontalSlider_Duration_actionTriggered(int action);
    void on_pushButton_Seek_Forward_clicked();
    void on_pushButton_Seek_Backward_clicked();

    // Tree view - remote
    void on_treeView_doubleClicked(const QModelIndex &index);
    void on_treeView_customContextMenuRequested(const QPoint &pos);

    // Tree view - local
    void on_treeView_Local_doubleClicked(const QModelIndex &index);
    void on_treeView_Local_clicked(const QModelIndex &index);
    void on_treeView_Local_customContextMenuRequested(const QPoint &pos);

    // Nhận signal từ SftpController
    void handleConnectionStateChanged(bool connected);
    void handleConnectionFailed(const QString &reason);
    void handleVideoReadyToPlay(const QString &localPath);

    // Nhận signal từ PlayerController
    void handlePlayerError(const QString &message);
    void handleDurationChangedSec(qint64 durationSec);
    void handlePositionChangedSec(qint64 positionSec);
    void handlePlayStateChanged(bool isPaused);
    void handleSeekableChanged(bool seekable);
    void handleStreamStatus(const QString &message);

private:
    void updateTimeLabels(qint64 currentSec);
    void promptRenameRemote(const QString &oldName);
    void promptRenameLocal(const QString &oldName);
    void playNeighbourInLocalTree(int rowOffset);

    Ui::MainWindow *ui;

    SftpController *m_sftpController;
    PlayerController *m_playerController;
    LocalFileController *m_localFileController;

    QFileSystemModel *m_localModel;

    qint64 m_totalDurationSec = 0;
};
#endif // MAINWINDOW_H
