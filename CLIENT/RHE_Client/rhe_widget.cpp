#include "rhe_widget.h"
#include "ui_rhe_widget.h"

RHE_Widget::RHE_Widget(QWidget *parent, General_Widget *widg, Send_Recieve_Module *snd_rcv_mod) : QWidget(parent), ui(new Ui::RHE_Widget)  {
    ui->setupUi(this);
    this->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    gen_widg = widg;
    snd_rcv_module = snd_rcv_mod;
    path_to_proj = new QString("");
    prev_path_to_proj = new QString("");
    svf_file = new QFile();
    csv_file = new QFile();
    inpt_hbxs = new QList<QHBoxLayout*>();
    inpt_spcrs = new QList<QSpacerItem*>();
    inpt_lbls = new QList<QLabel*>();
    inpt_sldrs = new QList<QSlider*>();
    inpt_stts = new QList<QLCDNumber*>();
    pixmp_names = new QList<QString>();
    jtag_id_codes = new QList<QString>();
    led_colors = new QList<QString>();
    led_x_y = new QList<QPoint>();
    led_width_height = new QList<QPoint>();
    wvfrm_vwr = new Waveform_Viewer_Widget(this, gen_widg, false);
    connect(wvfrm_vwr, &Waveform_Viewer_Widget::as_window_signal, this, &RHE_Widget::slot_as_window);
    connect(gen_widg, &General_Widget::re_translate_signal, wvfrm_vwr, &Waveform_Viewer_Widget::slot_re_translate);
    connect(snd_rcv_module, &Send_Recieve_Module::fpga_flashed_signal, this, &RHE_Widget::slot_fpga_flashed);
    connect(snd_rcv_module, &Send_Recieve_Module::debug_not_started, this, &RHE_Widget::slot_end_sequence_of_signals);
    connect(snd_rcv_module, &Send_Recieve_Module::end_sequence_of_signals_signal, this, &RHE_Widget::slot_end_sequence_of_signals);
    connect(snd_rcv_module, &Send_Recieve_Module::firmware_file_recieved_signal, this, &RHE_Widget::slot_firmware_file_sended);
    connect(snd_rcv_module, &Send_Recieve_Module::sequence_file_recieved_signal, this, &RHE_Widget::slot_sequence_of_signals_file_sended);
    connect(snd_rcv_module, &Send_Recieve_Module::end_debugging_signal, this, &RHE_Widget::on_pshBttn_stp_dbg_clicked);
    connect(snd_rcv_module, &Send_Recieve_Module::choose_board_signal, this, &RHE_Widget::slot_choose_board);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_board_signal, this, &RHE_Widget::slot_accept_board);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_debug_time_limit_signal, this, &RHE_Widget::slot_accept_debug_time_limit);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_debug_data_signal, this, &RHE_Widget::slot_accept_debug_data);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_input_data_table_signal, this, &RHE_Widget::slot_accept_input_data_table);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_output_data_table_signal, this, &RHE_Widget::slot_accept_output_data_table);
    prev_vals = new QList<int>();
    pi_pins_nums = new QList<int>();
    send_file_status = new QTimer(nullptr);
    state_strs = {tr("Firmware sending"), tr("Firmware Sended"), tr("FPGA Flashing"), tr("FPGA Flashed"), tr("Debugging"), ""};
    connect(send_file_status, &QTimer::timeout, this, &RHE_Widget::slot_timer_timeout);
    pre_initialize_ui();
}

RHE_Widget::~RHE_Widget() {
    delete send_file_status;
    delete pi_pins_nums;
    delete inpt_hbxs;
    delete inpt_spcrs;
    delete inpt_lbls;
    delete inpt_sldrs;
    delete inpt_stts;
    delete pixmp_names;
    delete led_x_y;
    delete led_width_height;
    delete led_colors;
    delete jtag_id_codes;
    delete path_to_proj;
    delete prev_path_to_proj;
    delete csv_file;
    delete svf_file;
    delete wvfrm_vwr;
    delete prev_vals;
    delete ui;
}

void RHE_Widget::showEvent(QShowEvent *) {
    paintEvent(nullptr);
    resizeEvent(nullptr);
}

void RHE_Widget::paintEvent(QPaintEvent *) {
    if(!pixmp_brd.isNull() && !clr_trnsprnt) {
        QPainter painter(&pixmp_brd);
        QRect rectangle(led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).x(), led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).y(), led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).x(), led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).y());
        QColor clr;
        if(!board_is_on) {
            clr.setNamedColor("#00DEFDEF"); //00-Alpha DE-Red FD-Green EF-Blue
        } else {
            clr.setNamedColor(led_colors->at(ui->cmbBx_chs_brd->currentIndex()));
        }
        painter.setPen(QPen(clr, 1, Qt::SolidLine));
        painter.setBrush(QBrush(clr));
        if(clr.alpha() == 0) {
            clr_trnsprnt = true;
        } else {
            clr_trnsprnt = false;
            painter.drawRect(rectangle);
        }
    }
}

void RHE_Widget::resizeEvent(QResizeEvent *) {
    ui->horizontalLayoutWidget->setGeometry(0, (this->height() - ui->horizontalLayoutWidget->height()), this->width(), ui->horizontalLayoutWidget->height());
    ui->line_horizontal->setGeometry(0, (ui->horizontalLayoutWidget->y() - 1), this->width(), ui->line_horizontal->geometry().height());
    ui->widget->resize(this->width(), (this->height() - ui->horizontalLayoutWidget->height()));
    ui->horizontalLayoutWidget_2->setGeometry(0, 0, ui->widget->width(), ui->widget->height());
    ui->verticalLayout_3->setGeometry(QRect((ui->widget->width() - ui->verticalLayout_3->geometry().width()), 0, ui->verticalLayout_3->geometry().width(), ui->widget->height()));
    if(!language_changed) {
        ui->prgrssBr_fl_sts->resize(ui->prgrssBr_fl_sts->minimumWidth(), ui->prgrssBr_fl_sts->height());
    }
    ui->scrollArea->setMinimumSize(ui->prgrssBr_fl_sts->width(), 0);
    ui->scrollArea->adjustSize();
    ui->scrollArea->setMinimumSize(ui->scrollArea->width(), (ui->scrollAreaWidgetContents->height() + 2));
    if(!wvfrm_vwr->as_window) {
        ui->label->resize(ui->verticalLayout_3->geometry().width(), (ui->verticalLayout_3->geometry().height() - wvfrm_vwr->height() - ui->verticalLayout_3->contentsMargins().bottom() * 2));
    }
    if(!pixmp_brd.isNull()) {
        ui->label->setPixmap(pixmp_brd.scaled(ui->label->size(), Qt::KeepAspectRatio));
    }
}

void RHE_Widget::on_pshBttn_strt_dbg_clicked() {
    set_enable_board_power_led(true);
    new_debug = true;
    int flag;
    send_file_status->stop();
    if(static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState())) && sqnc_of_sgnls_file_sended && static_cast<bool>(static_cast<int>(ui->chckBx_strt_dbg_aftr_flsh->checkState()))) {
        flag = CLIENT_WANT_FLASH_ALL_SYNC;
        sqnc_of_sgnls_strtd = true;
        scrll_area_sgnls_set_enabled(false);
        crrnt_state_strs = 2;
        send_file_status->setInterval(200);
        send_file_status->start();
    } else {
        flag = CLIENT_WANT_START_DEBUG;
        if(static_cast<bool>(static_cast<int>(ui->chckBx_strt_dbg_aftr_flsh->checkState()))) {
            crrnt_state_strs = 4;
        }
    }
    ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    snd_rcv_module->start_debug(static_cast<uint16_t>(ui->spnBx_dbg_tm->value()), static_cast<uint8_t>(ui->cmbBx_dbg_tm_tp->currentIndex()), flag);
    if(inpt_sldrs->count() != 0) {
        slot_input_val_changed(inpt_sldrs->at(0)->value());
    }
    if(static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState())) && sqnc_of_sgnls_file_sended && !static_cast<bool>(static_cast<int>(ui->chckBx_strt_dbg_aftr_flsh->checkState()))) {
        on_pshBttn_strt_sgnls_sqnc_clicked();
    }
    set_button_state_debug(true);
}

void RHE_Widget::on_pshBttn_stp_dbg_clicked() {
    crrnt_state_strs = 5;
    ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    snd_rcv_module->stop_debug();
    set_button_state_debug(false);
}

void RHE_Widget::on_chckBx_strt_dbg_aftr_flsh_stateChanged(int state) {
    if(ui_initialized) {
        gen_widg->save_setting("settings/START_DEBUG_AFTER_FPGA_FLASHING", state);
        ui->pshBttn_strt_dbg->setEnabled(!static_cast<bool>(state));
    }
}

void RHE_Widget::on_pshBttn_chs_sgnls_sqnc_clicked() {
    QString str = gen_widg->load_file_path(tr("Choose csv-file with sequence of signals"), tr("Comma-Separated Values files (*.csv)"));
    if(str.length() != 0) {
        csv_file->setFileName(str);
        ui->pshBttn_snd_sgnls_sqnc->setEnabled(true);
        pshBttn_strt_sgnls_sqnc_set_enabled(false);
    } else {
        gen_widg->show_message_box("", tr("File with sequence of signals not choosed"), 0);
    }
}

void RHE_Widget::on_pshBttn_snd_sgnls_sqnc_clicked() {
    if(csv_file->open(QIODevice::ReadOnly)) {
        snd_rcv_module->send_file_to_ss(csv_file->readAll(), CLIENT_START_SEND_DSQ_FILE, CLIENT_SENDING_DSQ_FILE, CLIENT_FINISH_SEND_DSQ_FILE);
        csv_file->close();
    }
}

void RHE_Widget::on_pshBttn_strt_sgnls_sqnc_clicked() {
    if(dgb_strtd) {
        sqnc_of_sgnls_strtd = true;
        snd_rcv_module->start_sequence_of_signals();
        scrll_area_sgnls_set_enabled(false);
    } else {
        gen_widg->show_message_box("", tr("Debug not started"), 0);
    }
}

void RHE_Widget::on_chckBx_strt_sqnc_of_sgn_with_dbg_stateChanged(int state) {
    if(ui_initialized) {
        gen_widg->save_setting("settings/START_SEQUENCE_OF_SIGNALS_WITH_DEBUG", state);
        ui->pshBttn_strt_sgnls_sqnc->setEnabled(!(static_cast<bool>(state)) && sqnc_of_sgnls_file_sended);
    }
}

void RHE_Widget::on_cmbBx_chs_brd_currentIndexChanged(int index) {
    if(ui_initialized && (index != -1)) {
        gen_widg->save_setting("settings/CURRENT_BOARD", index);
        snd_rcv_module->set_FPGA_id(jtag_id_codes->at(index));
        pshBttn_snd_frmwr_set_enabled(false);
        if(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool()) {
            pshBttn_ld_frmwr_set_enabled(false);
        }
        wvfrm_vwr->debug_show = true;
        change_board_pixmap();
        set_enable_board_power_led(false);
    }
}

void RHE_Widget::on_spnBx_dbg_tm_valueChanged(int value) {
    if(ui_initialized) {
        gen_widg->save_setting("settings/DEBUG_DISCRETENESS_TIME", value);
    }
}

void RHE_Widget::on_cmbBx_dbg_tm_tp_currentIndexChanged(int index) {
    if(ui_initialized && language_changed) {
        gen_widg->save_setting("settings/DEBUG_DISCRETENESS_TIME_TYPE", index);
        ui->spnBx_dbg_tm->setMinimum(pow(10, index));
    }
}

void RHE_Widget::on_pshBttn_set_path_to_proj_clicked() {
    QStringList *lst = gen_widg->load_files(false, true, "", "");
    if(lst == nullptr) {
        return;
    }
    path_to_proj->clear();
    path_to_proj->append(lst->first());
    check_is_proj_folder(false);
    path_exist = qpf_exist;
}

void RHE_Widget::on_pshBttn_chk_prj_stat_clicked() {
    check_is_proj_folder(path_exist);
}

void RHE_Widget::on_pshBttn_ld_frmwr_clicked() {
    QString str = gen_widg->load_file_path(tr("Choose svf-file with firmware"), tr("Serial Vector Format files (*.svf)"));
    if(str.length() != 0) {
        if(!gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool()) {
            QRegExp tag_exp_path("/");
            QStringList path_lst = str.split(tag_exp_path);
            path_to_proj->clear();
            path_to_proj->append(str);
            path_to_proj->replace(("/" + path_lst.last()), "");
            check_is_proj_folder(false);
        }
        svf_exist = true;
        pshBttn_snd_frmwr_set_enabled(svf_exist);
        svf_file->setFileName(str);
    }
}

void RHE_Widget::on_pshBttn_snd_frmwr_clicked() {
    if(!svf_exist) {
        gen_widg->show_message_box("", tr("svf-file not generated"), 0);
        return;
    }
    if(svf_file->open(QIODevice::ReadOnly)) {
        send_file_status->setInterval(200);
        send_file_status->start();
        crrnt_state_strs = 0;
        ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
        snd_rcv_module->send_file_to_ss(svf_file->readAll(), CLIENT_START_SEND_FILE, CLIENT_SENDING_FILE, CLIENT_FINISH_SEND_FILE);
        svf_file->close();
    }
}

void RHE_Widget::pshBttn_set_path_to_proj_set_visible(bool flag) {
    ui->pshBttn_set_path_to_proj->setVisible(flag);
}

void RHE_Widget::pshBttn_chk_prj_stat_set_visible(bool flag) {
    ui->pshBttn_chk_prj_stat->setVisible(flag);
}

void RHE_Widget::pshBttn_ld_frmwr_set_visible(bool flag) {
    ui->pshBttn_ld_frmwr->setVisible(flag);
}

void RHE_Widget::pshBttn_chk_prj_stat_set_enabled(bool flag) {
    ui->pshBttn_chk_prj_stat->setEnabled(flag && (static_cast<bool>(path_to_proj->count())));
}

void RHE_Widget::pshBttn_ld_frmwr_set_enabled(bool flag) {
    ui->pshBttn_ld_frmwr->setEnabled(flag);
}

void RHE_Widget::pshBttn_snd_frmwr_set_enabled(bool flag) {
    ui->pshBttn_snd_frmwr->setEnabled(flag);
}

void RHE_Widget::pshBttn_strt_sgnls_sqnc_set_enabled(bool flag) {
    ui->pshBttn_strt_sgnls_sqnc->setEnabled(flag);
}

void RHE_Widget::scrll_area_sgnls_set_enabled(bool flag) {
    ui->scrollArea->setEnabled(flag);
}

void RHE_Widget::pre_initialize_ui() {
    wvfrm_vwr->initialize_ui();
    ui->verticalLayout_3->addWidget(wvfrm_vwr);
    initialize_ui();
}

void RHE_Widget::initialize_ui() {
    ui_initialized = false;
    send_file_status->stop();
    ui->prgrssBr_fl_sts->setValue(ui->prgrssBr_fl_sts->minimum());
    on_pshBttn_stp_dbg_clicked();
    path_to_proj->clear();
    QDir::setCurrent(qApp->applicationDirPath());
    pshBttn_snd_frmwr_set_enabled(false);
    pshBttn_chk_prj_stat_set_enabled(false);
    ui->pshBttn_snd_sgnls_sqnc->setEnabled(false);
    ui->prgrssBr_fl_sts->setStyleSheet("QProgressBar { border: 2px solid grey; border-radius: 5px; color: #FFFFFF; background-color: #000000; } QProgressBar::chunk { background-color: #0020FF; width: 10px; margin: 0.5px; }");
    ui->prgrssBr_fl_sts->setValue(0);
    ui->chckBx_strt_dbg_aftr_flsh->setCheckState(static_cast<Qt::CheckState>(abs(gen_widg->get_setting("settings/START_DEBUG_AFTER_FPGA_FLASHING").toInt() - 2)));
    ui->chckBx_strt_sqnc_of_sgn_with_dbg->setCheckState(static_cast<Qt::CheckState>(abs(gen_widg->get_setting("settings/START_SEQUENCE_OF_SIGNALS_WITH_DEBUG").toInt() - 2)));
    if(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool()) {
        pshBttn_ld_frmwr_set_enabled(false);
    }
    if(ui->cmbBx_chs_brd->count() == 0) {
        if(read_xml_file(false)) {
            post_initialize_ui();
        } else {
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        }
    } else {
        post_initialize_ui();
    }
}

void RHE_Widget::post_initialize_ui() {
    ui_initialized = true;
    set_ui_text();
    prev_board_index = gen_widg->get_setting("settings/CURRENT_BOARD").toInt();
    ui->cmbBx_chs_brd->setCurrentIndex(prev_board_index);
    change_cnt_of_dbg_pins(16);
    ui->spnBx_dbg_tm->setValue(gen_widg->get_setting("settings/DEBUG_DISCRETENESS_TIME").toInt());
    ui->cmbBx_dbg_tm_tp->setCurrentIndex(gen_widg->get_setting("settings/DEBUG_DISCRETENESS_TIME_TYPE").toInt());
    ui->chckBx_strt_dbg_aftr_flsh->setCheckState(static_cast<Qt::CheckState>(gen_widg->get_setting("settings/START_DEBUG_AFTER_FPGA_FLASHING").toInt()));
    ui->chckBx_strt_sqnc_of_sgn_with_dbg->setCheckState(static_cast<Qt::CheckState>(gen_widg->get_setting("settings/START_SEQUENCE_OF_SIGNALS_WITH_DEBUG").toInt()));
    ui->chckBx_strt_sqnc_of_sgn_with_dbg->setEnabled(false);
    pshBttn_strt_sgnls_sqnc_set_enabled(false);
    set_enable_board_power_led(false);
}

void RHE_Widget::set_ui_text() {
    language_changed = false;
    ui->pshBttn_strt_dbg->setText(tr("Start Debug"));
    ui->pshBttn_stp_dbg->setText(tr("Stop Debug"));
    ui->lbl_FName_LName->setText(tr("Hello, ") + lname_fname);
    if(ui->cmbBx_dbg_tm_tp->count() == 0) {
        ui->cmbBx_dbg_tm_tp->addItem(tr("s"));
        ui->cmbBx_dbg_tm_tp->addItem(tr("ms"));
//        ui->cmbBx_dbg_tm_tp->addItem(tr("us"));
    } else {
        ui->cmbBx_dbg_tm_tp->setItemText(0, tr("s"));
        ui->cmbBx_dbg_tm_tp->setItemText(1, tr("ms"));
//        ui->cmbBx_dbg_tm_tp->setItemText(2, tr("us"));
    }
    state_strs = {tr("Firmware sending"), tr("Firmware Sended"), tr("FPGA Flashing"), tr("FPGA Flashed"), tr("Debugging"), ""};
    ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    ui->lbl_dbg_tm_tp_lmt->setText(ui->cmbBx_dbg_tm_tp->itemText(dbg_tm_tp_lmt));
    resizeEvent(nullptr);
    language_changed = true;
}

void RHE_Widget::set_fname_lname(QString str) {
    lname_fname.clear();
    lname_fname.append(str);
    set_ui_text();
}

void RHE_Widget::change_cnt_of_dbg_pins(int value) {
    wvfrm_vwr->plot_re_scale = true;
    prev_vals->clear();
    for(int i = 0; i < wvfrm_vwr->get_all_pins_count(); i++) {
        prev_vals->append(0);
        prev_vals->append(0);
    }
    wvfrm_vwr->remove_graphs_from_plot();
    wvfrm_vwr->graph_count = value;
    wvfrm_vwr->re_scale_graph();
    wvfrm_vwr->add_graphs_to_plot();
    wvfrm_vwr->plot_re_scale = false;
    wvfrm_vwr->re_scale_graph();
}

void RHE_Widget::change_board_pixmap() {
    if(pixmp_names->count() != 0) {
        if(pixmp_names->at(ui->cmbBx_chs_brd->currentIndex()).count() == 0) {
            ui->label->clear();
            QPixmap tmp;
            pixmp_brd.swap(tmp);
        } else {
            if(pixmp_brd.load(qApp->applicationDirPath() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString() + pixmp_names->at(ui->cmbBx_chs_brd->currentIndex()))){
                showEvent(nullptr);
                ui->label->setPixmap(pixmp_brd.scaled(ui->label->size(), Qt::KeepAspectRatio));
            }
        }
    }
}

void RHE_Widget::set_enable_board_power_led(bool flg) {
    if((led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).x() != -1) || (led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).y() != -1) || (led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).x() != -1) || (led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).y() != -1) || (led_colors->at(ui->cmbBx_chs_brd->currentIndex()).count() == 9)) {
        board_is_on = flg;
        clr_trnsprnt = false;
        showEvent(nullptr);
        if(clr_trnsprnt) {
            change_board_pixmap();
        }
    }
}

void RHE_Widget::set_button_state_debug(bool flg) {
    dgb_strtd = flg;
    wvfrm_vwr->debugging = flg;
    wvfrm_vwr->pshBttn_slct_dsplbl_pins_set_enabled(!flg);
    wvfrm_vwr->pshBttn_open_save_wvfrm_set_enabled(!flg);
    ui->pshBttn_strt_dbg->setEnabled(!flg && !static_cast<bool>(static_cast<int>(ui->chckBx_strt_dbg_aftr_flsh->checkState())));
    ui->spnBx_dbg_tm->setEnabled(!flg);
    ui->cmbBx_dbg_tm_tp->setEnabled(!flg);
    pshBttn_chk_prj_stat_set_enabled(!flg);
    pshBttn_ld_frmwr_set_enabled(!flg);
    pshBttn_snd_frmwr_set_enabled(!flg && svf_exist);
}

void RHE_Widget::check_is_proj_folder(bool folder_exist) {
    qpf_exist = false;
    fit_exist = false;
    sof_exist = false;
    svf_exist = false;
    path_exist = false;
    if(!ui->pshBttn_chk_prj_stat->isVisible()) {
        QDir::setCurrent(*path_to_proj);
        prev_path_to_proj->clear();
        prev_path_to_proj->append(*path_to_proj);
        return;
    }
    QDirIterator proj_files(*path_to_proj, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
    QRegExp tag_exp_path("/");
    QString path_to_fit_file = "";
    QStringList path_lst = path_to_proj->split(tag_exp_path);
    if(path_lst.at(1).count() == 0) {
        prev_path_to_proj->clear();
        prev_path_to_proj->append(*path_to_proj);
        gen_widg->show_message_box("", tr("Root-folder cannot be as project-folder"), 0);
        return;
    }
    while(proj_files.hasNext()) {
        proj_files.next();
        if(!folder_exist) {
            if(proj_files.filePath().contains(".qpf", Qt::CaseInsensitive)) {
                QStringList name_lst = proj_files.fileName().split(QLatin1Char('.'));
                if(path_lst.last().compare(name_lst.first(), Qt::CaseInsensitive) == 0) {
                    qpf_exist = true;
                    pshBttn_chk_prj_stat_set_enabled(qpf_exist);
                    QDir::setCurrent(*path_to_proj);
                }
            }
        }
        if(proj_files.filePath().contains(".fit.rpt", Qt::CaseInsensitive)) {
            QStringList name_lst = proj_files.fileName().split(QLatin1Char('.'));
            if(path_lst.last().compare(name_lst.first(), Qt::CaseInsensitive) == 0) {
                fit_exist = true;
                path_to_fit_file.append(proj_files.filePath());
            }
        } else if(proj_files.filePath().contains(".sof", Qt::CaseInsensitive)) {
            QStringList name_lst = proj_files.fileName().split(QLatin1Char('.'));
            if(path_lst.last().compare(name_lst.first(), Qt::CaseInsensitive) == 0) {
                sof_exist = true;
            }
        } else if(proj_files.filePath().contains(".svf", Qt::CaseInsensitive)) {
            QStringList name_lst = proj_files.fileName().split(QLatin1Char('.'));
            if(path_lst.last().compare(name_lst.first(), Qt::CaseInsensitive) == 0) {
                svf_exist = true;
                if(!gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool()) {
                    svf_file->setFileName(proj_files.filePath());
                }
            }
        }
    }
    if(!folder_exist) {
        if(!qpf_exist) {
            path_to_proj->clear();
            path_to_proj->append(*prev_path_to_proj);
            gen_widg->show_message_box("", tr("Folder with project not chosen"), 0);
            return;
        }
    }
    if(!fit_exist) {
        gen_widg->show_message_box("", tr("Project not fittered"), 0);
        return;
    }
    if(pins_chk) {
        if(!check_fpga_connections(path_to_fit_file)) {
            pshBttn_snd_frmwr_set_enabled(false);
            return;
        }
    }
    if(!sof_exist) {
        gen_widg->show_message_box("", tr("sof-file not generated"), 0);
        return;
    }
    pshBttn_ld_frmwr_set_enabled(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool());
    pshBttn_snd_frmwr_set_enabled(svf_exist && !gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool());
    if(!svf_exist) {
        svf_file->setFileName("");
        gen_widg->show_message_box("", tr("svf-file not generated"), 0);
        return;
    }
}

bool RHE_Widget::check_fpga_connections(QString path_to_fit_rprtr) {
    pshBttn_ld_frmwr_set_enabled(false);
    QString cur_fpga = "";
    QList<QString> rght_pins_numb;
    QList<QString> rght_pins_dir;
    QList<QString> rght_pins_io_stndrd;
    rght_pins_numb.clear();
    rght_pins_dir.clear();
    rght_pins_io_stndrd.clear();
    if(!read_xml_file(true, &cur_fpga, &rght_pins_numb, &rght_pins_dir, &rght_pins_io_stndrd)) {
        return false;
    }
    QFile fit_file(path_to_fit_rprtr);
    int mode = -1;
    bool rght_board = false;
    QRegExp tagExp(";");
    QList<QString> cur_pins_numb;
    QList<QString> cur_pins_dir;
    QList<QString> cur_pins_io_stndrd;
    cur_pins_numb.clear();
    cur_pins_dir.clear();
    cur_pins_io_stndrd.clear();
    if(fit_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while(!fit_file.atEnd()) {
            QString str = fit_file.readLine();
            if((str.contains("; Fitter Settings", Qt::CaseSensitive)) || (str.contains("; Location ; Pad Number ; I/O Bank", Qt::CaseSensitive)) || ((str.contains("+----------+", Qt::CaseSensitive)) && ((mode == 1) || (mode == 2)))) {
                mode++;
                continue;
            }
            if(mode == 0) {
                if((str.contains("; Device    ", Qt::CaseSensitive)) && (str.contains(cur_fpga, Qt::CaseInsensitive))) {
                    rght_board = true;
                }
            } else if(mode == 2) {
                if(!rght_board) {
                    break;
                }
                QStringList lst = str.split(tagExp);
                cur_pins_numb.append(static_cast<QString>(lst.at(1)).replace(" ", ""));
                cur_pins_dir.append(static_cast<QString>(lst.at(5)).replace(" ", ""));
                cur_pins_io_stndrd.append(static_cast<QString>(lst.at(6)).replace(" ", ""));
            } else if(mode == 3) {
                break;
            }
        }
        fit_file.close();
    }
    if(cur_pins_numb.count() < rght_pins_numb.count()) {
        gen_widg->show_message_box("", (tr("Count of pins in board list are greater than in project(for current FPGA: ") + cur_fpga + ")\nPlease, contact with teacher or administrator"), 0);
        return false;
    }
    int pins_cnt = 0;
    for(int i = 0; i < rght_pins_numb.count(); i++) {
        for(int k = 0; k < cur_pins_numb.count(); k++) {
            if(cur_pins_numb.at(k).compare(rght_pins_numb.at(i), Qt::CaseInsensitive) == 0) {
                if(cur_pins_dir.at(k).compare(rght_pins_dir.at(i), Qt::CaseInsensitive) != 0) {
                    gen_widg->show_message_box("", (tr("In project, direction '") + cur_pins_dir.at(k) + tr("' for pin ") + cur_pins_numb.at(k) + tr(" isn't correct, set '") + rght_pins_dir.at(i) + ("' direction")), 0);
                    return false;
                } else if(cur_pins_io_stndrd.at(k).compare(rght_pins_io_stndrd.at(i), Qt::CaseInsensitive) != 0) {
                    gen_widg->show_message_box("", (tr("In project, I/O Standart '") + cur_pins_io_stndrd.at(k) + tr("' for pin ") + cur_pins_numb.at(k) + tr(" isn't correct, set '") + rght_pins_io_stndrd.at(i) + ("' I/O Standart")), 0);
                    return false;
                } else {
                    pins_cnt++;
                }
            }
        }
    }
    if(!rght_board) {
        gen_widg->show_message_box("", (tr("FPGA in project isn't ") + cur_fpga + tr(" for board ") + (ui->cmbBx_chs_brd->itemText(ui->cmbBx_chs_brd->currentIndex()))), 0);
        return false;
    } else if((pins_cnt != rght_pins_numb.count()) && (cur_pins_numb.count() != 0)) {
        gen_widg->show_message_box("", (tr("Pins in board list doesn't exist in project(for current FPGA: ") + cur_fpga + ")\nPlease, contact with teacher or administrator"), 0);
        return false;
    }
    return true;
}

bool RHE_Widget::read_xml_file(bool read_board_params, QString *cur_fpga, QList<QString> *pins_numb, QList<QString> *pins_typ, QList<QString> *pins_io_stndrd) {
    QString fl_lst_str = tr("file-list of boards and their parameters");
    QFile xml_file(qApp->applicationDirPath() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString() + gen_widg->get_setting("settings/BOARDS_LIST_FILENAME").toString());
    if(!xml_file.open(QFile::ReadOnly | QFile::Text)){
        gen_widg->show_message_box("", (tr("Cannot open ") + fl_lst_str), 0);
        return false;
    }
    QXmlStreamReader xmlReader(&xml_file);
    bool read_board_data = false;
    while(!xmlReader.atEnd() && !xmlReader.hasError()) {
        xmlReader.readNext();
        if(read_board_data) {
            if((xmlReader.name().toString().compare("pin", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("number"))) {
                pins_numb->append(xmlReader.attributes().value("number").toString().replace(" ", ""));
            }
            if((xmlReader.name().toString().compare("pin", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("direction"))) {
                pins_typ->append(xmlReader.attributes().value("direction").toString().replace(" ", ""));
            }
            if((xmlReader.name().toString().compare("pin", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("io_standard"))) {
                pins_io_stndrd->append(xmlReader.attributes().value("io_standard").toString().replace(" ", ""));
            }
            if((xmlReader.name().toString().compare("fpga", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("name"))) {
                cur_fpga->append(xmlReader.attributes().value("name").toString().replace(" ", ""));
            } else if(xmlReader.name().toString().compare("board", Qt::CaseInsensitive) == 0) {
                read_board_data = false;
                break;
            }
        } else if((xmlReader.name().toString().compare("board", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("name"))) {
            if(read_board_params) {
                if(xmlReader.attributes().value("name").toString().compare(ui->cmbBx_chs_brd->itemText(ui->cmbBx_chs_brd->currentIndex()), Qt::CaseInsensitive) == 0) {
                    read_board_data = true;
                }
            } else {
                ui->cmbBx_chs_brd->addItem(xmlReader.attributes().value("name").toString());
            }
        } else if(!read_board_params && (xmlReader.name().toString().compare("fpga", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("jtag_id_code"))) {
            QString tmp_str(xmlReader.attributes().value("jtag_id_code").toString().replace(" ", ""));
            if(tmp_str.count() != 0) {
                jtag_id_codes->append(tmp_str);
            } else {
                gen_widg->show_message_box("", (tr("'jtag_id_code' in board list doesn't exist for board: ") + ui->cmbBx_chs_brd->itemText(ui->cmbBx_chs_brd->count() - 1) + "\nPlease, contact with teacher or administrator"), 0);
                xml_file.close();
                return false;
            }
        } else if(!ui_initialized) {
            if((xmlReader.name().toString().compare("pic", Qt::CaseInsensitive) == 0) && (xmlReader.attributes().hasAttribute("name"))) {
                pixmp_names->append(xmlReader.attributes().value("name").toString());
            } else if(xmlReader.name().toString().compare("led", Qt::CaseInsensitive) == 0) {
                if(xmlReader.attributes().hasAttribute("x_on_pic")) {
                    add_data_to_qpoint(led_x_y, xmlReader.attributes().value("x_on_pic").toInt(), true);
                }
                if(xmlReader.attributes().hasAttribute("y_on_pic")) {
                    add_data_to_qpoint(led_x_y, xmlReader.attributes().value("y_on_pic").toInt(), false);
                }
                if(xmlReader.attributes().hasAttribute("width")) {
                    add_data_to_qpoint(led_width_height, xmlReader.attributes().value("width").toInt(), true);
                }
                if(xmlReader.attributes().hasAttribute("height")) {
                    add_data_to_qpoint(led_width_height, xmlReader.attributes().value("height").toInt(), false);
                }
                if(xmlReader.attributes().hasAttribute("color")) {
                    led_colors->append(xmlReader.attributes().value("color").toString());
                }
            }
        } else if((xmlReader.name().toString().compare("board", Qt::CaseInsensitive) == 0) && (!xmlReader.attributes().hasAttribute("name"))) {
            if(pixmp_names->count() < ui->cmbBx_chs_brd->count()) {
                while(pixmp_names->count() != ui->cmbBx_chs_brd->count()) {
                    pixmp_names->append("");
                }
            }
            if(led_x_y->count() < ui->cmbBx_chs_brd->count()) {
                while(led_x_y->count() != ui->cmbBx_chs_brd->count()) {
                    add_data_to_qpoint(led_x_y, -1, true);
                }
            }
            if(led_width_height->count() < ui->cmbBx_chs_brd->count()) {
                while(led_width_height->count() != ui->cmbBx_chs_brd->count()) {
                    add_data_to_qpoint(led_width_height, -1, true);
                }
            }
            if(led_colors->count() < ui->cmbBx_chs_brd->count()) {
                while(led_colors->count() != ui->cmbBx_chs_brd->count()) {
                    led_colors->append("");
                }
            }
        }
    }
    xml_file.close();
    return true;
}

void RHE_Widget::add_data_to_qpoint(QList<QPoint> *lst, int val, bool is_x) {
    if(lst->count() != ui->cmbBx_chs_brd->count()) {
        if(is_x) {
            lst->append(QPoint(val, -1));
        } else {
            lst->append(QPoint(-1, val));
        }
    } else {
        if(is_x) {
            lst->replace((lst->count() - 1), QPoint(val, lst->last().y()));
        } else {
            lst->replace((lst->count() - 1), QPoint(lst->last().x(), val));
        }
    }
}

void RHE_Widget::slot_input_val_changed(int val) {
    int pos = (val - (val % 2)) / 2;
    inpt_stts->at(pos)->display(val % 2);
    Send_Recieve_Module::set_state_Packet *switches_states = (Send_Recieve_Module::set_state_Packet*)malloc(sizeof(Send_Recieve_Module::set_state_Packet));
    memset(switches_states, 0, sizeof(Send_Recieve_Module::set_state_Packet));
    switches_states->pin_count = static_cast<uint8_t>(inpt_sldrs->count());
    for(int i = 0; i < inpt_sldrs->count(); i++) {
        Send_Recieve_Module::set_state switch_state;
        switch_state.pinNum = static_cast<uint8_t>(pi_pins_nums->at(i));
        switch_state.state = static_cast<uint8_t>(inpt_sldrs->at(i)->value() % 2);
        switches_states->pins[i] = switch_state;
    }
    snd_rcv_module->send_swtches_states(QByteArray::fromRawData(reinterpret_cast<const char*>(switches_states), sizeof(Send_Recieve_Module::set_state_Packet)));
    free(switches_states);
}

void RHE_Widget::slot_slider_pressed() {
    for(int i = 0; i < inpt_sldrs->count(); i++) {
        if(inpt_sldrs->at(i)->isSliderDown()) {
            if(inpt_sldrs->at(i)->value() == inpt_sldrs->at(i)->minimum()) {
                inpt_sldrs->at(i)->setValue(inpt_sldrs->at(i)->maximum());
            } else {
                inpt_sldrs->at(i)->setValue(inpt_sldrs->at(i)->minimum());
            }
        }
    }
}

void RHE_Widget::slot_choose_board(QString jtag_code) {
    for(int i = 0; i < jtag_id_codes->count(); i++) {
        if(jtag_id_codes->at(i).compare(jtag_code, Qt::CaseInsensitive) == 0) {
            emit ui->cmbBx_chs_brd->setCurrentIndex(i);
        }
    }
}

void RHE_Widget::slot_accept_board(bool flg) {
    if(flg) {
        prev_board_index = ui->cmbBx_chs_brd->currentIndex();
    } else {
        emit ui->cmbBx_chs_brd->setCurrentIndex(prev_board_index);
    }
}

void RHE_Widget::slot_accept_debug_time_limit(int time, int time_type) {
    ui->lcdNmbr_dbg_tm_lmt->display(time);
    dbg_tm_tp_lmt = time_type;
    ui->lbl_dbg_tm_tp_lmt->setText(ui->cmbBx_dbg_tm_tp->itemText(time_type));
}

void RHE_Widget::slot_accept_debug_data(QByteArray debug_data) {
    Send_Recieve_Module::debug_log_Packet *tmp_packet = reinterpret_cast<Send_Recieve_Module::debug_log_Packet *>(debug_data.data());
    if(new_debug) {
        new_debug = false;
        wvfrm_vwr->remove_all_data();
        prev_vals->clear();
        for(int i = 0; i < wvfrm_vwr->get_all_pins_count(); i++) {
            prev_vals->append(0);
            prev_vals->append(0);
        }
    }
    QList<int> val;
    val.clear();
    QList<QString> pin_names = *wvfrm_vwr->get_pin_names();
    QList<QPoint> nmd_pin_pos;
    nmd_pin_pos.clear();
    for(int i = 0; i < pin_names.count(); i++) {
        for(int k = 0; k < tmp_packet->pin_count; k++) {
            QString tmp_str(tmp_packet->pins[k].pinName);
            if((pin_names.at(i).compare(tmp_str, Qt::CaseInsensitive) == 0)) {
                nmd_pin_pos.append(QPoint(i, k));
                break;
            }
        }
    }
    bool val_changed = false;
    double dbg_time = (static_cast<double>(tmp_packet->time) / pow(1000.0, tmp_packet->time_mode));
    for(int i = 0; i < wvfrm_vwr->get_all_pins_count(); i++) {
        val.append(0);
        if(nmd_pin_pos.count() == 0) {
            if(i >= tmp_packet->pin_count) {
                val.append(prev_vals->at((2 * i) + 1));           //0
            } else {
                val.append(tmp_packet->pins[i].state);
            }
        } else {
            int tmp = 0;
            for(int k = 0; k < nmd_pin_pos.count(); k++) {
                if(i == nmd_pin_pos.at(k).x()) {
                    val.append(tmp_packet->pins[nmd_pin_pos.at(k).y()].state);
                    break;
                } else {
                    tmp++;
                }
            }
            if(tmp == nmd_pin_pos.count()) {
                val.append(prev_vals->at((2 * i) + 1));         //0
            }
        }
        if(prev_vals->at(val.count() - 1) != val.at(val.count() - 1)) {
            val_changed = true;
        }
    }
    wvfrm_vwr->add_data_to_saved_vals_list(val, prev_vals, dbg_time, val_changed);
    wvfrm_vwr->add_data_to_graph_rltm(val, prev_vals, dbg_time, val_changed);
}

void RHE_Widget::slot_accept_input_data_table(QByteArray input_data_table) {
    int pin_count = 0;
    memcpy(&pin_count, input_data_table.data(), sizeof(uint8_t));
    int hop = 5; // bytes
    wvfrm_vwr->remove_pin_names();
    for(int i = 0; i < pin_count; i++) {
        wvfrm_vwr->add_pin_names(QByteArray((input_data_table.data() + ((hop * i) + 1)), 5));
    }
    change_cnt_of_dbg_pins(pin_count);
    wvfrm_vwr->remove_all_data();
    wvfrm_vwr->remove_saved_vals_list();
    wvfrm_vwr->add_saved_vals_list(pin_count);
}

void RHE_Widget::slot_accept_output_data_table(QByteArray output_data_table) {
    int pin_count = 0;
    int hop = 5; // bytes
    pi_pins_nums->clear();
    memcpy(&pin_count, output_data_table.data(), sizeof(uint8_t));
    while(ui->verticalLayout_4->itemAt(0) != 0) {
        while(ui->verticalLayout_4->itemAt(0)->layout()->takeAt(0) != 0) {
            ui->verticalLayout_4->itemAt(0)->layout()->removeItem(ui->verticalLayout_4->itemAt(0)->layout()->takeAt(0));
        }
        ui->verticalLayout_4->removeItem(ui->verticalLayout_4->itemAt(0));
    }
    while(inpt_stts->count() != 0) {
        delete inpt_stts->at(0);
        delete inpt_spcrs->first();
        delete inpt_spcrs->last();
        delete inpt_hbxs->at(0);
        delete inpt_lbls->at(0);
        delete inpt_sldrs->at(0);
        inpt_hbxs->removeAt(0);
        inpt_spcrs->removeFirst();
        inpt_spcrs->removeLast();
        inpt_lbls->removeAt(0);
        inpt_sldrs->removeAt(0);
        inpt_stts->removeAt(0);
    }
    ui->verticalLayout_4->update();
    wvfrm_vwr->add_saved_vals_list(pin_count);
    for(int i = 0; i < pin_count; i++) {
        int val;
        memcpy(&val, (output_data_table.data() + ((hop * i) + 6 + i)), 1);
        pi_pins_nums->append(val);
        QString pin_name_str = QByteArray((output_data_table.data() + ((hop * i) + 1 + i)), 5);
        QHBoxLayout *h_layout = new QHBoxLayout();
        inpt_hbxs->append(h_layout);
        ui->verticalLayout_4->addLayout(h_layout);
        QLabel *pin_name = new QLabel();
        pin_name->setText(pin_name_str);
        wvfrm_vwr->add_pin_names(pin_name_str);
        pin_name->setFixedSize(40, 21);
        pin_name->setFont(ui->lbl_chs_brd->font());
        inpt_lbls->append(pin_name);
        h_layout->addWidget(pin_name);
        QSpacerItem *hrzntl_1 = new QSpacerItem(20, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);
        inpt_spcrs->append(hrzntl_1);
        h_layout->addSpacerItem(hrzntl_1);
        QSlider *sldr = new QSlider();
        sldr->setOrientation(Qt::Horizontal);
        sldr->setMinimum(i * 2);
        sldr->setMaximum(i * 2 + 1);
        sldr->setSingleStep(1);
        sldr->setBaseSize(100, 21);    //setFixedSize
        sldr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(sldr, &QSlider::valueChanged, this, &RHE_Widget::slot_input_val_changed);
        connect(sldr, &QSlider::sliderPressed, this, &RHE_Widget::slot_slider_pressed);
        inpt_sldrs->append(sldr);
        h_layout->addWidget(sldr);
        QSpacerItem *hrzntl_2 = new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Minimum);
        inpt_spcrs->append(hrzntl_2);
        h_layout->addSpacerItem(hrzntl_2);
        QLCDNumber *lcd_val = new QLCDNumber();
        lcd_val->setDecMode();
        lcd_val->setDigitCount(1);
        lcd_val->setFont(ui->lbl_chs_brd->font());
        lcd_val->setFixedSize(20, 21);
        lcd_val->display(0);
        lcd_val->setPalette(ui->lcdNmbr_dbg_tm_lmt->palette());
        inpt_stts->append(lcd_val);
        h_layout->addWidget(lcd_val);
    }
    change_cnt_of_dbg_pins(wvfrm_vwr->graph_count + pin_count);
    emit resize_signal();
}

void RHE_Widget::slot_firmware_file_sended() {
    send_file_status->stop();
    crrnt_state_strs = 1;
    ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    ui->prgrssBr_fl_sts->setValue(ui->prgrssBr_fl_sts->minimum());
    if(!static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState())) || !sqnc_of_sgnls_file_sended) {
        send_file_status->setInterval(200);
        send_file_status->start();
        crrnt_state_strs = 2;
        ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
        snd_rcv_module->flash_FPGA();
    } else if(static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState())) && sqnc_of_sgnls_file_sended) {
        on_pshBttn_strt_dbg_clicked();
    }
}

void RHE_Widget::slot_sequence_of_signals_file_sended(bool flg) {
    sqnc_of_sgnls_file_sended = flg;
    ui->chckBx_strt_sqnc_of_sgn_with_dbg->setEnabled(flg);
    if(!static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState()))) {
        pshBttn_strt_sgnls_sqnc_set_enabled(flg);
    } else {
        on_pshBttn_strt_sgnls_sqnc_clicked();
    }
}

void RHE_Widget::slot_fpga_flashed() {
    send_file_status->stop();
    crrnt_state_strs = 3;
    ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    ui->prgrssBr_fl_sts->setValue(ui->prgrssBr_fl_sts->minimum());
    set_enable_board_power_led(true);
    if(static_cast<bool>(static_cast<int>(ui->chckBx_strt_dbg_aftr_flsh->checkState()))) {
        if(!static_cast<bool>(static_cast<int>(ui->chckBx_strt_sqnc_of_sgn_with_dbg->checkState()))) {
            on_pshBttn_strt_dbg_clicked();
        }
        crrnt_state_strs = 4;
        ui->prgrssBr_fl_sts->setFormat(state_strs.at(crrnt_state_strs));
    }
}

void RHE_Widget::slot_end_sequence_of_signals() {
    sqnc_of_sgnls_strtd = false;
    scrll_area_sgnls_set_enabled(true);
}

void RHE_Widget::slot_as_window() {
    if(!wvfrm_vwr->as_window) {
        ui->verticalLayout_3->addWidget(wvfrm_vwr);
    } else {
        ui->verticalLayout_3->removeWidget(wvfrm_vwr);
    }
    emit resize_signal();
}

void RHE_Widget::slot_timer_timeout() {
    send_file_status->stop();
    int val = ui->prgrssBr_fl_sts->value() + 5;
    if(val > ui->prgrssBr_fl_sts->maximum()) {
        val = ui->prgrssBr_fl_sts->minimum();
    }
    ui->prgrssBr_fl_sts->setValue(val);
    send_file_status->setInterval(200);
    send_file_status->start();
}

void RHE_Widget::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}
