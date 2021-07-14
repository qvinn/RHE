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
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
}

RegistrationWidget::~RegistrationWidget() {
    delete ui;
}

//-------------------------------------------------------------------------
// DISPLAYING OF REGISTRATION WIDGET
//-------------------------------------------------------------------------
void RegistrationWidget::showEvent(QShowEvent *) {
    resizeEvent(nullptr);
}

//-------------------------------------------------------------------------
// RESIZING OF REGISTRATION WIDGET
//-------------------------------------------------------------------------
void RegistrationWidget::resizeEvent(QResizeEvent *) {
    ui->verticalLayoutWidget_3->setGeometry(0, 0, this->width(), ui->verticalLayoutWidget_3->height());
    ui->horizontalLayoutWidget->setGeometry(ui->grpBx_rgstr->contentsRect().x(), ui->grpBx_rgstr->contentsRect().y(), ui->grpBx_rgstr->contentsRect().width(), ui->grpBx_rgstr->contentsRect().height());
    ui->verticalLayoutWidget_2->setGeometry(ui->grpBx_lgn->contentsRect().x(), ui->grpBx_lgn->contentsRect().y(), ui->grpBx_lgn->contentsRect().width(), ui->grpBx_lgn->contentsRect().height());
    ui->verticalSpacer_2->changeSize(ui->verticalSpacer_2->geometry().width(), (ui->grpBx_rgstr->contentsMargins().top() + ui->verticalSpacer_4->sizeHint().height()));
}

//-------------------------------------------------------------------------
// REGISTRATION OF USER
//-------------------------------------------------------------------------
bool RegistrationWidget::register_user() {
    if((ui->lineEdit_FName->text().length() != 0) && (ui->lineEdit_LName->text().length() != 0) && (ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        flag = FILE_REGIST;
        return true;
    } else {
        gen_widg->show_message_box(tr("Error"), tr("Enter login, password, first and last names"), 0, gen_widg->get_position());
        return false;
    }
}

//-------------------------------------------------------------------------
// LOGGING OF USER
//-------------------------------------------------------------------------
void RegistrationWidget::login() {
    if((ui->lineEdit_login->text().length() != 0) && (ui->lineEdit_password->text().length() != 0)) {
        user_fname.clear();
        user_lname.clear();
        user_fname.append(ui->lineEdit_FName->text());
        user_lname.append(ui->lineEdit_LName->text());
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
        gen_widg->show_message_box(tr("Error"), tr("Enter login and password"), 0, gen_widg->get_position());
    }
}

//-------------------------------------------------------------------------
// CONNECTION WITH SERVER ESTABLISHED
//-------------------------------------------------------------------------
void RegistrationWidget::slot_link_established(bool flg) {
    if(!flg) {
        emit logined_signal(false);
        gen_widg->show_message_box(tr("Error"), tr("Can't init connection"), 0, gen_widg->get_position());
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
        gen_widg->show_message_box(tr("Error"), tr("Can't get ID"), 0, gen_widg->get_position());
        return;
    }
    emit send_login_register_data_signal(arr, flag);
    flag = -1;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void RegistrationWidget::slot_not_approved() {
    gen_widg->show_message_box(tr("Error"), tr("Your account not approved"), 0, gen_widg->get_position());
    emit logined_signal(false);
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void RegistrationWidget::slot_client_registered(bool flg) {
    if(!flg) {
        gen_widg->show_message_box(tr("Error"), tr("The same login does already exist"), 0, gen_widg->get_position());
    }
    emit logined_signal(flg);
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void RegistrationWidget::slot_client_logined(bool flg) {
    if(!flg) {
        gen_widg->show_message_box(tr("Error"), tr("You enter wrong login or password"), 0, gen_widg->get_position());
    }
    emit logined_signal(flg);
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
