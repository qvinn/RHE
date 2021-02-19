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
    pixmp_names = new QList<QString>();
    jtag_id_codes = new QList<QString>();
    led_colors = new QList<QString>();
    led_x_y = new QList<QPoint>();
    led_width_height = new QList<QPoint>();
    wvfrm_vwr = new Waveform_Viewer_Widget(this, gen_widg, false);
    connect(wvfrm_vwr, &Waveform_Viewer_Widget::built_in_signal, this, &RHE_Widget::slot_built_in);
    connect(snd_rcv_module, &Send_Recieve_Module::choose_board_signal, this, &RHE_Widget::slot_choose_board);
    connect(snd_rcv_module, &Send_Recieve_Module::accept_board_signal, this, &RHE_Widget::slot_accept_board);
    prev_vals = new QList<int>();
    initialize_ui();
    tmr = new QTimer(this);
    tmr->setInterval(100);
    connect(tmr, SIGNAL(timeout()), this, SLOT(slot_Timer()));
    on_pushButton_stp_drw_clicked();
}

RHE_Widget::~RHE_Widget() {
    delete pixmp_names;
    delete led_x_y;
    delete led_width_height;
    delete led_colors;
    delete jtag_id_codes;
    delete path_to_proj;
    delete prev_path_to_proj;
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
    ui->widget->resize(this->width(), (this->height() - ui->horizontalLayoutWidget->height()));
    ui->horizontalLayoutWidget_2->setGeometry(0, 0, ui->widget->width(), ui->widget->height());
    ui->verticalLayout_3->setGeometry(QRect(ui->cmbBx_chs_brd->width(), 0, (ui->widget->width() - ui->cmbBx_chs_brd->width()), ui->widget->height()));
    if(!pixmp_brd.isNull()) {
        ui->label->setPixmap(pixmp_brd.scaled(ui->label->size(), Qt::KeepAspectRatio));
    }
}

void RHE_Widget::on_pushButton_clicked() {
    if((led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).x() != -1) || (led_x_y->at(ui->cmbBx_chs_brd->currentIndex()).y() != -1) || (led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).x() != -1) || (led_width_height->at(ui->cmbBx_chs_brd->currentIndex()).y() != -1) || (led_colors->at(ui->cmbBx_chs_brd->currentIndex()).count() == 9)) {
        board_is_on = !board_is_on;
        clr_trnsprnt = false;
        showEvent(nullptr);
        if(clr_trnsprnt) {
            on_cmbBx_chs_brd_currentIndexChanged(ui->cmbBx_chs_brd->currentIndex());
        }
    }
}

void RHE_Widget::on_pushButton_2_clicked() {
    snd_rcv_module->ping_to_server();
}

void RHE_Widget::on_pushButton_3_clicked() {
    snd_rcv_module->ping_to_S_server();
}

void RHE_Widget::on_pushButton_strt_drw_clicked() {
    wvfrm_vwr->pshBttn_open_save_wvfrm_set_enabled(false);
    tmr->setInterval(100);
    tmr->start();
}


void RHE_Widget::on_pushButton_stp_drw_clicked() {
    tmr->stop();
    wvfrm_vwr->pshBttn_open_save_wvfrm_set_enabled(true);
}

void RHE_Widget::on_cmbBx_chs_brd_currentIndexChanged(int index) {
    if(ui_initialized && (index != -1)) {
        gen_widg->save_setting("settings/CURRENT_BOARD", index);
        snd_rcv_module->set_FPGA_id(jtag_id_codes->at(index));
        if(pixmp_names->at(index).count() == 0) {
            ui->label->clear();
            QPixmap tmp;
            pixmp_brd.swap(tmp);
        } else {
            if(pixmp_brd.load(qApp->applicationDirPath() + "/" + gen_widg->get_setting("settings/PATH_TO_DATA").toString() + pixmp_names->at(index))){
                showEvent(nullptr);
                ui->label->setPixmap(pixmp_brd.scaled(ui->label->size(), Qt::KeepAspectRatio));
            }
        }
    }
}

void RHE_Widget::on_hrzntlSldr_cnt_dbg_pins_valueChanged(int value) {
    if(ui_initialized) {
        ui->lcdNmbr_cnt_dbg_pins->display(value);
        gen_widg->save_setting("settings/DEBUG_PINS_NUMBER", value);
        wvfrm_vwr->plot_re_scale = true;
        cnt = 0;
        prev_debug_time = 0;
        prev_vals->clear();
        for(int i = 0; i < value; i++) {
            prev_vals->append(0);
        }
        wvfrm_vwr->remove_graphs_form_plot();
        wvfrm_vwr->graph_count = value;
        wvfrm_vwr->re_scale_graph();
        wvfrm_vwr->add_graphs_to_plot();
        wvfrm_vwr->plot_re_scale = false;
        wvfrm_vwr->re_scale_graph();
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
        snd_rcv_module->send_file_to_ss(svf_file->readAll());
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

void RHE_Widget::initialize_ui() {
    ui->hrzntlSldr_cnt_dbg_pins->setValue(gen_widg->get_setting("settings/DEBUG_PINS_NUMBER").toInt());
    ui->lcdNmbr_cnt_dbg_pins->display(ui->hrzntlSldr_cnt_dbg_pins->value());
    wvfrm_vwr->graph_count = ui->hrzntlSldr_cnt_dbg_pins->value();
    wvfrm_vwr->initialize_ui();
    for(int i = 0; i < ui->hrzntlSldr_cnt_dbg_pins->value(); i++) {
        prev_vals->append(0);
    }
    ui->verticalLayout_3->addWidget(wvfrm_vwr);
    path_to_proj->clear();
    QDir::setCurrent(qApp->applicationDirPath());
    pshBttn_snd_frmwr_set_enabled(false);
    pshBttn_chk_prj_stat_set_enabled(false);
    if(gen_widg->get_setting("settings/MANUALY_LOAD_FIRMWARE").toBool() && gen_widg->get_setting("settings/ENABLE_FILE_CHEKING").toBool()) {
        pshBttn_ld_frmwr_set_enabled(false);
    }
    if(ui->cmbBx_chs_brd->count() == 0) {
        if(read_xml_file(false)) {
            ui_initialized = true;
            prev_board_index = gen_widg->get_setting("settings/CURRENT_BOARD").toInt();
            ui->cmbBx_chs_brd->setCurrentIndex(prev_board_index);
        } else {
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        }
    } else {
        ui_initialized = true;
        prev_board_index = gen_widg->get_setting("settings/CURRENT_BOARD").toInt();
        ui->cmbBx_chs_brd->setCurrentIndex(prev_board_index);
    }
}

void RHE_Widget::set_ui_text() {
    ui->lbl_FName_LName->setText(tr("Hello, ") + lname_fname);
}

void RHE_Widget::set_fname_lname(QString str) {
    lname_fname.clear();
    lname_fname.append(str);
    set_ui_text();
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
    int cnt = 0;
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
                cnt++;
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

void RHE_Widget::slot_Timer() {
    on_pushButton_stp_drw_clicked();
    cnt++;
    debug_time = (static_cast<double>(cnt) / 100.0);
    QList<int> val;
    val.clear();
    for(int i = 0; i < ui->hrzntlSldr_cnt_dbg_pins->value(); i++) {
        val.append(rand() % 2);
    }
    wvfrm_vwr->add_data_to_graph_rltm(val, prev_vals, debug_time, prev_debug_time, true);
    prev_debug_time = debug_time;
    on_pushButton_strt_drw_clicked();
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

void RHE_Widget::slot_built_in() {
    ui->verticalLayout_3->addWidget(wvfrm_vwr);
}

void RHE_Widget::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}
