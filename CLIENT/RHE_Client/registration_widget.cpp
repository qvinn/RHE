#include "registration_widget.h"
#include "ui_registration_widget.h"

RegistrationWidget::RegistrationWidget(QWidget *parent, General_Widget *widg, Data_Transfer_Module *data_transfer_mod, Send_Receive_Module *snd_rcv_mod) : QWidget(parent), ui(new Ui::RegistrationWidget) {
    ui->setupUi(this);
    this->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    gen_widg = widg;
    connect(this, &RegistrationWidget::init_connection_signal, snd_rcv_mod, &Send_Receive_Module::init_connection);
    connect(this, &RegistrationWidget::get_id_for_client_signal, data_transfer_mod, &Data_Transfer_Module::get_id_for_client);
    connect(this, &RegistrationWidget::send_login_register_data_signal, data_transfer_mod, &Data_Transfer_Module::send_file_to_ss_universal);
    connect(snd_rcv_mod, &Send_Receive_Module::link_established_signal, this, &RegistrationWidget::slot_link_established);
    connect(data_transfer_mod, &Data_Transfer_Module::id_received_signal, this, &RegistrationWidget::slot_id_received);
    connect(data_transfer_mod, &Data_Transfer_Module::not_approved_signal, this, &RegistrationWidget::slot_not_approved);
    connect(data_transfer_mod, &Data_Transfer_Module::registered_signal, this, &RegistrationWidget::slot_client_registered);
    connect(data_transfer_mod, &Data_Transfer_Module::logined_signal, this, &RegistrationWidget::slot_client_logined);
    connect(data_transfer_mod, &Data_Transfer_Module::set_user_fname_lname_signal, this, &RegistrationWidget::slot_set_user_fname_lname);
#ifdef __linux__
    ui->label->setFixedHeight(ui->label->height() - 3);
    ui->frame->setContentsMargins(ui->frame->contentsMargins().left(), (ui->frame->contentsMargins().top() + 4), ui->frame->contentsMargins().right(), ui->frame->contentsMargins().bottom());
#endif
}

RegistrationWidget::~RegistrationWidget() {
    delete ui;
}

//-------------------------------------------------------------------------
// REGISTRATION OF USER
//-------------------------------------------------------------------------
bool RegistrationWidget::register_user() {
    if((ui->lineEdit_FName->text().length() != 0) && (ui->lineEdit_LName->text().length() != 0) && (ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        flag = FILE_REGIST;
        return true;
    } else {
        gen_widg->show_message_box(tr("Error"), tr("Enter login, password, first and last names"), 0, this);
        return false;
    }
}

//-------------------------------------------------------------------------
// LOGGING OF USER
//-------------------------------------------------------------------------
void RegistrationWidget::login() {
    if((ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        slot_set_user_fname_lname(ui->lineEdit_FName->text(), ui->lineEdit_LName->text());
        if(user_fname.count() == 0) {
            user_fname.append("-");
        }
        if(user_lname.count() == 0) {
            user_lname.append("-");
        }
        arr.clear();
        arr.append("first_name\t" + user_fname + "\n" + "second_name\t" + user_lname + "\n" + "login\t" + ui->lineEdit_login->text() + "\n" + "password\t" + ui->lineEdit_password->text());
        ui->lineEdit_login->setText("");
        ui->lineEdit_password->setText("");
        ui->lineEdit_FName->setText("");
        ui->lineEdit_LName->setText("");
        if(flag == -1) {
            flag = FILE_LOGIN;
        }
        emit init_connection_signal();      // Иницализируем поключение
    } else {
        emit logined_signal(false);
        gen_widg->show_message_box(tr("Error"), tr("Enter login and password"), 0, this);
    }
}

//-------------------------------------------------------------------------
// CONNECTION WITH SERVER ESTABLISHED
//-------------------------------------------------------------------------
void RegistrationWidget::slot_link_established(bool flg) {
    if(!flg) {
        emit logined_signal(false);
        gen_widg->show_message_box(tr("Error"), tr("Can't init connection"), 0, this);
        return;
    }
    emit get_id_for_client_signal();
}

//-------------------------------------------------------------------------
// SERVER RECEIVED USER ID
//-------------------------------------------------------------------------
void RegistrationWidget::slot_id_received(bool flg) {
    if(!flg) {
        emit logined_signal(false);
        gen_widg->show_message_box(tr("Error"), tr("Can't get ID"), 0, this);
        return;
    }
    emit send_login_register_data_signal(arr, flag);
    flag = -1;
}

//-------------------------------------------------------------------------
// USER ACCOUNT NOT APPROVED
//-------------------------------------------------------------------------
void RegistrationWidget::slot_not_approved() {
    gen_widg->show_message_box(tr("Error"), tr("Your account not approved"), 0, this);
    emit logined_signal(false);
}

//-------------------------------------------------------------------------
// CLIENT REGISTERED
//-------------------------------------------------------------------------
void RegistrationWidget::slot_client_registered(bool flg) {
    if(!flg) {
        gen_widg->show_message_box(tr("Error"), tr("The same login does already exist"), 0, this);
    }
    emit logined_signal(flg);
}

//-------------------------------------------------------------------------
// CLIENT LOGINED
//-------------------------------------------------------------------------
void RegistrationWidget::slot_client_logined(bool flg) {
    if(!flg) {
        gen_widg->show_message_box(tr("Error"), tr("You enter wrong login or password"), 0, this);
    }
    emit logined_signal(flg);
}

//-------------------------------------------------------------------------
// SET LOGINED USER NAME
//-------------------------------------------------------------------------
void RegistrationWidget::slot_set_user_fname_lname(QString f_name, QString l_name) {
    user_fname.clear();
    user_lname.clear();
    user_fname.append(f_name);
    user_lname.append(l_name);
}

//-------------------------------------------------------------------------
// GET REGISTERED USER FIRST NAME
//-------------------------------------------------------------------------
QString RegistrationWidget::get_user_fname() {
    return user_fname;
}

//-------------------------------------------------------------------------
// GET REGISTERED USER LAST NAME
//-------------------------------------------------------------------------
QString RegistrationWidget::get_user_lname() {
    return user_lname;
}

//-------------------------------------------------------------------------
// RETRANSLATING UI ON REGISTRATION WIDGET
//-------------------------------------------------------------------------
void RegistrationWidget::slot_re_translate() {
    ui->retranslateUi(this);
}
