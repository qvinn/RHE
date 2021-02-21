#include "general_widget.h"
#include "ui_waveform_viewer.h"

General_Widget::General_Widget() {
    files_list = new QStringList();
    QString path = qApp->applicationDirPath();
    path.append("/settings.cfg");
#ifdef __linux__
    settings = new QSettings(path, QSettings::NativeFormat);
#else
    settings = new QSettings(path, QSettings::IniFormat);
#endif
    create_base_settings();
    language_translator = new QTranslator();
}

General_Widget::~General_Widget() {
    delete files_list;
    delete settings;
    delete language_translator;
}

QVariant General_Widget::get_setting(QString type) {
    QRegExp tagExp("/");
    QStringList lst = type.split(tagExp);
    QVariant val;
    if(!settings->group().contains(lst.at(0))) {
        settings->beginGroup(lst.at(0));
        val.setValue(settings->value(lst.at(1)));
        settings->endGroup();
    } else {
        val.setValue(settings->value(lst.at(1)));
    }
    return val;
}

void General_Widget::save_setting(QString type, QVariant val) {
    QRegExp tagExp("/");
    QStringList lst = type.split(tagExp);
    if(!settings->group().contains(lst.at(0))) {
        settings->beginGroup(lst.at(0));
        settings->setValue(lst.at(1), val);
        settings->endGroup();
    } else {
        settings->setValue(lst.at(1), val);
    }
    settings->sync();
}

void General_Widget::create_base_settings() {
    check_setting_exist("settings/ENABLE_PINS_CHEKING", 1);
    check_setting_exist("settings/MANUALY_LOAD_FIRMWARE", 0);
    check_setting_exist("settings/LANGUAGE", 0);
    check_setting_exist("settings/CURRENT_BOARD", 0);
    check_setting_exist("settings/PATH_TO_DATA", "data/");
    check_setting_exist("settings/BOARDS_LIST_FILENAME", "Boards_List.xml");
    check_setting_exist("settings/SERVER_IP", "192.168.1.10");
    check_setting_exist("settings/VERSION", "1.0");
    check_setting_exist("settings/DEBUG_PINS_NUMBER", "10");
}

void General_Widget::check_setting_exist(QString type, QVariant val) {
    if(!settings->contains(type)) {
        save_setting(type, val);
    }
}

QStringList* General_Widget::load_files(bool files, bool path, QString title, QString filter) {
    QFileDialog *dialog = nullptr;
    dialog = new QFileDialog(nullptr, title, QDir::current().path(), filter);
    dialog->setOption(QFileDialog::HideNameFilterDetails);
    if(files) {
        dialog->setFileMode(QFileDialog::ExistingFiles);
    } else if(path) {
        dialog->setFileMode(QFileDialog::Directory);
    } else {
        dialog->setFileMode(QFileDialog::ExistingFile);
    }
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setModal(true);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    if(dialog->exec() == QDialog::Accepted) {
        files_list->clear();
        files_list->append(dialog->selectedFiles());
    } else {
        delete dialog;
        return nullptr;
    }
    if(files_list->count() == 0) {
        delete dialog;
        return nullptr;
    }
    delete dialog;
    return files_list;
}

//QFile* General_Widget::load_file(QString title, QString filter) {
//    fileList = load_files(false, false, title, filter);
//    if(fileList == nullptr) {
//        return nullptr;
//    }
//    QFile *file = nullptr;
//    for(int i = 0; i < fileList->count(); i++) {
//        QFileInfo fileInf(fileList->at(i));
//        file = new QFile(fileInf.absoluteFilePath());
//    }
//    return file;
//}

void General_Widget::save_file(QWidget *widg, QString title, QString filter, QString *data, QString *file_name, bool re_write, bool fl_nm_exist) {
    QString fileName;
    if(fl_nm_exist) {
        fileName.append(*file_name);
    } else {
        fileName = QFileDialog::getSaveFileName(widg, title, qApp->applicationDirPath(), filter);
#ifdef __linux__
        QStringList lst_1 = filter.split("*");
        QStringList lst_2 = lst_1.at(lst_1.count() - 1).split(")");
        if(!fileName.contains(lst_2.at(0))) {
            fileName.append(lst_2.at(0));
        }
#endif
    }
    if(!fileName.isEmpty()) {
        QFile *file = new QFile(fileName);
        if(!fl_nm_exist) {
            file_name->append(fileName);
            file->open(QFile::WriteOnly | QFile::Truncate);
            file->close();
        }
        if(re_write) {
            file->open(QFile::WriteOnly | QFile::Truncate);
            file->close();
        }
        if(file->open(QIODevice::Append)) {
            file->write(data->toLatin1().data());
        }
        file->close();
        delete file;
    }
}

QString General_Widget::load_file_path(QString title, QString filter) {
    QStringList *lst = load_files(false, false, title, filter);
    QString file_path = "";
    if(lst == nullptr) {
        return file_path;
    }
    for(int i = 0; i < lst->count(); i++) {
        QFileInfo fileInf(lst->at(i));
        file_path.append(fileInf.absoluteFilePath());
    }
    return file_path;
}

int General_Widget::show_message_box(QString str1, QString str2, int type) {
    QMessageBox msgBox(QMessageBox::Warning, str1, str2);
    if(str1.count() == 0) {
        msgBox.setWindowTitle(tr("Warning"));
    }
    msgBox.setParent(this);
    if(type == 1) {
        if(str1.count() == 0) {
            msgBox.setWindowTitle(tr("Question"));
        }
        msgBox.setIcon(QMessageBox::Question);
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
    } else if(type == 2) {
        if(str1.count() == 0) {
            msgBox.setWindowTitle(tr("Information"));
        }
        msgBox.setIcon(QMessageBox::Information);
    }
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setModal(true);
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
    return msgBox.exec();
}

void General_Widget::change_current_locale() {
    if(get_setting("settings/LANGUAGE").toInt() == 0) {
        language_translator->load(":/Language/RHE_Client_en.qm");
        cur_locale = QLocale("en_US");
    } else if(get_setting("settings/LANGUAGE").toInt() == 1) {
        language_translator->load(":/Language/RHE_Client_ua.qm");
        cur_locale = QLocale("uk_UA");
    } else if(get_setting("settings/LANGUAGE").toInt() == 2) {
        language_translator->load(":/Language/RHE_Client_ru.qm");
        cur_locale = QLocale("ru_RU");
    }
    QLocale::setDefault(cur_locale);
    qApp->installTranslator(language_translator);
    emit re_translate_signal();
}

//////////////////////////////////////////////////WAVEFORM VIEWER///////////////////////////////////////////////////
Waveform_Viewer_Widget::Waveform_Viewer_Widget(QWidget* parent, General_Widget *widg, bool stndln) : QWidget(parent), ui(new Ui::Waveform_Viewer) {
    ui->setupUi(this);
    gen_widg = widg;
    standalone = stndln;
    if(standalone) {
        ui->pshBttn_open_save_wvfrm->setText(tr("Open waveform"));
    } else {
        ui->pshBttn_open_save_wvfrm->setText(tr("Save waveform"));
    }
    ui->chckBx_as_wndw->setVisible(!stndln);
    this->setWindowTitle(tr("Waveform Viewer"));
    textTicker = new QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText());
    connect(ui->diagram->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_xAxisChanged(QCPRange)));
    connect(ui->diagram->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_yAxisChanged(QCPRange)));
    graph_list = new QList<QCPGraph *>();
    flags = this->windowFlags();
}

Waveform_Viewer_Widget::~Waveform_Viewer_Widget() {
    remove_graphs_form_plot();
    delete graph_list;
    delete textTicker;
    delete ui;
}

void Waveform_Viewer_Widget::showEvent(QShowEvent *) {
    resizeEvent(nullptr);
}

void Waveform_Viewer_Widget::resizeEvent(QResizeEvent *) {
    ui->verticalLayoutWidget->resize(this->width(), this->height());
    ui->diagram->resize(ui->verticalLayoutWidget->width(), ui->verticalLayoutWidget->height() - ui->horizontalLayout->geometry().height());
}

void Waveform_Viewer_Widget::on_pshBttn_fl_scl_clicked() {
    re_scale_graph();
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
    if(!standalone) {
        if(graph_count != 0) {
            int cnt = ui->diagram->graph(0)->data()->size();
            QString fn_nm = "";
            QString str = "";
            for(int i = 0; i < cnt; i++) {
                str.clear();
                for(int k = 0; k < graph_count; k++) {
                    if(k == 0) {
                        str.append(QString::number(ui->diagram->graph(k)->data()->at(i)->key) + "/");
                    }
                    str.append(QString::number(ui->diagram->graph(k)->data()->at(i)->value));
                    if(k != (graph_count - 1)) {
                        str.append("/");
                    } else {
                        str.append("\n");
                    }
                }
                gen_widg->save_file(this, tr("Saving waveform"), tr("Waveform (*.wvfrm)"), &str, &fn_nm, false, static_cast<bool>(i));
            }
        }
    } else {
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
            }
            re_scale_graph();
            add_graphs_to_plot();
            plot_re_scale = false;
            re_scale_graph();
            if(file->open(QIODevice::ReadOnly)) {
                QTextStream in(file);
                while(!in.atEnd()) {
                    QString str_dat = in.readLine();
                    str_dat.replace("\n", "");
                    QRegExp tagExp("/");
                    QStringList lst_dat = str_dat.split(tagExp);
                    for(int i = 0; i < lst_dat.count(); i++) {
                        if(i == 0) {
                            debug_time = lst_dat.at(i).toDouble();
                        } else {
                            vals.append(lst_dat.at(i).toInt());
                        }
                    }
                    add_data_to_graph(vals, &prev_vals, debug_time, false, false);
                    vals.clear();
                }
                file->close();
            }
        }
        delete file;
        re_scale_graph();
    }
}

void Waveform_Viewer_Widget::pshBttn_open_save_wvfrm_set_enabled(bool flg) {
    ui->pshBttn_open_save_wvfrm->setEnabled(flg);
}

void Waveform_Viewer_Widget::initialize_ui() {
    ui->diagram->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
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
    re_scale_graph();
    add_graphs_to_plot();
}

void Waveform_Viewer_Widget::add_graphs_to_plot() {
    for(int i = 0; i < graph_count; i++) {
        QString tmp = "pin ";
        if((i + 1) < 10) {
            tmp.append(QString::number(0));
        }
        graph_list->append(ui->diagram->addGraph());
        graph_list->at(i)->setPen(QPen(QColor(0, 255, 0), 1));
        (*textTicker)->addTick((i * 2 + 1), tmp.append(QString::number(i + 1)));
    }
    ui->diagram->yAxis->setTicker(*textTicker);
}

void Waveform_Viewer_Widget::remove_graphs_form_plot() {
    (*textTicker)->clear();
    ui->diagram->yAxis->ticker().clear();
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

void Waveform_Viewer_Widget::add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, bool val_binary) {
    for(int i = 0; i < graph_list->count(); i++) {
        if(val_binary) {
            if(val_changed) {
                graph_list->at(i)->addData(time, (1 + (i * 2) + prev_vals->at(i)));
            }
            graph_list->at(i)->addData(time, (1 + (i * 2) + val.at(i)));
        } else {
            if(val_changed) {
                graph_list->at(i)->addData(time, prev_vals->at(i));
            }
            graph_list->at(i)->addData(time, val.at(i));
        }
        prev_vals->replace(i, val.at(i));
    }
}

void Waveform_Viewer_Widget::add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, bool val_binary) {
    add_data_to_graph(val, prev_vals, time, val_changed, val_binary);
    ui->diagram->replot();
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
