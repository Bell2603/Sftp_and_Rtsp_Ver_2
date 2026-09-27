#ifndef LOCALFILECONTROLLER_H
#define LOCALFILECONTROLLER_H

#include <QObject>
#include <QString>

// Controller nhỏ: xử lý rename/delete file trên máy local.
// Tách khỏi MainWindow để View không tự ý đụng vào QFile.
class LocalFileController : public QObject
{
    Q_OBJECT
public:
    explicit LocalFileController(QObject *parent = nullptr);

    bool renameFile(const QString &oldName, const QString &newName);
    bool deleteFile(const QString &fileName);
};

#endif // LOCALFILECONTROLLER_H
