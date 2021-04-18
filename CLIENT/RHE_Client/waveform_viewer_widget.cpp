﻿#include "waveform_viewer_widget.h"
#include "ui_waveform_viewer.h"
#include "ui_dialog_select_displayable_pins.h"
#include "ui_dialog_select_diagram_settings.h"

Waveform_Viewer_Widget::Waveform_Viewer_Widget(QWidget *parent, General_Widget *widg, bool stndln) : QWidget(parent), ui(new Ui::Waveform_Viewer) {
    ui->setupUi(this);
    gen_widg = widg;
    standalone = stndln;
    ui->chckBx_as_wndw->setVisible(!stndln);
    textTicker = new QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());
    dyn_tckr = new QSharedPointer<QCPAxisTickerFixed>(new QCPAxisTickerFixed());
//    ui->diagram->setOpenGl(true);         //qcustomplot.cpp - line 909
    curs_ver_line = new QCPItemLine(ui->diagram);
    frst_msr_line = new QCPItemLine(ui->diagram);
    scnd_msr_line = new QCPItemLine(ui->diagram);
    thrd_msr_line = new QCPItemLine(ui->diagram);
    curs_time = new QCPItemText(ui->diagram);
    msr_time = new QCPItemText(ui->diagram);
    change_margin(5 * static_cast<int>(stndln));
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
    if(shw_at_cntr) {
        shw_at_cntr = false;
        this->move(((QGuiApplication::primaryScreen()->geometry().width() - this->geometry().width()) / 2), ((QGuiApplication::primaryScreen()->geometry().height() - this->geometry().height()) / 2));
    }
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

void Waveform_Viewer_Widget::on_chckBx_as_wndw_stateChanged(int state) {
    change_margin(5 * (state / 2));
    as_window = static_cast<bool>(state);
    if(state == 2) {
        this->setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);
        shw_at_cntr = true;
        this->show();
    } else {
        this->setWindowFlags(flags);
    }
    emit as_window_signal();
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
        set_measurement_label_text();
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

void Waveform_Viewer_Widget::on_pshBttn_chng_sttngs_clicked() {
    select_diagram_settings();
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

void Waveform_Viewer_Widget::on_pshBttn_msr_toggled(bool checked) {
    measure = checked;
    ui->pshBttn_clr->setEnabled(!measure);
    ui->pshBttn_chng_sttngs->setEnabled(!measure);
    ui->pshBttn_open_save_wvfrm->setEnabled(!measure);
    ui->pshBttn_slct_dsplbl_pins->setEnabled(!measure);
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
    ui->diagram->addLayer("layerCursor", 0, QCustomPlot::limAbove);
    ui->diagram->layer("layerCursor")->setMode(QCPLayer::lmBuffered);
    ui->diagram->addLayer("layerMeasure", 0, QCustomPlot::limAbove);
    ui->diagram->layer("layerMeasure")->setMode(QCPLayer::lmBuffered);
    ui->diagram->xAxis->setSubTicks(false);
    ui->diagram->yAxis->setSubTicks(false);
    change_settings();
    curs_time->setPadding(QMargins(2, 2, 2, 2));
    curs_time->setLayer("layerCursor");
    msr_time->setPadding(QMargins(2, 2, 2, 2));
    msr_time->setLayer("layerMeasure");
    msr_time->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    curs_ver_line->setLayer("layerCursor");
    frst_msr_line->setLayer("layerMeasure");
    scnd_msr_line->setLayer("layerMeasure");
    thrd_msr_line->setLayer("layerMeasure");
    ui->diagram->layer("layerCursor")->setVisible(false);
    ui->diagram->layer("layerMeasure")->setVisible(false);
    frst_msr_line->setVisible(false);
    scnd_msr_line->setVisible(false);
    thrd_msr_line->setVisible(false);
    msr_time->setVisible(false);
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
    set_measurement_label_text();
    language_changed = true;
}

void Waveform_Viewer_Widget::set_measurement_label_text() {
    if(msr_time->visible()) {
        msr_time->setText(QString::number(fabs(scnd_msr_line->start->key() - frst_msr_line->start->key()) * time_coef) + " " + ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->currentText());
    }
}

void Waveform_Viewer_Widget::change_settings() {
    QFont fnt = QFont("Tahoma", gen_widg->get_setting("settings/WVFRM_VWR_AXIS_LABELS_FONT_SIZE").toInt());
    ui->diagram->xAxis->setTickLabelFont(fnt);
    ui->diagram->yAxis->setTickLabelFont(fnt);
    QColor dgrm_grd_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_DIAGRAM_GRID_COLOR").toString());
    QColor axs_lbls_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_AXIS_LABELS_COLOR").toString());
    QColor dgrm_bckgrnd_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_DIAGRAM_BACKGROUND_COLOR").toString());
    QColor slctn_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_SELECTION_COLOR").toString());
    QColor slctn_brsh_clr = slctn_clr;
    slctn_brsh_clr.setAlpha(80);
    QColor line_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_LINE_COLOR").toString());
    QColor lbl_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_BORDER_COLOR").toString());
    QColor lbl_brsh_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_FILL_COLOR").toString());
    grph_clr = QColor(gen_widg->get_setting("settings/WVFRM_VWR_GRAPH_COLOR").toString());
    for(int i = 0; i < graph_list->count(); i++) {
        if((i % 2) == 1) {
            graph_list->at(i)->setPen(QPen(grph_clr, 1));
        }
    }
    grph_brsh_clr = grph_clr;
    grph_brsh_clr.setAlpha(60);
    ui->diagram->xAxis->setBasePen(QPen(dgrm_grd_clr, 1));
    ui->diagram->yAxis->setBasePen(QPen(dgrm_grd_clr, 1));
    ui->diagram->xAxis->setTickPen(QPen(dgrm_grd_clr, 1));
    ui->diagram->yAxis->setTickPen(QPen(dgrm_grd_clr, 1));
    ui->diagram->xAxis->grid()->setPen(QPen(dgrm_grd_clr, 1, Qt::SolidLine));
    ui->diagram->yAxis->grid()->setPen(QPen(dgrm_grd_clr, 1, Qt::SolidLine));
    ui->diagram->xAxis->setTickLabelColor(axs_lbls_clr);
    ui->diagram->yAxis->setTickLabelColor(axs_lbls_clr);
    QLinearGradient plot_gradient;
    plot_gradient.setStart(0, 0);
    plot_gradient.setFinalStop(0, 350);
    plot_gradient.setColorAt(0, dgrm_bckgrnd_clr);
    plot_gradient.setColorAt(1, dgrm_bckgrnd_clr);
    ui->diagram->setBackground(plot_gradient);
    QLinearGradient axis_rect_gradient;
    axis_rect_gradient.setStart(0, 0);
    axis_rect_gradient.setFinalStop(0, 350);
    axis_rect_gradient.setColorAt(0, dgrm_bckgrnd_clr);
    axis_rect_gradient.setColorAt(1, dgrm_bckgrnd_clr);
    ui->diagram->axisRect()->setBackground(axis_rect_gradient);
    ui->diagram->selectionRect()->setPen(QPen(slctn_clr, 1, Qt::SolidLine));
    ui->diagram->selectionRect()->setBrush(QBrush(slctn_brsh_clr));
    curs_ver_line->setPen(QPen(line_clr, 1, Qt::SolidLine));
    frst_msr_line->setPen(QPen(line_clr, 1, Qt::SolidLine));
    scnd_msr_line->setPen(QPen(line_clr, 1, Qt::SolidLine));
    thrd_msr_line->setPen(QPen(line_clr, 1, Qt::SolidLine));
    curs_time->setPen(QPen(lbl_clr, 1, Qt::SolidLine));
    curs_time->setBrush(QBrush(lbl_brsh_clr));
    curs_time->setFont(fnt);
    msr_time->setPen(QPen(lbl_clr, 1, Qt::SolidLine));
    msr_time->setBrush(QBrush(lbl_brsh_clr));
    msr_time->setFont(fnt);
}

void Waveform_Viewer_Widget::change_margin(int val) {
    ui->horizontalLayout->setContentsMargins(val, 5, val, val);
    ui->horizontalLayout_2->setContentsMargins(val, val, val, 5);
}

void Waveform_Viewer_Widget::load_waveform() {
    QStringList *lst = gen_widg->load_files(false, false, tr("Choose waveform file"), tr("Waveform (*.wvfrm)"));
    if((lst == nullptr) || (lst->count() == 0)) {
        gen_widg->show_message_box(tr("Error"), tr("Waveform file not choosed"), 0);
        return;
    }
    QFile *file = new QFile(QFileInfo(lst->at(0)).absoluteFilePath());
    if(file->open(QFile::ReadOnly)) {
        reset_measurement_data();
        QString str = file->readLine();
        file->close();
        str.replace("\n", "");
        QRegExp tag_exp("/");
        QStringList lst = str.split(tag_exp);
        add_names = true;
        debug_show = true;
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
                            if(str.compare(str_dat, Qt::CaseInsensitive) == 0) {
                                QString tmp = "pin ";
                                if(i < 10) {
                                    tmp.append(QString::number(0));
                                }
                                tmp.append(QString::number(i));
                                add_pin_names(tmp);
                            }
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
    if(svd_dbg_time->count() != 0) {
        int cnt = svd_dbg_time->count();
        QString fn_nm = "";
        QString str = "";
        for(int i = 0; i < pin_names->count(); i++) {
            if(i == 0) {
                str.append("time/");
            }
            str.append(pin_names->at(i));
            if(i != (pin_names->count() - 1)) {
                str.append("/");
            } else {
                str.append("\n");
            }
        }
        gen_widg->save_file(this, tr("Saving waveform"), tr("Waveform (*.wvfrm)"), &str, &fn_nm, false, false);
        str.clear();
        for(int i = 0; i < cnt; i++) {
            str.clear();
            for(int k = 0; k < svd_vals->count(); k++) {
                if(k == 0) {
                    str.append(QString::number(svd_dbg_time->at(i)) + "/");
                }
                str.append(QString::number(svd_vals->at(k)->at(i)));
                if(k != (svd_vals->count() - 1)) {
                    str.append("/");
                } else {
                    str.append("\n");
                }
            }
            gen_widg->save_file(this, tr("Saving waveform"), tr("Waveform (*.wvfrm)"), &str, &fn_nm, false, true);
        }
    } else {
        gen_widg->show_message_box("", tr("No data for saving"), 0);
    }
}

void Waveform_Viewer_Widget::add_graphs_to_plot() {
    for(int i = 0; i < graph_count; i++) {
        graph_list->append(ui->diagram->addGraph());
        graph_list->at(i * 2 + 0)->setPen(QPen(QColor("#00000000"), 1));
        graph_list->at(i * 2 + 0)->setBrush(QBrush(QColor("#00000000")));
        graph_list->append(ui->diagram->addGraph());
        graph_list->at(i * 2 + 1)->setPen(QPen(grph_clr, 1));
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

void Waveform_Viewer_Widget::add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed) {
    for(int i = 0; i < pin_names_board->count() * 2; i++) {
        if((i < graph_list->count()) && debug_show) {
            int shft_val = abs((i % 2) - 1);
            if(val_changed) {
                graph_list->at(i)->addData(time, (shft_val + i + prev_vals->at(i)));
            }
            if((val.at(i) == 1) && (i != 0)) {
                graph_list->at(i)->setBrush(QBrush(grph_brsh_clr));
                graph_list->at(i)->setChannelFillGraph(graph_list->at(i - 1));
            }
            graph_list->at(i)->addData(time, (shft_val + i + val.at(i)));
        }
        if(i < prev_vals->count()) {
            (*prev_vals).replace(i, val.at(i));
        }
    }
}

void Waveform_Viewer_Widget::add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed) {
    add_data_to_graph(val, prev_vals, time, val_changed);
    ui->diagram->replot();
}

void Waveform_Viewer_Widget::remove_data_from_graph() {
    for(int i = 0; i < graph_list->count(); i++) {
        graph_list->at(i)->data().data()->clear();
        graph_list->at(i)->data().clear();
    }
    reset_measurement_data();
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

void Waveform_Viewer_Widget::select_diagram_settings() {
    QList<QString> sttngs_lst;
    sttngs_lst.clear();
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_AXIS_LABELS_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_DIAGRAM_GRID_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_DIAGRAM_BACKGROUND_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_SELECTION_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_LINE_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_BORDER_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_FILL_COLOR").toString());
    sttngs_lst.append(gen_widg->get_setting("settings/WVFRM_VWR_GRAPH_COLOR").toString());
    sttngs_lst.append(QString::number(gen_widg->get_setting("settings/WVFRM_VWR_AXIS_LABELS_FONT_SIZE").toInt()));
    Dialog_Select_Diagram_Settings settings_select_dialog(sttngs_lst, this);
    int dlgCase = settings_select_dialog.exec();
    if(dlgCase == 1) {
        QList<QString> sttngs_lst_new = settings_select_dialog.get_diagram_settings();
        gen_widg->save_setting("settings/WVFRM_VWR_AXIS_LABELS_COLOR", sttngs_lst_new.at(0));
        gen_widg->save_setting("settings/WVFRM_VWR_DIAGRAM_GRID_COLOR", sttngs_lst_new.at(1));
        gen_widg->save_setting("settings/WVFRM_VWR_DIAGRAM_BACKGROUND_COLOR", sttngs_lst_new.at(2));
        gen_widg->save_setting("settings/WVFRM_VWR_SELECTION_COLOR", sttngs_lst_new.at(3));
        gen_widg->save_setting("settings/WVFRM_VWR_CURSOR_LINE_COLOR", sttngs_lst_new.at(4));
        gen_widg->save_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_BORDER_COLOR", sttngs_lst_new.at(5));
        gen_widg->save_setting("settings/WVFRM_VWR_CURSOR_TIME_LABEL_FILL_COLOR", sttngs_lst_new.at(6));
        gen_widg->save_setting("settings/WVFRM_VWR_GRAPH_COLOR", sttngs_lst_new.at(7));
        gen_widg->save_setting("settings/WVFRM_VWR_AXIS_LABELS_FONT_SIZE", sttngs_lst_new.at(8).toInt());
        change_settings();
        remove_data_from_graph();
        draw_from_saved_vals(pin_names->count());
    }
}

void Waveform_Viewer_Widget::select_displayable_pins() {
    QStringList avlbl_pins, dsplbl_pins;
    avlbl_pins.append(*pin_names_board);
    if(add_names && debug_show) {
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
        debug_show = static_cast<bool>(dsplbl_pins_lst.count());
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
        draw_from_saved_vals(dsplbl_pins_lst.count());
    }
}

void Waveform_Viewer_Widget::draw_from_saved_vals(int val) {
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
                if(k < val) {
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

void Waveform_Viewer_Widget::draw_line(QCPItemLine *line, double x_start, double x_end, double y_start, double y_end) {
    line->start->setCoords(x_start, y_start);
    line->end->setCoords(x_end, y_end);
    line->setVisible(true);
}

void Waveform_Viewer_Widget::reset_measurement_data() {
    count_of_press = 0;
    frst_msr_line->setVisible(false);
    scnd_msr_line->setVisible(false);
    thrd_msr_line->setVisible(false);
    msr_time->setVisible(false);
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
        if((x_coord < left) && (y_coord >= (bottom + 0.5)) && (y_coord < top)) {
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
        if(this->cursor().shape() != Qt::ClosedHandCursor) {
            qApp->setOverrideCursor(QCursor(Qt::OpenHandCursor));
            this->setCursor(Qt::OpenHandCursor);
        }
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
            this->setCursor(Qt::ArrowCursor);
        } else {
            if(!ui->diagram->layer("layerCursor")->visible()) {
                ui->diagram->layer("layerCursor")->setVisible(true);
            }
            qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
            this->setCursor(Qt::BlankCursor);
            if(static_cast<int>(ui->chckBx_attch_crsr->checkState()) == 2) {
                x_coord = round(x_coord / x_tckr_step) * x_tckr_step;
            }
            curs_time->position->setCoords(x_coord, y_coord);
            curs_time->setText(QString::number(x_coord * time_coef) + " " + ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->currentText());
            draw_line(curs_ver_line, x_coord, x_coord, -QCPRange::maxRange, QCPRange::maxRange);
            ui->diagram->layer("layerCursor")->replot();
            ui->diagram->layer("layerMeasure")->replot();
        }
    }
}

void Waveform_Viewer_Widget::slot_mouse_pressed(QMouseEvent *event) {
    if(this->cursor().shape() == Qt::OpenHandCursor) {
        qApp->setOverrideCursor(QCursor(Qt::ClosedHandCursor));
        this->setCursor(Qt::ClosedHandCursor);
    }
    if(event->button() == Qt::RightButton) {
        if(measure) {
            reset_measurement_data();
            ui->diagram->layer("layerMeasure")->replot();
        } else {
            double x_coord = ui->diagram->xAxis->pixelToCoord(event->pos().x());
            double y_coord = ui->diagram->yAxis->pixelToCoord(event->pos().y());
            if(mouse_inside_object(x_coord, y_coord, true)) {
                zoom_pressed = true;
                ui->diagram->layer("layerCursor")->setVisible(false);
                ui->diagram->replot();
                ui->diagram->setSelectionRectMode(QCP::srmZoom);
            }
        }
    } else if(event->button() == Qt::LeftButton) {
        if(measure) {
            double x_coord = ui->diagram->xAxis->pixelToCoord(event->pos().x());
            if(static_cast<int>(ui->chckBx_attch_crsr->checkState()) == 2) {
                x_coord = round(x_coord / x_tckr_step) * x_tckr_step;
            }
            double y_coord = ui->diagram->yAxis->pixelToCoord(event->pos().y());
            count_of_press++;
            if(count_of_press == 3) {
                reset_measurement_data();
                count_of_press++;
            }
            if(count_of_press == 1) {
                if(!ui->diagram->layer("layerMeasure")->visible()) {
                    ui->diagram->layer("layerMeasure")->setVisible(true);
                    ui->diagram->replot();
                }
                draw_line(frst_msr_line, x_coord, x_coord, -QCPRange::maxRange, QCPRange::maxRange);
            } else if(count_of_press == 2) {
                draw_line(scnd_msr_line, x_coord, x_coord, -QCPRange::maxRange, QCPRange::maxRange);
                draw_line(thrd_msr_line, frst_msr_line->start->key(), x_coord, y_coord, y_coord);
                msr_time->position->setCoords(((x_coord + frst_msr_line->start->key()) / 2), y_coord);
                msr_time->setVisible(true);
                set_measurement_label_text();
            }
            ui->diagram->layer("layerMeasure")->replot();
        } else {
            emit ui->diagram->plottableClick(ui->diagram->plottable(1), 3, event);
            drag_pressed = true;
        }
    }
}

void Waveform_Viewer_Widget::slot_mouse_unpressed(QMouseEvent *event) {
    if(this->cursor().shape() == Qt::ClosedHandCursor) {
        qApp->setOverrideCursor(QCursor(Qt::OpenHandCursor));
        this->setCursor(Qt::OpenHandCursor);
    }
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
    this->setWindowTitle(tr("Select pins for display"));
    this->setFixedSize(600, 300);
    this->updateGeometry();
    available_pins_model = new QStringListModel(this);
    available_pins_model->setStringList(avlbl_pins);
    ui->lstVw_avlbl_pins->setModel(available_pins_model);
    ui->lstVw_avlbl_pins->setEditTriggers(QAbstractItemView::NoEditTriggers);
    displayable_pins_model = new QStringListModel(this);
    displayable_pins_model->setStringList(dsplbl_pins);
    ui->lstVw_dspbl_pins->setModel(displayable_pins_model);
    ui->lstVw_dspbl_pins->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->lstVw_avlbl_pins->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(available_pins_selection_changed(const QItemSelection &)));
    connect(ui->lstVw_dspbl_pins->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(displayable_pins_selection_changed(const QItemSelection &)));
    ui->lstVw_avlbl_pins->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lstVw_avlbl_pins->selectionModel()->clearSelection();
    ui->lstVw_dspbl_pins->selectionModel()->clearSelection();
    ui->pshBttn_dlt->setEnabled(false);
    ui->pshBttn_add->setEnabled(false);
}

Dialog_Select_Displayable_Pins::~Dialog_Select_Displayable_Pins() {
    delete available_pins_model;
    delete displayable_pins_model;
    delete ui;
}

void Dialog_Select_Displayable_Pins::on_pshBttn_ok_clicked() {
    QDialog::done(1);
}

void Dialog_Select_Displayable_Pins::on_pshBttn_cncl_clicked() {
    QDialog::done(0);
}

void Dialog_Select_Displayable_Pins::on_pshBttn_add_clicked() {
    replace_selected_pin(displayable_pins_model, available_pins_model, ui->lstVw_dspbl_pins, ui->lstVw_avlbl_pins, ui->pshBttn_add);
}

void Dialog_Select_Displayable_Pins::on_pshBttn_dlt_clicked() {
    replace_selected_pin(available_pins_model, displayable_pins_model, ui->lstVw_avlbl_pins, ui->lstVw_dspbl_pins, ui->pshBttn_dlt);
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
    Q_UNUSED(sel);
    ui->pshBttn_dlt->setEnabled(true);
    ui->lstVw_avlbl_pins->selectionModel()->clearSelection();
}

void Dialog_Select_Displayable_Pins::available_pins_selection_changed(const QItemSelection &sel) {
    if(sel.indexes().count() > 0) {
        QModelIndex index = sel.indexes().first();
        QString pin_name = available_pins_model->data(index).toString();
        ui->lstVw_dspbl_pins->selectionModel()->clearSelection();
        ui->pshBttn_dlt->setEnabled(false);
        if(displayable_pins_model->stringList().contains(pin_name)) {
            ui->pshBttn_add->setEnabled(false);
        } else {
            ui->pshBttn_add->setEnabled(true);
        }
    } else {
        ui->pshBttn_add->setEnabled(false);
    }
}

QStringList Dialog_Select_Displayable_Pins::get_displayable_pins() {
    return displayable_pins_model->stringList();
}

QStringList Dialog_Select_Displayable_Pins::get_available_pins() {
    return available_pins_model->stringList();
}

////////////////////////////////////////////////////DIALOG SELECT DIAGRAM SETTINGS///////////////////////////////////////////////////
Dialog_Select_Diagram_Settings::Dialog_Select_Diagram_Settings(QList<QString> _sttngs_lst, QWidget *parent) : QDialog(parent), ui(new Ui::Dialog_Select_Diagram_Settings) {
    ui->setupUi(this);
    this->setWindowTitle(tr("Select diagram settings"));
    this->setFixedSize(600, 300);
    this->updateGeometry();
    sttngs_lst = _sttngs_lst;
    bttns_lst = new QList<QPushButton*>();
    bttns_lst->append(ui->pshBttn_axs_lbls_clr);
    bttns_lst->append(ui->pshBttn_dgrm_grd_clr);
    bttns_lst->append(ui->pshBttn_dgrm_bckgrnd_clr);
    bttns_lst->append(ui->pshBttn_slctn_clr);
    bttns_lst->append(ui->pshBttn_crsr_ln_clr);
    bttns_lst->append(ui->pshBttn_crsr_tm_lbl_brdr_clr);
    bttns_lst->append(ui->pshBttn_crsr_tm_lbl_fll_clr);
    bttns_lst->append(ui->pshBttn_grph_clr);
    for(int i = 0; i < bttns_lst->count(); i++) {
        bttns_lst->at(i)->setStyleSheet("background-color: " + sttngs_lst.at(i));
    }
    ui->spnBx_axs_lbls_fnr_sz->setValue(sttngs_lst.last().toInt());
}

Dialog_Select_Diagram_Settings::~Dialog_Select_Diagram_Settings() {
    delete bttns_lst;
    delete ui;
}

void Dialog_Select_Diagram_Settings::on_pshBttn_axs_lbls_clr_clicked() {
    change_color(0);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_dgrm_grd_clr_clicked() {
    change_color(1);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_dgrm_bckgrnd_clr_clicked() {
    change_color(2);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_slctn_clr_clicked() {
    change_color(3);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_crsr_ln_clr_clicked() {
    change_color(4);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_crsr_tm_lbl_brdr_clr_clicked() {
    change_color(5);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_crsr_tm_lbl_fll_clr_clicked() {
    change_color(6);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_grph_clr_clicked() {
    change_color(7);
}

void Dialog_Select_Diagram_Settings::on_spnBx_axs_lbls_fnr_sz_valueChanged(int value) {
    sttngs_lst.replace(8, QString::number(value));
}

void Dialog_Select_Diagram_Settings::on_pshBttn_ok_clicked() {
    QDialog::done(1);
}

void Dialog_Select_Diagram_Settings::on_pshBttn_cncl_clicked() {
    QDialog::done(0);
}

QList<QString> Dialog_Select_Diagram_Settings::get_diagram_settings() {
    return sttngs_lst;
}

void Dialog_Select_Diagram_Settings::change_color(int value) {
    QColorDialog dlg(this);
    QColor color = dlg.getColor(QColor(sttngs_lst.at(value)));
    if(color.isValid()) {
        sttngs_lst.replace(value, color.name());
        bttns_lst->at(value)->setStyleSheet("background-color: " + color.name());
    }
}
