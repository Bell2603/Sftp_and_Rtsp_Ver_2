#include "sftpfilemodel.h"

#include <QRegularExpression>
#include <QStyle>
#include <QApplication>

SftpFileModel::SftpFileModel(QObject *parent)
    : QObject(parent)
{
    m_sourceModel = new QStandardItemModel(this);
    m_sourceModel->setHorizontalHeaderLabels({"Server"});

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_sourceModel);

    QRegularExpression regExp(".*\\.(mp4|webm|mov)$",
                              QRegularExpression::CaseInsensitiveOption);
    m_proxyModel->setFilterRegularExpression(regExp);
}

void SftpFileModel::addEntry(const QString &name, bool isDir)
{
    QStandardItem *item = new QStandardItem(name);
    // Item remote chi de hien thi, khong cho sua ten truc tiep trong view
    // (doi ten that tren server di qua SftpController::renameRemote()).
    item->setEditable(false);

    if (isDir) {
        item->setIcon(qApp->style()->standardIcon(QStyle::SP_DirIcon));
        item->setData("dir", Qt::UserRole);
    } else {
        item->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
        item->setData("file", Qt::UserRole);
    }

    m_sourceModel->appendRow(item);
}

void SftpFileModel::clear()
{
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());
}

int SftpFileModel::rowCount() const
{
    return m_sourceModel->rowCount();
}
