#include "waveform_viewer_widget.h"
#include "ui_waveform_viewer.h"
#include "ui_dialog_select_displayable_pins.h"

Waveform_Viewer_Widget::Waveform_Viewer_Widget(QWidget *parent, General_Widget *widg, bool stndln) : QWidget(parent), ui(new Ui::Waveform_Viewer) {
    ui->setupUi(this);
    gen_widg = widg;
    standalone = stndln;
    ui->chckBx_as_wndw->setVisible(!stndln);
    textTicker = new QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());
    dyn_tckr = new QSharedPointer<QCPAxisTickerFixed>(new QCPAxisTickerFixed());
//    ui->diagram->setOpenGl(true);         //qcustomplot.cpp - line 909
    curs_ver_line = new QCPItemLine(ui->diagram);
    curs_time = new QCPItemText(ui->diagram);
    connect(ui->diagram->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_x_axis_changed(QCPRange)));
    connect(ui->diagram->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_y_axis_changed(QCPRange)));
    connect(ui->diagram, &QCustomPlot::mouseMove, this, &Waveform_Viewer_Widget::slot_mouse_move);
    connect(ui->diagram, &QCustomPlot::mousePress, this, &Waveform_Viewer_Widget::slot_mouse_pressed);
    connect(ui->diagram, &QCustomPlot::mouseRelease, this, &Waveform_Viewer_Widget::slot_mouse_unpressed);
    graph_list = new QList<QCPGraph *>();
    pin_names = new QList<QString>();
    pin_names_board = new QList<QString>();
    svd_vals = new QList<QList<int>*>();
    svd_dbg_time = new QList<double>();
    flags = this->windowFlags();
}

Waveform_Viewer_Widget::~Waveform_Viewer_Widget() {
    remove_graphs_from_plot();
    remove_saved_vals_list();
    delete svd_dbg_time;
    delete svd_vals;
    delete graph_list;
    delete textTicker;
    delete pin_names;
    delete pin_names_board;
    delete dyn_tckr;
    delete ui;
}

void Waveform_Viewer_Widget::showEvent(QShowEvent *) {
    resizeEvent(nullptr);
}

void Waveform_Viewer_Widget::leaveEvent(QEvent *) {
    ui->diagram->layer("layerCursor")->setVisible(false);
    ui->diagram->replot();
    qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
}

void Waveform_Viewer_Widget::resizeEvent(QResizeEvent *) {
    ui->verticalLayoutWidget->resize(this->width(), this->height());
    ui->diagram->resize(ui->verticalLayoutWidget->width(), (ui->verticalLayoutWidget->height() - ui->horizontalLayout->geometry().height() - ui->horizontalLayout_2->geometry().height()));
}

void Waveform_Viewer_Widget::closeEvent(QCloseEvent *) {
    if(standalone) {
        emit waveform_viewer_closed_signal();
    }
}

void Waveform_Viewer_Widget::on_chckBx_attch_crsr_stateChanged(int state) {
    if(ui_initialized) {
        if(standalone) {
            gen_widg->save_setting("settings/WVFRM_VWR_ATTCH_CRSR_STANDALONE", state);
        } else {
            gen_widg->save_setting("settings/WVFRM_VWR_ATTCH_CRSR", state);
        }
    }
}

void Waveform_Viewer_Widget::on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(int value) {
    if(ui_initialized) {
        if(standalone) {
            gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_STANDALONE", value);
        } else {
            gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME", value);
        }
        time_coef = pow(1000.0, static_cast<double>(ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->currentIndex()));
        x_tckr_step = static_cast<double>(value) / time_coef;
        (*dyn_tckr)->setTickStep(x_tckr_step);
        (*dyn_tckr)->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
        ui->diagram->xAxis->setTicker(*dyn_tckr);
        ui->diagram->replot();
    }
}

void Waveform_Viewer_Widget::on_cmbBx_wvfrm_vwr_dscrtnss_tm_tp_currentIndexChanged(int index) {
    if(ui_initialized && language_changed) {
        if(standalone) {
            gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE_STANDALONE", index);
        } else {
            gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE", index);
        }
        on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(ui->spnBx_wvfrm_vwr_dscrtnss_tm->value());
    }
}

void Waveform_Viewer_Widget::on_pshBttn_fl_scl_clicked() {
    re_scale_graph();
}

void Waveform_Viewer_Widget::on_pshBttn_clr_clicked() {
    remove_all_data();
}

void Waveform_Viewer_Widget::on_pshBttn_slct_dsplbl_pins_clicked() {
    select_displayable_pins();
}

void Waveform_Viewer_Widget::on_chckBx_as_wndw_stateChanged(int state) {
    if(state == 2) {
        this->setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);
        this->show();
        emit as_window_signal(true);
    } else {
        this->setWindowFlags(flags);
        emit as_window_signal(false);
    }
}

void Waveform_Viewer_Widget::on_pshBttn_open_save_wvfrm_clicked() {
    if(standalone) {
        load_waveform();
    } else {
        save_waveform();
    }
}

void Waveform_Viewer_Widget::pshBttn_slct_dsplbl_pins_set_enabled(bool flg) {
    ui->pshBttn_slct_dsplbl_pins->setEnabled(flg);
}

void Waveform_Viewer_Widget::pshBttn_open_save_wvfrm_set_enabled(bool flg) {
    ui->pshBttn_open_save_wvfrm->setEnabled(flg);
}

void Waveform_Viewer_Widget::initialize_ui() {
    ui->diagram->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom/* | QCP::iSelectPlottables*//* | QCP::iSelectAxes*/);     //iSelectAxes
    ui->diagram->xAxis->setBasePen(QPen(QColor(100, 100, 100), 1));
    ui->diagram->yAxis->setBasePen(QPen(QColor(100, 100, 100), 1));
    ui->diagram->xAxis->setTickPen(QPen(QColor(100, 100, 100), 1));
    ui->diagram->yAxis->setTickPen(QPen(QColor(100, 100, 100), 1));
    ui->diagram->xAxis->grid()->setPen(QPen(QColor(100, 100, 100), 1, Qt::SolidLine));
    ui->diagram->yAxis->grid()->setPen(QPen(QColor(100, 100, 100), 1, Qt::SolidLine));
    ui->diagram->xAxis->setTickLabelColor(Qt::white);
    ui->diagram->yAxis->setTickLabelColor(Qt::white);
    QLinearGradient plot_gradient;
    plot_gradient.setStart(0, 0);
    plot_gradient.setFinalStop(0, 350);
    plot_gradient.setColorAt(0, QColor(0, 0, 0));
    plot_gradient.setColorAt(1, QColor(0, 0, 0));
    ui->diagram->setBackground(plot_gradient);
    QLinearGradient axis_rect_gradient;
    axis_rect_gradient.setStart(0, 0);
    axis_rect_gradient.setFinalStop(0, 350);
    axis_rect_gradient.setColorAt(0, QColor(0, 0, 0));
    axis_rect_gradient.setColorAt(1, QColor(0, 0, 0));
    ui->diagram->axisRect()->setBackground(axis_rect_gradient);
    ui->diagram->xAxis->setSubTicks(false);
    ui->diagram->yAxis->setSubTicks(false);
    //////////////////////////////////////////////////////////////////////////////////
//    ui->diagram->xAxis->setSelectableParts(QCPAxis::spNone);
//    ui->diagram->yAxis->setSelectableParts(QCPAxis::spTickLabels);
//    ui->diagram->yAxis->setSelectedTickLabelColor(QColor("#FF00FF50"));
    //////////////////////////////////////////////////////////////////////////////////
    ui->diagram->selectionRect()->setPen(QPen(QColor("#FFFF0000"), 1, Qt::SolidLine));
    ui->diagram->selectionRect()->setBrush(QBrush(QColor("#50FF0000")));
    ui->diagram->addLayer("layerCursor", 0, QCustomPlot::limAbove);
    ui->diagram->layer("layerCursor")->setMode(QCPLayer::lmBuffered);
    curs_ver_line->setPen(QPen(QColor("#FFFFFF00"), 1, Qt::SolidLine));
    curs_time->setPen(QPen(QColor("#FFFFFFFF"), 1, Qt::SolidLine));
    curs_time->setBrush(QBrush(QColor("#FFFFFF00")));
    curs_time->setPadding(QMargins(2, 2, 2, 2));
    curs_ver_line->setLayer("layerCursor");
    curs_time->setLayer("layerCursor");
    ui->diagram->layer("layerCursor")->setVisible(false);
    post_initialize_ui();
}

void Waveform_Viewer_Widget::post_initialize_ui() {
    set_ui_text();
    if(standalone) {
        ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(abs(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR_STANDALONE").toInt() - 2)));
        ui->spnBx_wvfrm_vwr_dscrtnss_tm->setValue(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_STANDALONE").toInt());
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(abs(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE_STANDALONE").toInt() - 1));
    } else {
        ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(abs(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR").toInt() - 2)));
        ui->spnBx_wvfrm_vwr_dscrtnss_tm->setValue(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME").toInt());
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(abs(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE").toInt() - 1));
    }
    ui_initialized = true;
    if(standalone) {
        ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR_STANDALONE").toInt()));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE_STANDALONE").toInt());
    } else {
        ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR").toInt()));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE").toInt());
    }
}

void Waveform_Viewer_Widget::set_ui_text() {
    language_changed = false;
    this->setWindowTitle(tr("Waveform Viewer"));
    if(standalone) {
        ui->pshBttn_open_save_wvfrm->setText(tr("Open waveform"));
    } else {
        ui->pshBttn_open_save_wvfrm->setText(tr("Save waveform"));
    }
    if(ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->count() == 0) {
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->addItem(tr("s"));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->addItem(tr("ms"));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->addItem(tr("us"));
    } else {
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setItemText(0, tr("s"));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setItemText(1, tr("ms"));
        ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setItemText(2, tr("us"));
    }
    language_changed = true;
}

void Waveform_Viewer_Widget::load_waveform() {
    QStringList *lst = gen_widg->load_files(false, false, tr("Choose waveform file"), tr("Waveform (*.wvfrm)"));
    if((lst == nullptr) || (lst->count() == 0)) {
        gen_widg->show_message_box(tr("Error"), tr("Waveform file not choosed"), 0);
        return;
    }
    QFile *file = new QFile(QFileInfo(lst->at(0)).absoluteFilePath());
    if(file->open(QFile::ReadOnly)) {
        QString str = file->readLine();
        file->close();
        str.replace("\n", "");
        QRegExp tag_exp("/");
        QStringList lst = str.split(tag_exp);
        add_names = true;
        plot_re_scale = true;
        remove_data_from_saved_vals_list();
        remove_saved_vals_list();
        remove_graphs_from_plot();
        graph_count = lst.count() - 1;
        add_saved_vals_list(graph_count);
        double debug_time = 0.0;
        QList<int> vals;
        vals.clear();
        QList<int> prev_vals;
        prev_vals.clear();
        for(int i = 0; i < graph_count; i++) {
            prev_vals.append(0);
            prev_vals.append(0);
        }
        re_scale_graph();
        add_graphs_to_plot();
        plot_re_scale = false;
        re_scale_graph();
        if(file->open(QIODevice::ReadOnly)) {
            QTextStream in(file);
            remove_pin_names();
            bool rename_pins = true;
            while(!in.atEnd()) {
                QString str_dat = in.readLine();
                str_dat.replace("\n", "");
                QRegExp tagExp("/");
                QStringList lst_dat = str_dat.split(tagExp);
                for(int i = 0; i < lst_dat.count(); i++) {
                    if(str_dat.contains("time")) {
                        if(i != 0) {
                            add_pin_names(lst_dat.at(i));
                        }
                    } else {
                        if(i == 0) {
                            debug_time = lst_dat.at(i).toDouble();
                        } else {
                            vals.append(0);
                            vals.append(lst_dat.at(i).toInt());
                        }
                    }
                }
                if(rename_pins || str_dat.contains("time")) {
                    rename_pins = false;
                    change_pin_names();
                }
                if(!str_dat.contains("time")) {
                    add_data_to_saved_vals_list(vals, &prev_vals, debug_time, false);
                    add_data_to_graph(vals, &prev_vals, debug_time, false);
                    vals.clear();
                }
            }
            file->close();
        }
    }
    delete file;
    re_scale_graph();
}

void Waveform_Viewer_Widget::save_waveform() {
    if(graph_count != 0) {
        int cnt = ui->diagram->graph(0)->data()->size();
        QString fn_nm = "";
        QString str = "";
        for(int i = 0; i < graph_count; i++) {
            if(i == 0) {
                str.append("time/");
            }
            str.append(pin_names->at(i));
            if(i != (graph_count - 1)) {
                str.append("/");
            } else {
                str.append("\n");
            }
        }
        gen_widg->save_file(this, tr("Saving waveform"), tr("Waveform (*.wvfrm)"), &str, &fn_nm, false, false);
        str.clear();
        for(int i = 0; i < cnt; i++) {
            str.clear();
            for(int k = 0; k < graph_list->count(); k++) {
                if(k % 2 != 0) {
                    if(k == 1) {
                        str.append(QString::number(ui->diagram->graph(k)->data()->at(i)->key) + "/");
                    }
                    str.append(QString::number(ui->diagram->graph(k)->data()->at(i)->value - k));
                    if(k != (graph_list->count() - 1)) {
                        str.append("/");
                    } else {
                        str.append("\n");
                    }
                }
            }
            gen_widg->save_file(this, tr("Saving waveform"), tr("Waveform (*.wvfrm)"), &str, &fn_nm, false, true);
        }
    }
}

void Waveform_Viewer_Widget::add_graphs_to_plot() {
    for(int i = 0; i < graph_count; i++) {
        graph_list->append(ui->diagram->addGraph());
        graph_list->at(i * 2 + 0)->setPen(QPen(QColor(0, 255, 0, 0), 1));
        graph_list->at(i * 2 + 0)->setBrush(QBrush(QColor(0, 0, 255, 0)));
        graph_list->append(ui->diagram->addGraph());
        graph_list->at(i * 2 + 1)->setPen(QPen(QColor(0, 255, 0, 255), 1));
    }
    change_pin_names();
}

QList<QCPGraph *>* Waveform_Viewer_Widget::get_graphs_list() {
    return graph_list;
}

void Waveform_Viewer_Widget::remove_graphs_from_plot() {
    graph_list->clear();
    graph_count = 0;
    while(ui->diagram->graphCount() != 0) {
        ui->diagram->graph(ui->diagram->graphCount() - 1)->data().clear();
        ui->diagram->removeGraph(ui->diagram->graphCount() - 1);
    }
}

void Waveform_Viewer_Widget::re_scale_graph() {
    ui->diagram->xAxis->rescale();
    ui->diagram->yAxis->rescale();
    ui->diagram->yAxis->setRange(0, (graph_count * 2));
    ui->diagram->xAxis->setRange(0, ui->diagram->xAxis->range().maxRange);
    ui->diagram->replot();
    ui->diagram->setProperty("xmin", ui->diagram->xAxis->range().minRange);
    ui->diagram->setProperty("xmax", ui->diagram->xAxis->range().maxRange);
    ui->diagram->setProperty("ymin", ui->diagram->yAxis->range().lower);
    ui->diagram->setProperty("ymax", ui->diagram->yAxis->range().upper);
}

void Waveform_Viewer_Widget::add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, QList<bool> *drw_lst) {
    for(int i = 0; i < /*graph_list->count()*/pin_names_board->count() * 2; i++) {
//        if(i < graph_list->count()) {
//            if(drw_lst == nullptr) {
//                draw_data_on_graph(val, prev_vals, time, val_changed, i);
//            } else if(drw_lst->at(i)) {
//                draw_data_on_graph(val, prev_vals, time, val_changed, i);
//            }
//        }
        if(i < graph_list->count()) {
            draw_data_on_graph(val, prev_vals, time, val_changed, i);
        }
        if(i < prev_vals->count()) {
            (*prev_vals).replace(i, val.at(i));
        }
    }
}

void Waveform_Viewer_Widget::add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, QList<bool> *drw_lst) {
    add_data_to_graph(val, prev_vals, time, val_changed, drw_lst);
    ui->diagram->replot();
}

void Waveform_Viewer_Widget::draw_data_on_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, int i) {
    int shft_val = abs((i % 2) - 1);
    if(val_changed) {
        graph_list->at(i)->addData(time, (shft_val + i + prev_vals->at(i)));
    }
    if((val.at(i) == 1) && (i != 0)) {
        graph_list->at(i)->setBrush(QBrush(QColor(0, 255, 0, 60)));
        graph_list->at(i)->setChannelFillGraph(graph_list->at(i - 1));
    }
    graph_list->at(i)->addData(time, (shft_val + i + val.at(i)));
//    (*prev_vals).replace(i, val.at(i));
}

void Waveform_Viewer_Widget::remove_data_from_graph() {
    for(int i = 0; i < graph_list->count(); i++) {
        graph_list->at(i)->data().data()->clear();
        graph_list->at(i)->data().clear();
    }
    re_scale_graph();
}

void Waveform_Viewer_Widget::add_pin_names(QString pin_name) {
    pin_names->append(pin_name);
    pin_names_board->append(pin_name);
}

QList<QString>* Waveform_Viewer_Widget::get_pin_names() {
    return pin_names_board;
}

int Waveform_Viewer_Widget::get_all_pins_count() {
    return pin_names_board->count();
}

void Waveform_Viewer_Widget::remove_pin_names() {
    pin_names->clear();
    pin_names_board->clear();
}

void Waveform_Viewer_Widget::change_pin_names() {
    (*textTicker)->clear();
    ui->diagram->yAxis->ticker().clear();
    QList<QString> tmp_lst = *pin_names;
    pin_names->clear();
    for(int i = 0; i < graph_count; i++) {
        QString tmp = "pin ";
        if((i + 1) < 10) {
            tmp.append(QString::number(0));
        }
        if((tmp_lst.count() == 0) || (i >= tmp_lst.count())) {
            if((pin_names_board->count() == 0) || (i >= pin_names_board->count()) || !add_names) {
                pin_names->append(tmp.append(QString::number(i + 1)));
            } else {
                pin_names->append(pin_names_board->at(i));
            }
        } else {
            pin_names->append(tmp_lst.at(i));
        }
        (*textTicker)->addTick((i * 2 + 1), pin_names->at(i));
    }
    ui->diagram->yAxis->setTicker(*textTicker);
}

void Waveform_Viewer_Widget::add_saved_vals_list(int cnt) {
    for(int i = 0; i < cnt; i++) {
        QList<int> *val = new QList<int>();
        val->clear();
        svd_vals->append(val);
    }
}

void Waveform_Viewer_Widget::remove_saved_vals_list() {
    while(svd_vals->count() != 0) {
        delete svd_vals->at(0);
        svd_vals->removeAt(0);
    }
}

void Waveform_Viewer_Widget::add_data_to_saved_vals_list(QList<int> val, QList<int> *prev_vals, double time, bool val_changed) {
    svd_dbg_time->append(time);
    if(val_changed) {
        svd_dbg_time->append(time);
    }
    for(int i = 0; i < svd_vals->count(); i++) {
        if(val_changed) {
            svd_vals->at(i)->append(prev_vals->at((i * 2) + 1));
        }
        svd_vals->at(i)->append(val.at((i * 2) + 1));
    }
}

void Waveform_Viewer_Widget::remove_data_from_saved_vals_list() {
    for(int i = 0; i < svd_vals->count(); i++) {
        svd_vals->at(i)->clear();
    }
    svd_dbg_time->clear();
}

void Waveform_Viewer_Widget::remove_all_data() {
    remove_data_from_graph();
    remove_data_from_saved_vals_list();
    svd_dbg_time->clear();
    add_names = true;
    if(standalone) {
        plot_re_scale = true;
        remove_saved_vals_list();
        remove_pin_names();
        remove_graphs_from_plot();
        graph_count = 16;
        re_scale_graph();
        add_graphs_to_plot();
        *pin_names_board = *pin_names;
        plot_re_scale = false;
    }
    re_scale_graph();
}

void Waveform_Viewer_Widget::select_displayable_pins() {
    QStringList avlbl_pins, dsplbl_pins;
    avlbl_pins.append(*pin_names_board);
    if(add_names) {
        bool flag = true;
        while(flag) {
            for(int k = 0; k < pin_names->count(); k++) {
                int cnt = 0;
                bool skp = false;
                for(int i = 0; i < avlbl_pins.count(); i++) {
                    if(pin_names->at(k).compare(avlbl_pins.at(i), Qt::CaseInsensitive) == 0) {
                        avlbl_pins.removeAt(i);
                        skp = true;
                        break;
                    } else {
                        cnt++;
                    }
                }
                if(skp) {
                    break;
                } else if(((pin_names->count() + avlbl_pins.count()) == pin_names_board->count()) || (avlbl_pins.count() == 0)) {
                    flag = false;
                    break;
                }
            }
        }
        dsplbl_pins.append(*pin_names);
    }
    Dialog_Select_Displayable_Pins pins_select_dialog(avlbl_pins, dsplbl_pins, nullptr);
    pins_select_dialog.setWindowFlag(Qt::WindowStaysOnTopHint);
    int dlgCase = pins_select_dialog.exec();
    if(dlgCase == 1) {
        QStringList dsplbl_pins_lst = pins_select_dialog.get_displayable_pins();
        if((dsplbl_pins_lst.count() == 0) && (svd_vals->count() == 0)) {
            return;
        }
        QList<QPoint> graph_pos;
        graph_pos.clear();
        for(int i = 0; i < dsplbl_pins_lst.count(); i++) {
            for(int k = 0; k < pin_names_board->count(); k++) {
                if(dsplbl_pins_lst.at(i).compare(pin_names_board->at(k), Qt::CaseInsensitive) == 0) {
                    graph_pos.append(QPoint(i, k));
                }
            }
        }
        QStringList avlbl_pins_lst = pins_select_dialog.get_available_pins();
        for(int i = 0; i < avlbl_pins_lst.count(); i++) {
            for(int k = 0; k < pin_names_board->count(); k++) {
                if(avlbl_pins_lst.at(i).compare(pin_names_board->at(k), Qt::CaseInsensitive) == 0) {
                    graph_pos.append(QPoint(i + dsplbl_pins_lst.count(), k));
                }
            }
        }
        QList<QList<int> *> *new_svd_vals = new QList<QList<int> *>();
        QList<QString> *new_pin_names_board = new QList<QString>();
        new_svd_vals->clear();
        new_pin_names_board->clear();
        for(int i = 0; i < graph_pos.count(); i++) {
            if(svd_vals->count() != 0) {
                new_svd_vals->append(svd_vals->at(graph_pos.at(i).y()));
            }
            new_pin_names_board->append(pin_names_board->at(graph_pos.at(i).y()));
        }
        *svd_vals = *new_svd_vals;
        *pin_names_board = *new_pin_names_board;
        new_svd_vals->clear();
        new_pin_names_board->clear();
        delete new_svd_vals;
        delete new_pin_names_board;
        remove_data_from_graph();
        plot_re_scale = true;
        remove_graphs_from_plot();
        if(dsplbl_pins_lst.count() == 0) {
            graph_count = pin_names_board->count();
        } else {
            graph_count = dsplbl_pins_lst.count();
        }
        re_scale_graph();
        *pin_names = dsplbl_pins_lst;
        add_names = static_cast<bool>(pin_names->count());
        add_graphs_to_plot();
        plot_re_scale = false;
        QList<int> prev_vals;
        prev_vals.clear();
        for(int i = 0; i < graph_list->count(); i++) {
            prev_vals.append(0);
        }
        QList<QList<int>> tmp_vals_frm_svd;
        if(svd_vals->count() != 0) {
            for(int i = 0; i < svd_vals->at(0)->count(); i++) {
                QList<int> new_lst;
                new_lst.clear();
                for(int k = 0; k < svd_vals->count(); k++) {
                    if(k < dsplbl_pins_lst.count()) {
                        new_lst.append(0);
                        new_lst.append(svd_vals->at(k)->at(i));
                    }
                }
                if(new_lst.count() != 0) {
                    tmp_vals_frm_svd.append(new_lst);
                }
            }
        }
        if(tmp_vals_frm_svd.count() != 0) {
            for(int i = 0; i < svd_dbg_time->count(); i++) {
                bool val_changed = false;
                for(int k = 0; k < tmp_vals_frm_svd.at(i).count(); k++) {
                    if(tmp_vals_frm_svd.at(i).at(k) != prev_vals.at(k)) {
                        val_changed = true;
                        break;
                    }
                }
                add_data_to_graph(tmp_vals_frm_svd.at(i), &prev_vals, svd_dbg_time->at(i), val_changed);
            }
        }
        ui->diagram->replot();
    }
}

void Waveform_Viewer_Widget::limit_axis_range(QCPAxis *axis, const QCPRange &new_range, const QCPRange &limit_range) {
    auto lower_bound = limit_range.lower;
    auto upper_bound = limit_range.upper;
    QCPRange fixed_range(new_range);
    if(fixed_range.lower < lower_bound) {     // code assumes upperBound > lowerBound
        fixed_range.lower = lower_bound;
        fixed_range.upper = lower_bound + new_range.size();
        if((fixed_range.upper > upper_bound) || (qFuzzyCompare(new_range.size(), (upper_bound - lower_bound)))) {
            fixed_range.upper = upper_bound;
        }
        axis->setRange(fixed_range);         // adapt this line to use your plot/axis
    } else if(fixed_range.upper > upper_bound) {
        fixed_range.upper = upper_bound;
        fixed_range.lower = upper_bound - new_range.size();
        if((fixed_range.lower < lower_bound) || (qFuzzyCompare(new_range.size(), (upper_bound - lower_bound)))) {
            fixed_range.lower = lower_bound;
        }
        axis->setRange(fixed_range);         // adapt this line to use your plot/axis
    }
}

bool Waveform_Viewer_Widget::mouse_inside_object(double x_coord, double y_coord, bool graph_drawing_rect) {
    double top = ui->diagram->yAxis->pixelToCoord(ui->diagram->yAxis->axisRect()->top());
    double bottom = ui->diagram->yAxis->pixelToCoord(ui->diagram->yAxis->axisRect()->bottom());
    double left = ui->diagram->xAxis->pixelToCoord(ui->diagram->xAxis->axisRect()->left());
    double right = ui->diagram->xAxis->pixelToCoord(ui->diagram->xAxis->axisRect()->right());
    if(graph_drawing_rect) {
        if((x_coord <= left) || (y_coord <= bottom) || (x_coord >= right) || (y_coord >= top)) {
            return false;
        } else {
            return true;
        }
    } else {
        if((x_coord < left) && (y_coord >= (bottom + 1.0)) && (y_coord < top)) {
            return true;
        } else {
            return false;
        }
    }
}

void Waveform_Viewer_Widget::slot_mouse_move(QMouseEvent *event) {
    double x_coord = ui->diagram->xAxis->pixelToCoord(event->pos().x());
    double y_coord = ui->diagram->yAxis->pixelToCoord(event->pos().y());
    if(mouse_inside_object(x_coord, y_coord, false)) {
        if(ui->diagram->layer("layerCursor")->visible()) {
            ui->diagram->layer("layerCursor")->setVisible(false);
            ui->diagram->replot();
        }
        qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
        if(drag_pressed && !debugging) {
            double rnd_y = round(y_coord / 2.0) * 2.0 - 1.0;
            if(rnd_y < 1.0) {
                rnd_y = 1.0;
            }
            int int_cur_y = static_cast<int>(rnd_y);
            if(!drag_graph) {
                coef = int_cur_y;
                drag_graph = true;
            } else {
                if(int_cur_y != coef) {
                    QList<int> prev_vals;
                    prev_vals.clear();
                    QList<QList<int>> tmp_vals_frm_svd;
                    if((svd_vals->count() != 0) && add_names) {
                        svd_vals->swap(((coef - 1) / 2), ((int_cur_y - 1) / 2));
                        for(int i = 0; i < svd_vals->at(0)->count(); i++) {
                            QList<int> new_lst;
                            new_lst.clear();
                            for(int k = 0; k < svd_vals->count(); k++) {
                                new_lst.append(0);
                                new_lst.append(svd_vals->at(k)->at(i));
                                if(tmp_vals_frm_svd.count() == 0) {
                                    prev_vals.append(0);
                                    prev_vals.append(0);
                                }
                            }
                            if(new_lst.count() != 0) {
                                tmp_vals_frm_svd.append(new_lst);
                            }
                        }
                    }
                    QList<QString> old_ticks = (*textTicker)->ticks().values();
                    remove_data_from_graph();
                    int new_coef = ((coef - 1) / 2);
                    int new_cur_y = ((int_cur_y - 1) / 2);
                    pin_names->clear();
                    for(int i = 0; i < old_ticks.count(); i++) {
                        if(i == new_coef) {
                            pin_names->append(old_ticks.at(new_cur_y));
                        } else if(i == new_cur_y) {
                            pin_names->append(old_ticks.at(new_coef));
                        } else {
                            pin_names->append(old_ticks.at(i));
                        }
                    }
                    change_pin_names();
                    if(pin_names_board->count() == 0) {
                        *pin_names_board = *pin_names;
                    }
                    if(add_names) {
                        for(int i = 0; i < pin_names->count(); i++) {
                            pin_names_board->replace(i, pin_names->at(i));
                        }
                    }
                    if(tmp_vals_frm_svd.count() != 0) {
                        for(int i = 0; i < svd_dbg_time->count(); i++) {
                            bool val_changed = false;
                            for(int k = 0; k < tmp_vals_frm_svd.at(i).count(); k++) {
                                if(tmp_vals_frm_svd.at(i).at(k) != prev_vals.at(k)) {
                                    val_changed = true;
                                    break;
                                }
                            }
                            add_data_to_graph(tmp_vals_frm_svd.at(i), &prev_vals, svd_dbg_time->at(i), val_changed);
                        }
                    }
                    ui->diagram->replot();
                    coef = int_cur_y;
                }
            }
        }
    } else if(!zoom_pressed) {
        ui->diagram->setSelectionRectMode(QCP::srmNone);
        if(!mouse_inside_object(x_coord, y_coord, true)) {
            if(ui->diagram->layer("layerCursor")->visible()) {
                ui->diagram->layer("layerCursor")->setVisible(false);
                ui->diagram->replot();
            }
            qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
        } else {
            if(!ui->diagram->layer("layerCursor")->visible()) {
                ui->diagram->layer("layerCursor")->setVisible(true);
            }
            qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
            if(static_cast<int>(ui->chckBx_attch_crsr->checkState()) == 2) {
                double rnd_x = round(x_coord / x_tckr_step) * x_tckr_step;
                curs_time->position->setCoords(rnd_x, y_coord);
                curs_time->setText(QString::number(rnd_x * time_coef) + " " + ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->currentText());
                curs_ver_line->start->setCoords(rnd_x, -QCPRange::maxRange);
                curs_ver_line->end->setCoords(rnd_x, QCPRange::maxRange);
            } else {
                curs_time->position->setCoords(x_coord, y_coord);
                curs_time->setText(QString::number(x_coord * time_coef) + " " + ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->currentText());
                curs_ver_line->start->setCoords(x_coord, -QCPRange::maxRange);
                curs_ver_line->end->setCoords(x_coord, QCPRange::maxRange);
            }
            ui->diagram->layer("layerCursor")->replot();
        }
    }
}

void Waveform_Viewer_Widget::slot_mouse_pressed(QMouseEvent *event) {
    if(event->button() == Qt::RightButton) {
        double x_coord = ui->diagram->xAxis->pixelToCoord(event->pos().x());
        double y_coord = ui->diagram->yAxis->pixelToCoord(event->pos().y());
        if(mouse_inside_object(x_coord, y_coord, true)) {
            zoom_pressed = true;
            ui->diagram->layer("layerCursor")->setVisible(false);
            ui->diagram->replot();
            ui->diagram->setSelectionRectMode(QCP::srmZoom);
        }
    } else if(event->button() == Qt::LeftButton) {
        emit ui->diagram->plottableClick(ui->diagram->plottable(1), 3, event);
        drag_pressed = true;
    }
}

void Waveform_Viewer_Widget::slot_mouse_unpressed(QMouseEvent *event) {
    if(event->button() == Qt::RightButton) {
        zoom_pressed = false;
    } else if(event->button() == Qt::LeftButton) {
        drag_pressed = false;
        drag_graph = false;
    }
}

void Waveform_Viewer_Widget::slot_x_axis_changed(const QCPRange &new_range) {
    if(!plot_re_scale) {
        QCPAxis *axis = qobject_cast<QCPAxis *>(QObject::sender());
        QCustomPlot *plot = axis->parentPlot();
        QCPRange limit_range(plot->property("xmin").toDouble(), plot->property("xmax").toDouble());
        limit_axis_range(axis, new_range, limit_range);
    }
}

void Waveform_Viewer_Widget::slot_y_axis_changed(const QCPRange &new_range) {
    if(!plot_re_scale) {
        QCPAxis *axis = qobject_cast<QCPAxis *>(QObject::sender());
        QCustomPlot *plot = axis->parentPlot();
        QCPRange limit_range(plot->property("ymin").toDouble(), plot->property("ymax").toDouble());
        limit_axis_range(axis, new_range, limit_range);
    }
}

void Waveform_Viewer_Widget::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}

////////////////////////////////////////////////////DIALOG SELECT DISPLAYABLE PINS///////////////////////////////////////////////////
Dialog_Select_Displayable_Pins::Dialog_Select_Displayable_Pins(QStringList avlbl_pins, QStringList dsplbl_pins, QWidget *parent) : QDialog(parent), ui(new Ui::Dialog_Select_Displayable_Pins) {
    ui->setupUi(this);
    available_pins_model = new QStringListModel(this);
    available_pins_model->setStringList(avlbl_pins);
    ui->listView_All->setModel(available_pins_model);
    ui->listView_All->setEditTriggers(QAbstractItemView::NoEditTriggers);
    displayable_pins_model = new QStringListModel(this);
    displayable_pins_model->setStringList(dsplbl_pins);
    ui->listView_Sel->setModel(displayable_pins_model);
    ui->listView_Sel->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->listView_All->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(available_pins_selection_changed(const QItemSelection &)));
    this->setWindowTitle(tr("Select pins for display"));
    this->setFixedSize(600, 300);
    this->updateGeometry();
    connect(ui->listView_Sel->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(displayable_pins_selection_changed(const QItemSelection &)));
    ui->pushButton_Ok->setText(tr("OK"));
    ui->listView_All->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listView_All->selectionModel()->clearSelection();
    ui->listView_Sel->selectionModel()->clearSelection();
    ui->pushButton_Del->setEnabled(false);
    ui->pushButton_Add->setEnabled(false);
}

Dialog_Select_Displayable_Pins::~Dialog_Select_Displayable_Pins() {
    delete available_pins_model;
    delete displayable_pins_model;
    delete ui;
}

void Dialog_Select_Displayable_Pins::on_pushButton_Ok_clicked() {
    QDialog::done(1);
}

void Dialog_Select_Displayable_Pins::on_pushButton_Cancell_clicked() {
    QDialog::done(0);
}

void Dialog_Select_Displayable_Pins::on_pushButton_Add_clicked() {
    replace_selected_pin(displayable_pins_model, available_pins_model, ui->listView_Sel, ui->listView_All, ui->pushButton_Add);
}

void Dialog_Select_Displayable_Pins::on_pushButton_Del_clicked() {
    replace_selected_pin(available_pins_model, displayable_pins_model, ui->listView_All, ui->listView_Sel, ui->pushButton_Del);
}

void Dialog_Select_Displayable_Pins::replace_selected_pin(QStringListModel *recv_model, QStringListModel *sndr_model, QListView *recv_view, QListView *sndr_view, QPushButton *sndr_button) {
    QStringList lst = recv_model->stringList();
    lst.append(sndr_model->stringList().at(sndr_view->currentIndex().row()));
    recv_model->setStringList(lst);
    recv_view->setModel(recv_model);
    sndr_model->removeRows(sndr_view->currentIndex().row(), 1);
    if(sndr_model->rowCount() == 0) {
        sndr_view->selectionModel()->clearSelection();
        sndr_button->setEnabled(false);
    }
}

void Dialog_Select_Displayable_Pins::displayable_pins_selection_changed(const QItemSelection &sel) {
    Q_UNUSED(sel)
    ui->pushButton_Del->setEnabled(true);
    ui->listView_All->selectionModel()->clearSelection();
}

void Dialog_Select_Displayable_Pins::available_pins_selection_changed(const QItemSelection &sel) {
    if(sel.indexes().count() > 0) {
        QModelIndex index = sel.indexes().first();
        QString pin_name = available_pins_model->data(index).toString();
        ui->listView_Sel->selectionModel()->clearSelection();
        ui->pushButton_Del->setEnabled(false);
        if(displayable_pins_model->stringList().contains(pin_name)) {
            ui->pushButton_Add->setEnabled(false);
        } else {
            ui->pushButton_Add->setEnabled(true);
        }
    } else {
        ui->pushButton_Add->setEnabled(false);
    }
}

QStringList Dialog_Select_Displayable_Pins::get_displayable_pins() {
    return displayable_pins_model->stringList();
}

QStringList Dialog_Select_Displayable_Pins::get_available_pins() {
    return available_pins_model->stringList();
}

