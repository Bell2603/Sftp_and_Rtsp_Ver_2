#include "playercontroller.h"

#include <QFile>
#include <QVideoSink>
#include <QVideoFrame>

PlayerController::PlayerController(QVideoWidget *videoWidget, QObject *parent)
    : QObject(parent)
    , m_videoWidget(videoWidget)
{
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerController::handleDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::handlePositionChanged);

    // Seek: bao cho View biet media hien tai co tua duoc khong
    connect(m_player, &QMediaPlayer::seekableChanged, this, [this](bool) {
        emit seekableChanged(canSeek());
    });

    // ----- Theo doi RTSP -----
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &PlayerController::handleMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &PlayerController::handlePlayerError);

    // Moi frame video nhan duoc = stream con song (khong phu thuoc backend co bao
    // BufferedMedia hay khong)
    connect(m_videoWidget->videoSink(), &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
        if (frame.isValid()) {
            handleStreamProgress();
        }
    });

    m_streamRetryTimer = new QTimer(this);
    m_streamRetryTimer->setSingleShot(true);
    connect(m_streamRetryTimer, &QTimer::timeout, this, &PlayerController::attemptStreamReconnect);

    m_streamWatchdog = new QTimer(this);
    m_streamWatchdog->setSingleShot(true);
    m_streamWatchdog->setInterval(kStreamNoDataTimeoutMs);
    connect(m_streamWatchdog, &QTimer::timeout, this, &PlayerController::handleStreamWatchdogTimeout);
}

void PlayerController::play(const QString &localPath)
{
    if (!QFile::exists(localPath)) {
        emit error("File not exist " + localPath);
        return;
    }

    // Chuyen sang file local -> tat che do theo doi/retry RTSP
    stopStreamMonitoring();

    if (!m_player->isAvailable()) {
        emit error("Video can't play");
    }

    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(localPath));
    startPlayback();
}

void PlayerController::playUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        emit error("RTSP link is empty or invalid. Check [Stream] rtsp=... in config.ini");
        return;
    }

    // Bat dau 1 phien stream moi: huy moi retry cu, reset bo dem
    stopStreamMonitoring();
    m_streamUrl = url;
    m_isLiveStream = true;
    m_isPaused = false; // user vua bam Play Stream -> loi ket noi phai duoc retry, khong coi la "dang pause"
    emit seekableChanged(false); // khong tua live stream

    // (Da bo kiem tra m_player->error() ngay sau setSource: loi ket noi gio duoc
    // xu ly boi errorOccurred -> retry, va chi bao popup 1 lan khi dung han.)
    startStreamPlayback();
}

void PlayerController::startStreamPlayback()
{
    loadStreamSource();

    // Mot so backend (vd FFmpeg tren Qt 6.4) mo ket noi DONG BO ngay trong setSource()
    // va phat errorOccurred ngay tai do -> retry da duoc len lich, hoac da dung han.
    // Khi do KHONG goi play() tren source loi.
    if (!m_isLiveStream) {
        return; // da dung han, playStateChanged(paused=true) da duoc phat
    }
    if (m_streamRetryTimer->isActive()) {
        emit playStateChanged(m_isPaused); // dang cho retry
        return;
    }
    startPlayback();
}

void PlayerController::startPlayback()
{
    m_videoWidget->show();
    m_player->play();

    // reset lai trang thai cua nut Play_Pause
    m_isPaused = false;
    emit playStateChanged(m_isPaused);
}

void PlayerController::togglePlayPause()
{
    if (m_isPaused) {
        m_isPaused = false;

        if (m_isLiveStream && m_streamNeedsReload) {
            // Stream bi ngat (hoac retry bi huy) trong luc pause -> ket noi lai tu dau
            m_streamRetryCount = 0;
            startStreamPlayback(); // tu phat playStateChanged
            return;
        }
        if (m_isLiveStream) {
            m_streamWatchdog->start();
        }

        m_player->play();
    } else {
        m_isPaused = true;
        m_player->pause();

        // User chu dong pause -> khong tu ket noi lai, khong dem thoi gian mat du lieu
        if (m_isLiveStream) {
            if (m_streamRetryTimer->isActive()) {
                m_streamRetryTimer->stop();
                m_streamNeedsReload = true;
            }
            m_streamWatchdog->stop();
        }
    }
    emit playStateChanged(m_isPaused);
}

// ---------------- Seek ----------------

bool PlayerController::canSeek() const
{
    return !m_isLiveStream && m_player->isSeekable();
}

void PlayerController::seekToSec(qint64 sec)
{
    if (!canSeek()) {
        return;
    }
    m_player->setPosition(sec * 1000); // QMediaPlayer dung don vi ms
}

void PlayerController::handleDurationChanged(qint64 duration)
{
    m_durationSec = duration / 1000; // Doi sang don vi giay
    emit durationChangedSec(m_durationSec);
}

void PlayerController::handlePositionChanged(qint64 position)
{
    emit positionChangedSec(position / 1000);

    // Mot so backend (vd FFmpeg Qt 6.4) van phat positionChanged dinh ky va nhich
    // vai ms moi giay ke ca khi stream dung hinh -> chi tinh la "con du lieu"
    // khi vi tri thay doi dang ke (>= 500 ms) so voi lan ghi nhan truoc.
    if (qAbs(position - m_lastStreamPosMs) >= kStreamPosProgressMs) {
        m_lastStreamPosMs = position;
        handleStreamProgress();
    }
}

// ---------------- RTSP: theo doi & ket noi lai ----------------

void PlayerController::loadStreamSource()
{
    // stop() va setSource(QUrl()) co the phat signal dong bo (NoMedia, LoadedMedia,
    // position 0...) -> danh dau de cac handler bo qua.
    m_reloadingStream = true;
    m_player->stop();
    // Bat buoc clear truoc: Qt 6 bo qua setSource() neu URL trung voi source hien tai,
    // khi do player se khong ket noi lai RTSP.
    m_player->setSource(QUrl());
    m_reloadingStream = false;

    m_streamConfirmed = false;
    m_streamNeedsReload = false;
    m_lastStreamPosMs = 0;
    m_player->setSource(m_streamUrl);

    // Gioi han thoi gian cho lan ket noi nay (ket noi treo / khong co du lieu)
    m_streamWatchdog->start();
}

void PlayerController::handleStreamProgress()
{
    if (!m_isLiveStream || m_isPaused || m_reloadingStream) {
        return;
    }
    if (m_streamRetryTimer->isActive()) {
        return; // dang cho retry, bo qua frame/position con sot lai
    }

    m_streamWatchdog->start(); // van nhan duoc du lieu -> gia han watchdog

    if (!m_streamConfirmed) {
        // Lan ket noi nay da nhan duoc du lieu -> nguon RTSP con song, reset bo dem
        m_streamConfirmed = true;
        if (m_streamRetryCount > 0) {
            emit streamStatus("RTSP: reconnected");
        }
        m_streamRetryCount = 0;
    }
}

void PlayerController::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (!m_isLiveStream || m_reloadingStream) {
        return;
    }

    if (status == QMediaPlayer::EndOfMedia) {
        scheduleStreamReconnect("stream ended");
    } else if (status == QMediaPlayer::InvalidMedia) {
        scheduleStreamReconnect("stream unavailable");
    }
    // NoMedia / Loading / Loaded / Stalled / Buffering / Buffered: khong xu ly,
    // watchdog + frame/position lo phan phat hien "con song" hay "treo".
}

void PlayerController::handlePlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    if (error == QMediaPlayer::NoError || !m_isLiveStream || m_reloadingStream) {
        return;
    }
    scheduleStreamReconnect(errorString.isEmpty() ? QString("player error") : errorString);
}

void PlayerController::handleStreamWatchdogTimeout()
{
    scheduleStreamReconnect(m_streamConfirmed ? QString("no data received")
                                              : QString("connection timed out"));
}

void PlayerController::scheduleStreamReconnect(const QString &reason)
{
    if (!m_isLiveStream) {
        return;
    }
    if (m_isPaused) {
        // User dang pause: khong tu phat lai, doi user bam Play
        m_streamNeedsReload = true;
        return;
    }
    if (m_streamRetryTimer->isActive()) {
        // EndOfMedia + errorOccurred co the den cung luc cho 1 lan loi:
        // chi len lich 1 lan retry duy nhat
        return;
    }

    m_streamWatchdog->stop();

    if (m_streamRetryCount >= kMaxStreamRetries) {
        // Nguon RTSP thuc su khong con kha dung -> dung han, bao loi 1 lan
        const QString msg = QString("RTSP stream unavailable (%1). Stopped after %2 retries.")
                                .arg(reason)
                                .arg(kMaxStreamRetries);
        stopStreamMonitoring();
        m_player->stop();
        m_isPaused = true;
        emit playStateChanged(m_isPaused);
        emit streamStatus(msg);
        emit error(msg);
        return;
    }

    ++m_streamRetryCount;
    const int delayMs = qMin(kRetryBaseDelayMs * m_streamRetryCount, kRetryMaxDelayMs);
    emit streamStatus(QString("RTSP: %1 - retry %2/%3 in %4 s...")
                          .arg(reason)
                          .arg(m_streamRetryCount)
                          .arg(kMaxStreamRetries)
                          .arg(delayMs / 1000));
    m_streamRetryTimer->start(delayMs);
}

void PlayerController::attemptStreamReconnect()
{
    if (!m_isLiveStream || m_isPaused) {
        return;
    }
    emit streamStatus(QString("RTSP: reconnecting (%1/%2)...")
                          .arg(m_streamRetryCount)
                          .arg(kMaxStreamRetries));
    startStreamPlayback();
}

void PlayerController::stopStreamMonitoring()
{
    m_isLiveStream = false;
    m_streamRetryTimer->stop();
    m_streamWatchdog->stop();
    m_streamRetryCount = 0;
    m_streamConfirmed = false;
    m_streamNeedsReload = false;
}
