#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    QDir::setCurrent(qApp->applicationDirPath());
    gen_widg = new General_Widget();
    snd_rcv_module = new Send_Recieve_Module(gen_widg->get_setting("settings/SERVER_IP").toString(), gen_widg->get_setting("settings/SERVER_PORT").toInt(), gen_widg);
    ptr_registration_widg = new RegistrationWidget(this, gen_widg, snd_rcv_module);
    ptr_RHE_widg = new RHE_Widget(this, gen_widg, snd_rcv_module);
    ui->stackedWidget->addWidget(ptr_registration_widg);
    ui->stackedWidget->addWidget(ptr_RHE_widg);
    ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
    initialize_ui();
    set_ui_text();
    load_settings();
    connect(ptr_RHE_widg, &RHE_Widget::resize_signal, this, &MainWindow::slot_re_size);
    connect(gen_widg, &General_Widget::re_translate_signal, this, &MainWindow::slot_re_translate);
    connect(gen_widg, &General_Widget::re_translate_signal, ptr_registration_widg, &RegistrationWidget::slot_re_translate);
    connect(gen_widg, &General_Widget::re_translate_signal, ptr_RHE_widg, &RHE_Widget::slot_re_translate);
    connect(snd_rcv_module, &Send_Recieve_Module::logout_signal, this, &MainWindow::logout);
    gen_widg->change_current_locale();
    tmr = new QTimer(this);
    connect(tmr, &QTimer::timeout, this, &MainWindow::slot_timer_timeout);
    tmr->stop();
}

MainWindow::~MainWindow() {
    disconnect(tmr, &QTimer::timeout, this, &MainWindow::slot_timer_timeout);
    slot_timer_timeout();
    delete tmr;
    delete ptr_registration_widg;
    delete ptr_RHE_widg;
    delete wvfrm_vwr_actn;
    delete ext_actn;
    delete chkBx_fls_chckng_actn;
    delete chkBx_pins_chckng_actn;
    delete chkBx_ld_mnl_frmwr_actn;
    delete cmbBx_lng_chs;
    delete cmbBx_lng_chs_actn;
    delete menu_file;
    delete menu_settngs;
    delete menu_bar;
    delete snd_rcv_module;
    delete gen_widg;
    delete ui;
}

void MainWindow::showEvent(QShowEvent *) {
    if(shw_at_cntr) {
        shw_at_cntr = false;
        this->move(((QGuiApplication::primaryScreen()->geometry().width() - this->geometry().width()) / 2), ((QGuiApplication::primaryScreen()->geometry().height() - this->geometry().height()) / 2));
        resizeEvent(nullptr);
    }
}

void MainWindow::resizeEvent(QResizeEvent *) {
    ui->horizontalLayoutWidget->setGeometry(0, (this->height() - ui->horizontalLayoutWidget->height()), this->width(), ui->horizontalLayoutWidget->height());
    ui->verticalLayoutWidget->resize(this->width(), (this->height() - ui->horizontalLayoutWidget->height()));
    ui->stackedWidget->setGeometry(0, (menu_bar->height() + ui->line_1->height()), this->width(), (ui->verticalLayoutWidget->height() - menu_bar->height() - (2 * ui->line_1->height())));
}

void MainWindow::closeEvent(QCloseEvent *) {
    onPshBttnExt();
}

void MainWindow::onPshBttnWvfrmVwr() {
    wvfrm_vwr = new Waveform_Viewer_Widget(nullptr, gen_widg, true);
    connect(gen_widg, &General_Widget::re_translate_signal, wvfrm_vwr, &Waveform_Viewer_Widget::slot_re_translate);
    connect(wvfrm_vwr, &Waveform_Viewer_Widget::waveform_viewer_closed_signal, this, &MainWindow::slot_waveform_viewer_closed);
    wvfrm_vwr->initialize_ui();
    wvfrm_vwr->remove_all_data();
    wvfrm_vwr->show();
}

void MainWindow::onPshBttnExt() {
    QApplication::quit();
}

void MainWindow::onChkBxFlsChckngStateChanged() {
    if(ui_initialized) {
        ptr_RHE_widg->pshBttn_chk_prj_stat_set_visible(chkBx_fls_chckng_actn->isChecked());
        ptr_RHE_widg->pshBttn_chk_prj_stat_set_enabled(chkBx_fls_chckng_actn->isChecked());
        if(!chkBx_fls_chckng_actn->isChecked() && !chkBx_ld_mnl_frmwr_actn->isChecked()) {
            chkBx_ld_mnl_frmwr_actn->setChecked(!chkBx_ld_mnl_frmwr_actn->isChecked());
        }
        chkBx_ld_mnl_frmwr_actn->setEnabled(chkBx_fls_chckng_actn->isChecked());
        ptr_RHE_widg->pshBttn_set_path_to_proj_set_visible(chkBx_fls_chckng_actn->isChecked());
        if(chkBx_pins_chckng_actn->isVisible()) {
            chkBx_pins_chckng_actn->setEnabled(chkBx_fls_chckng_actn->isChecked());
            gen_widg->save_setting("settings/ENABLE_PINS_CHEKING", (gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() * (-1)));
            chkBx_pins_chckng_actn->setChecked(chkBx_fls_chckng_actn->isChecked() && gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toBool());
        }
        gen_widg->save_setting("settings/ENABLE_FILE_CHEKING", (static_cast<int>(chkBx_fls_chckng_actn->isChecked())));
        ptr_RHE_widg->pshBttn_snd_frmwr_set_enabled(/*(ptr_RHE_widg->svf_exist) && (static_cast<bool>(main_settings->value("settings/MANUALY_LOAD_FIRMWARE").toInt()))*/false);
        ptr_RHE_widg->pshBttn_ld_frmwr_set_enabled(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && (!chkBx_fls_chckng_actn->isChecked() || ptr_RHE_widg->sof_exist));
    }
}

void MainWindow::onChkBxPinsChckngStateChanged() {
    if(ui_initialized) {
        if(gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() != -1) {
            ptr_RHE_widg->pins_chk = chkBx_pins_chckng_actn->isChecked();
            gen_widg->save_setting("settings/ENABLE_PINS_CHEKING", (static_cast<int>(chkBx_pins_chckng_actn->isChecked())));
        }
    }
}

void MainWindow::onChkBxLdMnlFrmwrStateChanged() {
    if(ui_initialized) {
        ptr_RHE_widg->pshBttn_ld_frmwr_set_visible(chkBx_ld_mnl_frmwr_actn->isChecked());
        gen_widg->save_setting("settings/MANUALY_LOAD_FIRMWARE", (static_cast<int>(chkBx_ld_mnl_frmwr_actn->isChecked())));
        ptr_RHE_widg->pshBttn_ld_frmwr_set_enabled(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && (!gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool() || ptr_RHE_widg->sof_exist));
    }
}

void MainWindow::onCmbBxLngChsCurrentIndexChanged(int index) {
    if(ui_initialized && language_changed) {
        gen_widg->save_setting("settings/LANGUAGE", index);
        gen_widg->change_current_locale();
    }
}

void MainWindow::on_button_login_logout_clicked() {
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        login();
    } else {
        snd_rcv_module->set_disconnected();
    }
}

void MainWindow::on_button_register_clicked() {
    if(ptr_registration_widg->register_user()) {
        on_button_login_logout_clicked();
    }
}

void MainWindow::initialize_ui() {
    menu_bar = new QMenuBar(this);
    menu_bar->setStyleSheet("QMenuBar { background-color: #F5F5F5 } QMenuBar::item:selected { background: #9D9D90; } QMenuBar::item:pressed { background: #5D5D50; }" );
    ui->verticalLayout->insertWidget(0, menu_bar);
    menu_file = new QMenu(menu_bar);
    menu_file->setStyleSheet("QMenu { background-color: #F5F5F5 } QMenu::item:selected { background: #9D9D90; }" );
    menu_bar->addMenu(menu_file);
    wvfrm_vwr_actn = new QAction(menu_file);
    menu_file->addAction(wvfrm_vwr_actn);
    menu_file->addSeparator();
    ext_actn = new QAction(menu_file);
    menu_file->addAction(ext_actn);
    menu_settngs = new QMenu(menu_bar);
    menu_settngs->setStyleSheet("QMenu { background-color: #F5F5F5 } QMenu::item:selected { background: #9D9D90; }" );
    menu_bar->addMenu(menu_settngs);
    chkBx_fls_chckng_actn = new QAction(menu_settngs);
    chkBx_fls_chckng_actn->setCheckable(true);
    menu_settngs->addAction(chkBx_fls_chckng_actn);
    chkBx_pins_chckng_actn = new QAction(menu_settngs);
    chkBx_pins_chckng_actn->setCheckable(true);
    menu_settngs->addAction(chkBx_pins_chckng_actn);
    chkBx_ld_mnl_frmwr_actn = new QAction(menu_settngs);
    chkBx_ld_mnl_frmwr_actn->setCheckable(true);
    menu_settngs->addAction(chkBx_ld_mnl_frmwr_actn);
    menu_settngs->addSeparator();
    cmbBx_lng_chs = new QComboBox(menu_settngs);
    cmbBx_lng_chs_actn = new QWidgetAction(menu_settngs);
    cmbBx_lng_chs_actn->setDefaultWidget(cmbBx_lng_chs);
    menu_settngs->addAction(cmbBx_lng_chs_actn);
    connect(wvfrm_vwr_actn, &QAction::triggered, this, &MainWindow::onPshBttnWvfrmVwr);
    connect(ext_actn, &QAction::triggered, this, &MainWindow::onPshBttnExt);
    connect(chkBx_fls_chckng_actn, &QAction::changed, this, &MainWindow::onChkBxFlsChckngStateChanged);
    connect(chkBx_pins_chckng_actn, &QAction::changed, this, &MainWindow::onChkBxPinsChckngStateChanged);
    connect(chkBx_ld_mnl_frmwr_actn, &QAction::changed, this, &MainWindow::onChkBxLdMnlFrmwrStateChanged);
    connect(cmbBx_lng_chs, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbBxLngChsCurrentIndexChanged(int)));
}

void MainWindow::set_ui_text() {
    language_changed = false;
    this->setWindowTitle(tr("Remote Hardware Education"));
    menu_file->setTitle(tr("File"));
    menu_settngs->setTitle(tr("Settings"));
    wvfrm_vwr_actn->setText(tr("Open waveform viewer"));
    ext_actn->setText(tr("Exit"));
    chkBx_fls_chckng_actn->setText(tr("Files checking"));
    chkBx_pins_chckng_actn->setText(tr("Pins checking"));
    chkBx_ld_mnl_frmwr_actn->setText(tr("Manualy load firmware"));
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
        ui->button_login_logout->setText(tr("Login"));
    } else {
        ui->button_login_logout->setText(tr("Logout"));
    }
    if(!ui->button_register->isHidden()) {
        ui->button_register->setText(tr("Register"));
    }
    language_changed = true;
}

void MainWindow::load_settings() {
    bool fls_chk = gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool();
    int pins_chk;
    if(gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toInt() < 0) {
        pins_chk = false;
    } else {
        pins_chk = gen_widg->get_setting("settings/ENABLE_PINS_CHEKING").toBool();
    }
    bool ld_firmwr = gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool();
    chkBx_pins_chckng_actn->setChecked(!pins_chk);
    chkBx_fls_chckng_actn->setChecked(!fls_chk);
    chkBx_ld_mnl_frmwr_actn->setChecked(!ld_firmwr);
    ui_initialized = true;
    cmbBx_lng_chs->setCurrentIndex(gen_widg->get_setting("settings/LANGUAGE").toInt());
    chkBx_pins_chckng_actn->setChecked(pins_chk);
    chkBx_pins_chckng_actn->setVisible(false);
    chkBx_fls_chckng_actn->setChecked(fls_chk);
    chkBx_ld_mnl_frmwr_actn->setChecked(ld_firmwr);
    chkBx_pins_chckng_actn->setVisible(true);
}

void MainWindow::login() {
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        if(ptr_registration_widg->login()) {
            ptr_RHE_widg->set_fname_lname(ptr_registration_widg->get_user_fname() + " " + ptr_registration_widg->get_user_lname());
            ui->button_register->hide();
            ui->stackedWidget->setCurrentWidget(ptr_RHE_widg);
            ui->button_login_logout->setText(tr("Logout"));
        }
    }
}

void MainWindow::logout() {
    if(ui->stackedWidget->currentWidget() != ptr_registration_widg) {
        ui->button_login_logout->setText(tr("Login"));
        ui->button_register->show();
        ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
        ptr_RHE_widg->initialize_ui();
    }
}

void MainWindow::slot_re_size() {
    QSize cur_size = this->size();
    this->resize(QSize((this->width() + 1), (this->height() + 1)));
    this->resize(cur_size);
}

void MainWindow::slot_waveform_viewer_closed() {
    tmr->setInterval(100);
    tmr->start();
}

void MainWindow::slot_timer_timeout() {
    tmr->stop();
    if(wvfrm_vwr != nullptr) {
        disconnect(gen_widg, &General_Widget::re_translate_signal, wvfrm_vwr, &Waveform_Viewer_Widget::slot_re_translate);
        disconnect(wvfrm_vwr, &Waveform_Viewer_Widget::waveform_viewer_closed_signal, this, &MainWindow::slot_waveform_viewer_closed);
        wvfrm_vwr->hide();
        delete wvfrm_vwr;
        wvfrm_vwr = nullptr;
    }
}

void MainWindow::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}
