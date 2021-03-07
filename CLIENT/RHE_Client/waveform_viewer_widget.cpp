#include "waveform_viewer_widget.h"
#include "ui_waveform_viewer.h"

Waveform_Viewer_Widget::Waveform_Viewer_Widget(QWidget* parent, General_Widget *widg, bool stndln) : QWidget(parent), ui(new Ui::Waveform_Viewer) {
    ui->setupUi(this);
    gen_widg = widg;
    standalone = stndln;
    ui->chckBx_as_wndw->setVisible(!stndln);
    ui->horizontalSpacer_3->changeSize(5 * static_cast<int>(!stndln), ui->horizontalSpacer_3->geometry().height());
    textTicker = new QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());
    dyn_tckr = new QSharedPointer<QCPAxisTickerFixed>(new QCPAxisTickerFixed());
//    ui->diagram->setOpenGl(true);         //qcustomplot.cpp - line 909
    curs_ver_line = new QCPItemLine(ui->diagram);
    curs_time = new QCPItemText(ui->diagram);
    connect(ui->diagram->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_xAxisChanged(QCPRange)));
    connect(ui->diagram->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_yAxisChanged(QCPRange)));
    connect(ui->diagram, &QCustomPlot::mouseMove, this, &Waveform_Viewer_Widget::slot_mouse_move);
    connect(ui->diagram, &QCustomPlot::mousePress, this, &Waveform_Viewer_Widget::slot_mouse_pressed);
    connect(ui->diagram, &QCustomPlot::mouseRelease, this, &Waveform_Viewer_Widget::slot_mouse_unpressed);
    graph_list = new QList<QCPGraph *>();
    pin_names = new QList<QString>();
    pin_names_board = new QList<QString>();
    flags = this->windowFlags();
}

Waveform_Viewer_Widget::~Waveform_Viewer_Widget() {
    remove_graphs_form_plot();
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

void Waveform_Viewer_Widget::on_chckBx_attch_crsr_stateChanged(int state) {
    if(ui_initialized) {
        gen_widg->save_setting("settings/WVFRM_VWR_ATTCH_CRSR", state);
    }
}

void Waveform_Viewer_Widget::on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(int value) {
    if(ui_initialized) {
        gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME", value);
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
        gen_widg->save_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE", index);
        on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(ui->spnBx_wvfrm_vwr_dscrtnss_tm->value());
    }
}

void Waveform_Viewer_Widget::on_pshBttn_fl_scl_clicked() {
    re_scale_graph();
}

void Waveform_Viewer_Widget::on_pshBttn_clr_clicked() {
    remove_data_from_graph();
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
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(0, 0, 0));
    plotGradient.setColorAt(1, QColor(0, 0, 0));
    ui->diagram->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(0, 0, 0));
    axisRectGradient.setColorAt(1, QColor(0, 0, 0));
    ui->diagram->axisRect()->setBackground(axisRectGradient);
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
    ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(abs(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR").toInt() - 2)));
    ui->spnBx_wvfrm_vwr_dscrtnss_tm->setValue(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME").toInt());
    ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(abs(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE").toInt() - 1));
    ui_initialized = true;
    ui->chckBx_attch_crsr->setCheckState(static_cast<Qt::CheckState>(gen_widg->get_setting("settings/WVFRM_VWR_ATTCH_CRSR").toInt()));
    ui->cmbBx_wvfrm_vwr_dscrtnss_tm_tp->setCurrentIndex(gen_widg->get_setting("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE").toInt());
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
        QRegExp tagExp("/");
        QStringList lst = str.split(tagExp);
        plot_re_scale = true;
        remove_graphs_form_plot();
        graph_count = lst.count() - 1;
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
                            pin_names->append(lst_dat.at(i));
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
    on_pshBttn_clr_clicked();
}

void Waveform_Viewer_Widget::remove_graphs_form_plot() {
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
    for(int i = 0; i < graph_list->count(); i++) {
        int shft_val = abs((i % 2) - 1);
        if(val_changed) {
            graph_list->at(i)->addData(time, (shft_val + i + prev_vals->at(i)));
        }
        if((val.at(i) == 1) && (i != 0)) {
            graph_list->at(i)->setBrush(QBrush(QColor(0, 255, 0, 60)));
            graph_list->at(i)->setChannelFillGraph(graph_list->at(i - 1));
        }
        graph_list->at(i)->addData(time, (shft_val + i + val.at(i)));
        prev_vals->replace(i, val.at(i));
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
    re_scale_graph();
}

void Waveform_Viewer_Widget::add_pin_names(QString pin_name) {
    pin_names->append(pin_name);
    pin_names_board->append(pin_name);
}

QList<QString>* Waveform_Viewer_Widget::get_pin_names() {
    return pin_names;
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
            if((pin_names_board->count() == 0) || (i >= pin_names_board->count())) {
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

void Waveform_Viewer_Widget::limitAxisRange(QCPAxis *axis, const QCPRange &newRange, const QCPRange &limitRange) {
    auto lowerBound = limitRange.lower;
    auto upperBound = limitRange.upper;
    QCPRange fixedRange(newRange);
    if(fixedRange.lower < lowerBound) {     // code assumes upperBound > lowerBound
        fixedRange.lower = lowerBound;
        fixedRange.upper = lowerBound + newRange.size();
        if((fixedRange.upper > upperBound) || (qFuzzyCompare(newRange.size(), upperBound-lowerBound))) {
            fixedRange.upper = upperBound;
        }
        axis->setRange(fixedRange);         // adapt this line to use your plot/axis
    } else if(fixedRange.upper > upperBound) {
        fixedRange.upper = upperBound;
        fixedRange.lower = upperBound - newRange.size();
        if((fixedRange.lower < lowerBound) || (qFuzzyCompare(newRange.size(), upperBound-lowerBound))) {
            fixedRange.lower = lowerBound;
        }
        axis->setRange(fixedRange);         // adapt this line to use your plot/axis
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
                    QList<double> time;
                    time.clear();
                    QList<int> prev_vals;
                    prev_vals.clear();
                    for(int i = 0; i < graph_list->count(); i++) {
                        prev_vals.append(0);
                    }
                    QList<QList<int>*> vals;
                    vals.clear();
                    if(graph_list->count() != 0) {
                        for(int i = 0; i < graph_list->at(0)->data()->size(); i++) {
                            QList<int> *val = new QList<int>();
                            val->clear();
                            vals.append(val);
                        }
                        for(int i = 0; i < graph_list->at(0)->data()->size(); i++) {
                            for(int k = 0; k < graph_list->count(); k++) {
                                if(k == 0) {
                                    time.append(graph_list->at(k)->data()->at(i)->key);
                                }
                                if(k == coef) {
                                    vals.at(i)->append(abs((static_cast<int>(graph_list->at(int_cur_y)->data()->at(i)->value) % 2) - 1));
                                } else if(k == int_cur_y) {
                                    vals.at(i)->append(abs((static_cast<int>(graph_list->at(coef)->data()->at(i)->value) % 2) - 1));
                                } else {
                                    vals.at(i)->append(abs((static_cast<int>(graph_list->at(k)->data()->at(i)->value) % 2) - 1));
                                }
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
                    for(int i = 0; i < time.count(); i++) {
                        add_data_to_graph(*vals.at(i), &prev_vals, time.at(i), false);
                    }
                    ui->diagram->replot();
                    coef = int_cur_y;
                    while(vals.count() != 0) {
                        delete vals.at(vals.count() - 1);
                        vals.removeAt(vals.count() - 1);
                    }
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

void Waveform_Viewer_Widget::slot_xAxisChanged(const QCPRange &newRange) {
    if(!plot_re_scale) {
        QCPAxis *axis = qobject_cast<QCPAxis *>(QObject::sender());
        QCustomPlot *plot = axis->parentPlot();
        QCPRange limitRange(plot->property("xmin").toDouble(), plot->property("xmax").toDouble());
        limitAxisRange(axis, newRange, limitRange);
    }
}

void Waveform_Viewer_Widget::slot_yAxisChanged(const QCPRange &newRange) {
    if(!plot_re_scale) {
        QCPAxis *axis = qobject_cast<QCPAxis *>(QObject::sender());
        QCustomPlot *plot = axis->parentPlot();
        QCPRange limitRange(plot->property("ymin").toDouble(), plot->property("ymax").toDouble());
        limitAxisRange(axis, newRange, limitRange);
    }
}

void Waveform_Viewer_Widget::slot_re_translate() {
    ui->retranslateUi(this);
    set_ui_text();
}
