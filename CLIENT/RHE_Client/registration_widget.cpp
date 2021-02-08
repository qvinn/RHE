#include "registration_widget.h"
#include "ui_registration_widget.h"

RegistrationWidget::RegistrationWidget(QWidget *parent, General_Widget *widg, Send_Recieve_Module *snd_rcv_mod) : QWidget(parent), ui(new Ui::RegistrationWidget) {
    ui->setupUi(this);
    this->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    gen_widg = widg;
    snd_rcv_module = snd_rcv_mod;
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
                gen_widg->show_message_box(tr("Error"), tr("The same login does already exist"), 0);
                return false;
            }
        }
        account_info->setValue(ui->lineEdit_login->text() + "_" + ui->lineEdit_password->text(), ui->lineEdit_FName->text() + " " + ui->lineEdit_LName->text());
        account_info->sync();
        return true;
    } else {
        gen_widg->show_message_box(tr("Error"), tr("Enter login, password, first and last names"), 0);
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

            // Иницализируем поключение
            if(!snd_rcv_module->init_connection()) {
                gen_widg->show_message_box(tr("Error"), tr("Can't init connection"), 0);
                return false;
            }
            // Создаем поток для приема входящих пакетов
            std::thread waiting_thread(&Send_Recieve_Module::wait_analize_recv_data, snd_rcv_module);
            waiting_thread.detach();
            // Запросим у сервера ID
            if(snd_rcv_module->get_id_for_client() != CS_OK) {
                gen_widg->show_message_box(tr("Error"), tr("Can't get ID"), 0);
                return false;
            }

            return true;
        } else {
            gen_widg->show_message_box(tr("Error"), tr("You enter wrong login or password"), 0);
            return false;
        }
    } else {
        gen_widg->show_message_box(tr("Error"), tr("Enter login and password"), 0);
        return false;
    }
}

QString RegistrationWidget::get_user_fname() {
    return user_fname;
}

QString RegistrationWidget::get_user_lname() {
    return user_lname;
}

void RegistrationWidget::slot_re_translate() {
    ui->retranslateUi(this);
}
