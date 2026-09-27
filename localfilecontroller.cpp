#include "localfilecontroller.h"
#include "appconfig.h"

#include <QFile>

LocalFileController::LocalFileController(QObject *parent)
    : QObject(parent)
{
}

bool LocalFileController::renameFile(const QString &oldName, const QString &newName)
{
    const auto &config = AppConfig::instance();
    QString oldPath = config.localPath() + "/" + oldName;
    QString newPath = config.localPath() + "/" + newName + ".mp4";
    return QFile::rename(oldPath, newPath);
}

bool LocalFileController::deleteFile(const QString &fileName)
{
    const auto &config = AppConfig::instance();
    QString filePath = config.localPath() + "/" + fileName;
    return QFile::remove(filePath);
}
