#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_dialog_set_server_ip.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    gen_widg = new General_Widget();
    QDir::setCurrent(gen_widg->get_app_path());
    snd_rcv_module = new Send_Receive_Module(gen_widg);
    data_transfer_module = new Data_Transfer_Module(gen_widg);
    ptr_registration_widg = new RegistrationWidget(this, gen_widg, data_transfer_module, snd_rcv_module);
    ptr_RHE_widg = new RHE_Widget(this, gen_widg, data_transfer_module);
    ui->stackedWidget->addWidget(ptr_registration_widg);
    ui->stackedWidget->addWidget(ptr_RHE_widg);
    ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
    initialize_ui();
    set_ui_text();
    load_settings();
    connect(ptr_RHE_widg, &RHE_Widget::resize_signal, this, &MainWindow::slot_re_size);
    connect(ptr_RHE_widg, &RHE_Widget::set_disconnected_signal, snd_rcv_module, &Send_Receive_Module::set_disconnected);
    connect(gen_widg, &General_Widget::re_translate_signal, this, &MainWindow::slot_re_translate);
    connect(gen_widg, &General_Widget::re_translate_signal, ptr_registration_widg, &RegistrationWidget::slot_re_translate);
    connect(gen_widg, &General_Widget::re_translate_signal, ptr_RHE_widg, &RHE_Widget::slot_re_translate);
    connect(ptr_registration_widg, &RegistrationWidget::logined_signal, this, &MainWindow::logined);
    connect(this, &MainWindow::update_data_signal, data_transfer_module, &Data_Transfer_Module::get_update_data);
    connect(this, &MainWindow::set_disconnected_signal, snd_rcv_module, &Send_Receive_Module::set_disconnected);
    connect(snd_rcv_module, &Send_Receive_Module::logout_signal, this, &MainWindow::logout);
    connect(snd_rcv_module, &Send_Receive_Module::received_data, data_transfer_module, &Data_Transfer_Module::analyze_recv_data, Qt::QueuedConnection);
    connect(snd_rcv_module, &Send_Receive_Module::reset_ID_signal, data_transfer_module, &Data_Transfer_Module::reset_ID);
    connect(snd_rcv_module, &Send_Receive_Module::clear_FPGA_id_code_signal, data_transfer_module, &Data_Transfer_Module::clear_FPGA_id_code_slot);
    connect(data_transfer_module, &Data_Transfer_Module::data_updated_signal, this, &MainWindow::data_updated);
    connect(data_transfer_module, &Data_Transfer_Module::set_disconnected_signal, snd_rcv_module, &Send_Receive_Module::set_disconnected);
    connect(data_transfer_module, &Data_Transfer_Module::send_data_signal, snd_rcv_module, &Send_Receive_Module::send_data, Qt::QueuedConnection);
    snd_rcv_module->moveToThread(&thread_send_recv_mod);
    data_transfer_module->moveToThread(&thread_data_trnsfr_mod);
    thread_send_recv_mod.start();
    thread_data_trnsfr_mod.start();
    gen_widg->change_current_locale();
    state_strs = {tr("Connecting To Server"), tr("Updating Data"), ""};
    tmr_waveform_viewer = new QTimer(this);
    connect(tmr_waveform_viewer, &QTimer::timeout, this, &MainWindow::slot_timer_waveform_viewer_timeout);
    tmr_waveform_viewer->stop();
    tmr_progress_bar = new QTimer(nullptr);
    connect(tmr_progress_bar, &QTimer::timeout, this, &MainWindow::slot_timer_progress_bar_timeout);
}

MainWindow::~MainWindow() {
//    thread.terminate();
    thread_send_recv_mod.quit();
    thread_data_trnsfr_mod.quit();
    disconnect(tmr_waveform_viewer, &QTimer::timeout, this, &MainWindow::slot_timer_waveform_viewer_timeout);
    slot_timer_waveform_viewer_timeout();
    delete tmr_waveform_viewer;
    delete tmr_progress_bar;
    delete ptr_registration_widg;
    delete ptr_RHE_widg;
    delete cmbBx_lng_chs;
    delete cmbBx_lng_chs_actn;
//    delete snd_rcv_module;
//    delete data_transfer_module
    delete gen_widg;
    delete ui;
}

//-------------------------------------------------------------------------
// HACK TO DISPLAY WINDOW AT CENTRE OF SCREEN ON THE FIRST RUN
//-------------------------------------------------------------------------
void MainWindow::showEvent(QShowEvent *) {
    if(shw_at_cntr) {
        shw_at_cntr = false;
        this->move(((QGuiApplication::primaryScreen()->geometry().width() - this->geometry().width()) / 2), ((QGuiApplication::primaryScreen()->geometry().height() - this->geometry().height()) / 2));
    }
}

//-------------------------------------------------------------------------
// SAVING POSITION OF CURRENT ACTIVE WINDOW
//-------------------------------------------------------------------------
void MainWindow::moveEvent(QMoveEvent *) {
    gen_widg->set_position(QPoint((this->pos().x() + (this->width() / 2)), (this->pos().y() + (this->height() / 2))));
}

//-------------------------------------------------------------------------
// CLOSING OF APPLICATION
//-------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *) {
    pshBttn_exit();
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'WAVEFORM VIEWER' CLICKED
//-------------------------------------------------------------------------
void MainWindow::pshBttn_wvfrm_vwr() {
    wvfrm_vwr = new Waveform_Viewer_Widget(nullptr, gen_widg, true);
    connect(gen_widg, &General_Widget::re_translate_signal, wvfrm_vwr, &Waveform_Viewer_Widget::slot_re_translate);
    connect(wvfrm_vwr, &Waveform_Viewer_Widget::waveform_viewer_closed_signal, this, &MainWindow::slot_waveform_viewer_closed);
    wvfrm_vwr->initialize_ui();
    wvfrm_vwr->remove_all_data();
    wvfrm_vwr->show();
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'EXIT' CLICKED
//-------------------------------------------------------------------------
void MainWindow::pshBttn_exit() {
    QApplication::quit();
}

//-------------------------------------------------------------------------
// CHECK BOX 'FILES CHECKING' CHECKED
//-------------------------------------------------------------------------
void MainWindow::chkBx_fls_chckng_state_changed() {
    if(ui_initialized) {
        ptr_RHE_widg->pshBttn_chk_prj_stat_set_visible(ui->chkBx_fls_chckng_actn->isChecked());
        ptr_RHE_widg->pshBttn_chk_prj_stat_set_enabled(ui->chkBx_fls_chckng_actn->isChecked());
        if(!ui->chkBx_fls_chckng_actn->isChecked() && !ui->chkBx_ld_mnl_frmwr_actn->isChecked()) {
            ui->chkBx_ld_mnl_frmwr_actn->setChecked(!ui->chkBx_ld_mnl_frmwr_actn->isChecked());
        }
        ui->chkBx_ld_mnl_frmwr_actn->setEnabled(ui->chkBx_fls_chckng_actn->isChecked());
        ptr_RHE_widg->pshBttn_set_path_to_proj_set_visible(ui->chkBx_fls_chckng_actn->isChecked());
        if(ui->chkBx_pins_chckng_actn->isVisible()) {
            ui->chkBx_pins_chckng_actn->setEnabled(ui->chkBx_fls_chckng_actn->isChecked());
            gen_widg->save_setting("settings/ENABLE_PINS_CHEKING", (gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() * (-1)));
            ui->chkBx_pins_chckng_actn->setChecked(ui->chkBx_fls_chckng_actn->isChecked() && gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toBool());
        }
        gen_widg->save_setting("settings/ENABLE_FILE_CHEKING", (static_cast<int>(ui->chkBx_fls_chckng_actn->isChecked())));
        ptr_RHE_widg->pshBttn_snd_frmwr_set_enabled(/*(ptr_RHE_widg->svf_exist) && (static_cast<bool>(main_settings->value("settings/MANUALY_LOAD_FIRMWARE").toInt()))*/false);
        ptr_RHE_widg->pshBttn_chs_frmwr_set_enabled(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && (!ui->chkBx_fls_chckng_actn->isChecked() || ptr_RHE_widg->sof_exist));
    }
}

//-------------------------------------------------------------------------
// CHECK BOX 'PINS CHECKING' CHECKED
//-------------------------------------------------------------------------
void MainWindow::chkBx_pins_chckng_state_changed() {
    if(ui_initialized) {
        if(gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() != -1) {
            ptr_RHE_widg->pins_chk = ui->chkBx_pins_chckng_actn->isChecked();
            gen_widg->save_setting("settings/ENABLE_PINS_CHEKING", (static_cast<int>(ui->chkBx_pins_chckng_actn->isChecked())));
        }
    }
}

//-------------------------------------------------------------------------
// CHECK BOX 'MANUALY LOAD FIRMWARE' CHECKED
//-------------------------------------------------------------------------
void MainWindow::chkBx_ld_mnl_frmwr_state_changed() {
    if(ui_initialized) {
        ptr_RHE_widg->pshBttn_chs_frmwr_set_visible(ui->chkBx_ld_mnl_frmwr_actn->isChecked());
        gen_widg->save_setting("settings/MANUALY_LOAD_FIRMWARE", (static_cast<int>(ui->chkBx_ld_mnl_frmwr_actn->isChecked())));
        ptr_RHE_widg->pshBttn_chs_frmwr_set_enabled(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && (!gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool() || ptr_RHE_widg->sof_exist));
    }
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'SET SERVER IP' CLICKED
//-------------------------------------------------------------------------
void MainWindow::pshBttn_set_srvr_IP() {
    Dialog_Set_Server_IP set_server_ip(gen_widg, this);
    set_server_ip.exec();
}

//-------------------------------------------------------------------------
// CHOOSING LANGUAUGE OF APPLICATION
//-------------------------------------------------------------------------
void MainWindow::cmbBx_lng_chs_current_index_changed(int index) {
    if(ui_initialized && language_changed) {
        gen_widg->save_setting("settings/LANGUAGE", index);
        gen_widg->change_current_locale();
    }
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'LOGIN/LOGOUT' CLICKED
//-------------------------------------------------------------------------
void MainWindow::on_pshBttn_login_logout_clicked() {
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        login();
    } else {
        emit set_disconnected_signal();
    }
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'REGISTER' CLICKED
//-------------------------------------------------------------------------
void MainWindow::on_pshBttn_register_clicked() {
    if(ptr_registration_widg->register_user()) {
        on_pshBttn_login_logout_clicked();
    }
}

//-------------------------------------------------------------------------
// INITIALIZING OF UI COMPONENTS
//-------------------------------------------------------------------------
void MainWindow::initialize_ui() {
    QString qmenu_stylesheet = "QMenu { background-color: #F5F5F5; color: #000000; border: 1px solid black; } QMenu:disabled { color: #898989; } "
                               "QMenu::item:selected { background: #9D9D90; color: #FFFFFF; } QMenu::item:pressed { background: #5D5D50; color: #FFFFFF; }";
    ui->prgrssBr_cnnctn_stat->setValue(ui->prgrssBr_cnnctn_stat->minimum());
    ui->prgrssBr_cnnctn_stat->setStyleSheet("QProgressBar { border: 2px solid grey; border-radius: 5px; color: #FFFFFF; background-color: #000000; } "
                                            "QProgressBar::chunk { background-color: #0020FF; width: 10px; margin: 0.5px; }");
    ui->prgrssBr_cnnctn_stat->setVisible(false);
    ui->menu_bar->setStyleSheet("QMenuBar { background-color: #F5F5F5; color: #000000; } QMenu:disabled { color: #898989; } "
                                "QMenuBar::item:selected { background: #9D9D90; color: #FFFFFF; } QMenuBar::item:pressed { background: #5D5D50; color: #FFFFFF; }");
    ui->menu_file->setStyleSheet(qmenu_stylesheet);
    ui->menu_settings->setStyleSheet(qmenu_stylesheet);
    cmbBx_lng_chs = new QComboBox(ui->menu_settings);
    cmbBx_lng_chs->setMinimumHeight(21);
    cmbBx_lng_chs->setMaximumHeight(21);
    cmbBx_lng_chs->setStyleSheet("QComboBox { background-color: #F6F6F6; color: #000000; selection-background-color: #308CC6; selection-color: #FFFFFF; }"
                                 "QComboBox QAbstractItemView { background-color: #EFEFEF; color: #000000; selection-background-color: #308CC6; selection-color: #FFFFFF; }");
    cmbBx_lng_chs_actn = new QWidgetAction(ui->menu_settings);
    cmbBx_lng_chs_actn->setDefaultWidget(cmbBx_lng_chs);
    ui->menu_settings->addAction(cmbBx_lng_chs_actn);
    connect(ui->wvfrm_vwr_actn, &QAction::triggered, this, &MainWindow::pshBttn_wvfrm_vwr);
    connect(ui->ext_actn, &QAction::triggered, this, &MainWindow::pshBttn_exit);
    connect(ui->chkBx_fls_chckng_actn, &QAction::changed, this, &MainWindow::chkBx_fls_chckng_state_changed);
    connect(ui->chkBx_pins_chckng_actn, &QAction::changed, this, &MainWindow::chkBx_pins_chckng_state_changed);
    connect(ui->chkBx_ld_mnl_frmwr_actn, &QAction::changed, this, &MainWindow::chkBx_ld_mnl_frmwr_state_changed);
    connect(ui->pshBttn_set_srvr_ip, &QAction::triggered, this, &MainWindow::pshBttn_set_srvr_IP);
    connect(cmbBx_lng_chs, SIGNAL(currentIndexChanged(int)), this, SLOT(cmbBx_lng_chs_current_index_changed(int)));
}

//-------------------------------------------------------------------------
// UPDATE UI TEXT AFTER CHANGING LANGUAGE OF APPLICATION
//-------------------------------------------------------------------------
void MainWindow::set_ui_text() {
    language_changed = false;
    this->setWindowTitle(tr("Remote Hardware Education"));
    if(cmbBx_lng_chs->count() == 0) {
        cmbBx_lng_chs->addItem(tr("English"));
        cmbBx_lng_chs->addItem(tr("Ukrainian"));
        cmbBx_lng_chs->addItem(tr("Russian"));
    } else {
        cmbBx_lng_chs->setItemText(0, tr("English"));
        cmbBx_lng_chs->setItemText(1, tr("Ukrainian"));
        cmbBx_lng_chs->setItemText(2, tr("Russian"));
    }
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        ui->pshBttn_login_logout->setText(tr("Login"));
    } else {
        ui->pshBttn_login_logout->setText(tr("Logout"));
    }
    if(!ui->pshBttn_register->isHidden()) {
        ui->pshBttn_register->setText(tr("Register"));
    }
    state_strs = {tr("Connecting To Server"), ""};
    if(!ui->prgrssBr_cnnctn_stat->isHidden()) {
        ui->prgrssBr_cnnctn_stat->setFormat(state_strs.at(crrnt_state_strs));
    }
    language_changed = true;
}

//-------------------------------------------------------------------------
// LOAD SETTINGS OF MAIN PART UI
//-------------------------------------------------------------------------
void MainWindow::load_settings() {
    bool fls_chk = gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool();
    int pins_chk;
    if(gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() < 0) {
        pins_chk = false;
    } else {
        pins_chk = gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toBool();
    }
    bool ld_firmwr = gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool();
    ui->chkBx_pins_chckng_actn->setChecked(!pins_chk);
    ui->chkBx_fls_chckng_actn->setChecked(!fls_chk);
    ui->chkBx_ld_mnl_frmwr_actn->setChecked(!ld_firmwr);
    ui_initialized = true;
    cmbBx_lng_chs->setCurrentIndex(gen_widg->get_setting("settings/LANGUAGE").toInt());
    ui->chkBx_pins_chckng_actn->setChecked(pins_chk);
    ui->chkBx_pins_chckng_actn->setVisible(false);
    ui->chkBx_fls_chckng_actn->setChecked(fls_chk);
    ui->chkBx_ld_mnl_frmwr_actn->setChecked(ld_firmwr);
    ui->chkBx_pins_chckng_actn->setVisible(true);
}

//-------------------------------------------------------------------------
// ENABLE/DISABLE OF SOME PUSH BUTTONS ON LOGIN STARTS
//-------------------------------------------------------------------------
void MainWindow::set_enable_login_buttons(bool flg) {
    ui->prgrssBr_cnnctn_stat->setVisible(!flg);
    ui->pshBttn_register->setEnabled(flg);
    ui->pshBttn_login_logout->setEnabled(flg);
    ui->pshBttn_set_srvr_ip->setEnabled(flg);
    ui->prgrssBr_cnnctn_stat->setFormat(state_strs.at(crrnt_state_strs));
}

//-------------------------------------------------------------------------
// START OF USER SESSION ON SERVER
//-------------------------------------------------------------------------
void MainWindow::login() {
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        if(gen_widg->get_setting("settings/SERVER_IP").toString().compare("0.0.0.0", Qt::CaseInsensitive) == 0) {
            gen_widg->show_message_box(tr("Warning"), tr("Change server-IP in settings"), 0, gen_widg->get_position());
            return;
        } else if(gen_widg->get_setting("settings/SERVER_PORT").toInt() == -1) {
            gen_widg->show_message_box(tr("Warning"), tr("Change server-port in settings"), 0, gen_widg->get_position());
            return;
        }
        crrnt_state_strs = 0;
        tmr_progress_bar->setInterval(200);
        tmr_progress_bar->start();
        set_enable_login_buttons(false);
        ptr_registration_widg->login();
    }
}

//-------------------------------------------------------------------------
// USER LOGINED
//-------------------------------------------------------------------------
void MainWindow::logined(bool flg) {
    tmr_progress_bar->stop();
    ui->prgrssBr_cnnctn_stat->setValue(ui->prgrssBr_cnnctn_stat->minimum());
    if(flg) {
        gen_widg->check_is_data_dir_exist();
        crrnt_state_strs = 1;
        tmr_progress_bar->setInterval(200);
        tmr_progress_bar->start();
        emit update_data_signal();
    } else {
        crrnt_state_strs = 2;
    }
    set_enable_login_buttons(!flg);
}

//-------------------------------------------------------------------------
// USER DATA UPDATED
//-------------------------------------------------------------------------
void MainWindow::data_updated(bool flg) {
    tmr_progress_bar->stop();
    ui->prgrssBr_cnnctn_stat->setValue(ui->prgrssBr_cnnctn_stat->minimum());
    crrnt_state_strs = 2;
    set_enable_login_buttons(true);
    if(flg) {
        ptr_RHE_widg->initialize_ui();
        ptr_RHE_widg->set_fname_lname(ptr_registration_widg->get_user_fname() + " " + ptr_registration_widg->get_user_lname());
        ui->pshBttn_register->hide();
        ui->stackedWidget->setCurrentWidget(ptr_RHE_widg);
        ui->pshBttn_login_logout->setText(tr("Logout"));
    }
}

//-------------------------------------------------------------------------
// END OF USER SESSION ON SERVER
//-------------------------------------------------------------------------
void MainWindow::logout() {
    if(ui->stackedWidget->currentWidget() != ptr_registration_widg) {
        ui->pshBttn_login_logout->setText(tr("Login"));
        ui->pshBttn_register->show();
        ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
    } else {
        logined(false);
    }
}

//-------------------------------------------------------------------------
// HACK TO UPDATING SIZE OF ALL COMPONENTS OF UI
//-------------------------------------------------------------------------
void MainWindow::slot_re_size() {
    QSize cur_size = this->size();
    this->resize(QSize((this->width() + 1), (this->height() + 1)));
    this->updateGeometry();
    this->resize(cur_size);
}

//-------------------------------------------------------------------------
// START OF TIMER WHICH INDICATES CLOSING OF STANDALONE WAVEFORM VIEWER
//-------------------------------------------------------------------------
void MainWindow::slot_waveform_viewer_closed() {
    tmr_waveform_viewer->setInterval(100);
    tmr_waveform_viewer->start();
}

//-------------------------------------------------------------------------
// TIMEOUT OF TIMER WHICH INDICATES CLOSING OF STANDALONE WAVEFORM VIEWER
//-------------------------------------------------------------------------
void MainWindow::slot_timer_waveform_viewer_timeout() {
    tmr_waveform_viewer->stop();
    if(wvfrm_vwr != nullptr) {
        disconnect(gen_widg, &General_Widget::re_translate_signal, wvfrm_vwr, &Waveform_Viewer_Widget::slot_re_translate);
        disconnect(wvfrm_vwr, &Waveform_Viewer_Widget::waveform_viewer_closed_signal, this, &MainWindow::slot_waveform_viewer_closed);
        wvfrm_vwr->hide();
        delete wvfrm_vwr;
        wvfrm_vwr = nullptr;
    }
}

//-------------------------------------------------------------------------
// TIMEOUT OF PROGRESS BAR TIMER
//-------------------------------------------------------------------------
void MainWindow::slot_timer_progress_bar_timeout() {
    tmr_progress_bar->stop();
    int val = ui->prgrssBr_cnnctn_stat->value() + 5;
    if(val > ui->prgrssBr_cnnctn_stat->maximum()) {
        val = ui->prgrssBr_cnnctn_stat->minimum();
    }
    ui->prgrssBr_cnnctn_stat->setValue(val);
    tmr_progress_bar->setInterval(200);
    tmr_progress_bar->start();
}

//-------------------------------------------------------------------------
// RETRANSLATING UI ON BASIC WINDOW
//-------------------------------------------------------------------------
void MainWindow::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}

////////////////////////////////////////////////////DIALOG SET SERVER IP///////////////////////////////////////////////////
Dialog_Set_Server_IP::Dialog_Set_Server_IP(General_Widget *widg, QWidget *parent) : QDialog(parent), ui(new Ui::Dialog_Set_Server_IP) {
    ui->setupUi(this);
    gen_widg = widg;
    this->setWindowTitle(tr("Setting of server IP"));
    this->setFixedSize(450, 105);
    this->updateGeometry();
    octt_lst = new QList<QSpinBox *>{ui->spnBx_frst_octt, ui->spnBx_scnd_octt, ui->spnBx_thrd_octt, ui->spnBx_frth_octt};
    QList<QString> lst = gen_widg->get_setting("settings/SERVER_IP").toString().split(".");
    for(int i = 0; i < lst.count(); i++) {
        octt_lst->at(i)->setValue(lst.at(i).toInt());
    }
    int serv_port = gen_widg->get_setting("settings/SERVER_PORT").toInt();
    if(serv_port > -1) {
        ui->lnEdt_port->setText(QString::number(serv_port));
    }
    connect(ui->spnBx_frst_octt->findChild<QLineEdit *>(), SIGNAL(textEdited(const QString &)), this, SLOT(lineEdit_frst_octet_text_edited(const QString &)));
    connect(ui->spnBx_scnd_octt->findChild<QLineEdit *>(), SIGNAL(textEdited(const QString &)), this, SLOT(lineEdit_scnd_octet_text_edited(const QString &)));
    connect(ui->spnBx_thrd_octt->findChild<QLineEdit *>(), SIGNAL(textEdited(const QString &)), this, SLOT(lineEdit_thrd_octet_text_edited(const QString &)));
    connect(ui->spnBx_frth_octt->findChild<QLineEdit *>(), SIGNAL(textEdited(const QString &)), this, SLOT(lineEdit_frth_octet_text_edited(const QString &)));
    ui_initialized = true;
}

Dialog_Set_Server_IP::~Dialog_Set_Server_IP() {
    delete octt_lst;
    delete ui;
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'OK' CLICKED
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_pshBttn_ok_clicked() {
    if(ui->lnEdt_port->text().count() != 0) {
        int port = ui->lnEdt_port->text().toInt();
        if((port < 0) || (port > 65535)) {
            gen_widg->show_message_box(tr("Error"), tr("Wrong server port!"), 0, gen_widg->get_position());
        } else {
            QString ip = "";
            for(int i = 0; i < octt_lst->count(); i++) {
                if(i != 0) {
                    ip.append(".");
                }
                ip.append(QString::number(octt_lst->at(i)->value()));
            }
            gen_widg->save_setting("settings/SERVER_IP", ip);
            gen_widg->save_setting("settings/SERVER_PORT", port);
            QDialog::done(1);
        }
    } else {
        gen_widg->show_message_box("Error", tr("Set port value"), 0, gen_widg->get_position());
    }
}

//-------------------------------------------------------------------------
// PUSH BUTTON 'CANCEL' CLICKED
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_pshBttn_cncl_clicked() {
    QDialog::done(0);
}

//-------------------------------------------------------------------------
// SET FIRST OCTET OF SERVER IP
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_spnBx_frst_octt_valueChanged(int val) {
    spinBox_roll_value(ui->spnBx_frst_octt, val);
}

//-------------------------------------------------------------------------
// SET SECOND OCTET OF SERVER IP
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_spnBx_scnd_octt_valueChanged(int val) {
    spinBox_roll_value(ui->spnBx_scnd_octt, val);
}

//-------------------------------------------------------------------------
// SET THIRD OCTET OF SERVER IP
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_spnBx_thrd_octt_valueChanged(int val) {
    spinBox_roll_value(ui->spnBx_thrd_octt, val);
}

//-------------------------------------------------------------------------
// SET FOURTH OCTET OF SERVER IP
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_spnBx_frth_octt_valueChanged(int val) {
    spinBox_roll_value(ui->spnBx_frth_octt, val);
}

//-------------------------------------------------------------------------
// TEXT EDITED OF QLINEEDIT WHICH CHILD OF QSPINBOX 'FIRST_OCTET'
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::lineEdit_frst_octet_text_edited(const QString &val) {
    change_IP_item_focus(val.toInt(), 0, 1);
}

//-------------------------------------------------------------------------
// TEXT EDITED OF QLINEEDIT WHICH CHILD OF QSPINBOX 'SECOND_OCTET'
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::lineEdit_scnd_octet_text_edited(const QString &val) {
    change_IP_item_focus(val.toInt(), 1, 2);
}

//-------------------------------------------------------------------------
// TEXT EDITED OF QLINEEDIT WHICH CHILD OF QSPINBOX 'THIRD_OCTET'
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::lineEdit_thrd_octet_text_edited(const QString &val) {
    change_IP_item_focus(val.toInt(), 2, 3);
}

//-------------------------------------------------------------------------
// TEXT EDITED OF QLINEEDIT WHICH CHILD OF QSPINBOX 'FOURTH_OCTET'
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::lineEdit_frth_octet_text_edited(const QString &val) {
    change_IP_item_focus(val.toInt(), 3, 4);
}

//-------------------------------------------------------------------------
// SET SERVER PORT
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::on_lnEdt_port_textEdited(const QString &val) {
    if(ui_initialized) {
        QString tmp = val;
        bool end = false;
        while(!end) {
            int cnt = 0;
            for(int i = 0; i < tmp.count(); i++) {
                if(!tmp.at(i).isNumber()) {
                    tmp.remove(i, 1);
                    break;
                } else {
                    cnt++;
                }
            }
            if(cnt == tmp.count()) {
                end = true;
            }
        }
        ui->lnEdt_port->setText(tmp);
    }
}

//-------------------------------------------------------------------------
// ROLL SPIN BOX VALUE
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::spinBox_roll_value(QSpinBox *spnBx, int val) {
    if(ui_initialized) {
        if(val < 0) {
            spnBx->setValue(spnBx->maximum() - spnBx->singleStep());
        } else if((spnBx->maximum() == val) || (val > spnBx->maximum())) {
            spnBx->setValue(0);
        }
    }
}

//-------------------------------------------------------------------------
// CHANGE FOCUS OF IP-ITEM IF INPUTED VALUE HAS 3 DIGITS
//-------------------------------------------------------------------------
void Dialog_Set_Server_IP::change_IP_item_focus(int val, int crrnt_item, int next_item) {
    if(val > 99) {
        octt_lst->at(crrnt_item)->setValue(val);
        if(next_item == 4) {
            ui->lnEdt_port->setFocus();
            ui->lnEdt_port->selectAll();
        } else {
            octt_lst->at(next_item)->setFocus();
            octt_lst->at(next_item)->selectAll();
        }
    }
}
