#include "registration_widget.h"
#include "ui_registration_widget.h"

RegistrationWidget::RegistrationWidget(QWidget *parent) : QWidget(parent), ui(new Ui::RegistrationWidget) {
    ui->setupUi(this);
    this->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    account_info = new QSettings("TestRegistration.cfg", QSettings::IniFormat);
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
}

RegistrationWidget::~RegistrationWidget() {
    delete account_info;
    delete ui;
}

bool RegistrationWidget::register_user() {
    if((ui->lineEdit_FName->text().length() != 0) && (ui->lineEdit_LName->text().length() != 0) && (ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        QStringList lst = account_info->allKeys();
        for(int i = 0; i < lst.size(); i++) {
            QRegExp tagExp("_");
            QStringList log_pwd = lst.at(i).split(tagExp);
            if((log_pwd.at(0).compare(ui->lineEdit_login->text(), Qt::CaseSensitive)) == 0) {
                QMessageBox::warning(this, tr("Error"), tr("The same login does already exist"));
                return false;
            }
        }
        account_info->setValue(ui->lineEdit_login->text() + "_" + ui->lineEdit_password->text(), ui->lineEdit_FName->text() + " " + ui->lineEdit_LName->text());
        account_info->sync();
        return true;
    } else {
         QMessageBox::warning(this, tr("Error"), tr("Enter login, password, first and last names"));
         return false;
    }
}

bool RegistrationWidget::login() {
    if((ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        if(account_info->contains(ui->lineEdit_login->text() + "_" + ui->lineEdit_password->text())) {
            QString data = account_info->value(ui->lineEdit_login->text() + "_" + ui->lineEdit_password->text()).toString();
            QRegExp tagExp(" ");
            QStringList lst = data.split(tagExp);
            user_fname = lst.at(0);
            user_lname = lst.at(1);
            ui->lineEdit_login->setText("");
            ui->lineEdit_password->setText("");
            ui->lineEdit_FName->setText("");
            ui->lineEdit_LName->setText("");
            return true;
        } else {
            QMessageBox::warning(this, tr("Error"), tr("You enter wrong login or password"));
            return false;
        }
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Enter login and password"));
        return false;
    }
}

QString RegistrationWidget::get_user_fname() {
    return user_fname;
}

QString RegistrationWidget::get_user_lname() {
    return user_lname;
}
