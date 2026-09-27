#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

QString Dialog::NewName_Test()
{
    return ui->lineEdit_Name->text().trimmed();
}

void Dialog::on_pushButton_OK_clicked()
{
    accept();
}

void Dialog::on_pushButton_Cancel_clicked()
{
    reject();
}
