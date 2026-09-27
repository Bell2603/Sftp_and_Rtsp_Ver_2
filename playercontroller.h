#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QUrl>
#include <QTimer>

// Controller: quản lý QMediaPlayer + QVideoWidget.
// MainWindow không còn tự giữ biến IS_Pause, mDuration nữa - toàn bộ
// trạng thái play/pause/duration nằm ở đây, MainWindow chỉ lắng nghe signal.
class PlayerController : public QObject
{
    Q_OBJECT
public:
    explicit PlayerController(QVideoWidget *videoWidget, QObject *parent = nullptr);

    // Phát file local (kiểm tra file tồn tại trước khi phát)
    void play(const QString &localPath);

    // Phát trực tiếp URL (dùng cho RTSP stream, không kiểm tra QFile::exists).
    // Bật chế độ theo dõi stream: tự kết nối lại khi stream kết thúc/lỗi/đứng hình,
    // có giới hạn số lần thử (xem kMaxStreamRetries).
    void playUrl(const QUrl &url);

    void togglePlayPause();

    // Tua tới vị trí (giây). Bỏ qua nếu media không hỗ trợ seek hoặc đang phát live stream.
    void seekToSec(qint64 sec);
    bool canSeek() const;

    bool isPaused() const { return m_isPaused; }
    qint64 totalDurationSec() const { return m_durationSec; }

signals:
    void error(const QString &message);
    void durationChangedSec(qint64 durationSec);
    void positionChangedSec(qint64 positionSec);
    void playStateChanged(bool isPaused);
    void seekableChanged(bool seekable);
    void streamStatus(const QString &message); // trạng thái kết nối lại RTSP (hiện lên status bar)

private slots:
    void handleDurationChanged(qint64 duration);
    void handlePositionChanged(qint64 position);

    // Theo dõi RTSP
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handlePlayerError(QMediaPlayer::Error error, const QString &errorString);
    void handleStreamWatchdogTimeout();
    void attemptStreamReconnect();

private:
    void startPlayback();

    // RTSP
    void loadStreamSource();
    void startStreamPlayback();
    void handleStreamProgress();
    void scheduleStreamReconnect(const QString &reason);
    void stopStreamMonitoring();

    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    bool m_isPaused = true;
    qint64 m_durationSec = 0;

    // ----- Trạng thái RTSP -----
    // Chỉ có 1 QMediaPlayer + 2 QTimer được tạo 1 lần trong constructor,
    // retry chỉ nạp lại source cho player hiện có -> không tạo player/connection trùng.
    QUrl m_streamUrl;
    bool m_isLiveStream = false;     // đang ở chế độ phát RTSP
    bool m_streamConfirmed = false;  // lần kết nối hiện tại đã nhận được dữ liệu chưa
    bool m_streamNeedsReload = false;// stream bị ngắt/huỷ retry trong lúc user pause
    bool m_reloadingStream = false;  // đang tự stop/clear source -> bỏ qua signal phát sinh
    int m_streamRetryCount = 0;      // số lần thử liên tiếp thất bại
    qint64 m_lastStreamPosMs = 0;    // vị trí lần ghi nhận trước, để phát hiện stream đứng hình
    QTimer *m_streamRetryTimer;      // delay trước mỗi lần thử lại (single-shot)
    QTimer *m_streamWatchdog;        // quá thời gian không có dữ liệu -> coi như mất kết nối

    static constexpr int kMaxStreamRetries = 5;
    static constexpr int kRetryBaseDelayMs = 2000;   // 2s, 4s, 6s, 8s, 10s
    static constexpr int kRetryMaxDelayMs = 10000;
    static constexpr int kStreamNoDataTimeoutMs = 15000;
    static constexpr qint64 kStreamPosProgressMs = 500;
};

#endif // PLAYERCONTROLLER_H
