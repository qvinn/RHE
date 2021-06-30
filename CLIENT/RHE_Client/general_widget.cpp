#include "general_widget.h"

General_Widget::General_Widget() {
#ifdef __linux__
    app_path.append(QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE")));        //Path Of Image Deployed with 'linuxdeployqt'
#endif
    if(app_path.count() == 0) {
        app_path.append(qApp->applicationDirPath());
    } else {
        QRegExp tagExp("/");
        QStringList lst = app_path.split(tagExp);
        app_path.clear();
        for(int i = 0; i < (lst.count() - 1); i++) {
            app_path.append("/" + QString(lst.at(i)).remove("/"));
        }
    }
    files_list = new QStringList();
    settings = new QSettings(app_path + "/settings.cfg", QSettings::IniFormat);
    create_base_settings();
#ifdef __linux__
    QString icon_path = (app_path + "/" + get_setting("settings/PATH_TO_DATA").toString() + "icon.png");
    check_is_icon_exist(icon_path);
    qApp->setWindowIcon(QIcon(icon_path));
#endif
    language_translator = new QTranslator();
    current_pos = new QPoint(-1, -1);
    palette.setColor(QPalette::WindowText, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Button, QColor(246, 246, 246, 255));
    palette.setColor(QPalette::Text, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::ButtonText, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Base, QColor(246, 246, 246, 255));
    palette.setColor(QPalette::Window, QColor(246, 246, 246, 255));
    palette.setColor(QPalette::Highlight, QColor(48, 140, 198, 255));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
    palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 255));
    QString sub_style_sheet = "background-color: lightblue; border-style: outset; color: black; font: bold 10p; border-style: outset; border-width: 1px; border-radius: 0px; border-color: blue; padding: 0px 0px;";
    style_sheet.append("pattern_1 { background-color: #F5F5F5; }"
                       "pattern_2"
                       "QPushButton:checked { " + sub_style_sheet + " }"
                       "QPushButton:pressed { " + sub_style_sheet + " }"
                       "QPushButton:disabled { " + sub_style_sheet + " }"
                       "QPushButton { background-color: lightGray; color: black; border-style: outset; border-width: 1px; border-radius: 0px; border-color: gray; padding: 0px 0px; }"
                       "QPushButton:hover { border-style: outset; border-width: 1px; border-radius: 0px; border-color: royalblue; }"
                       "QPushButton { min-width:73; min-height:21; }"
                      );
}

General_Widget::~General_Widget() {
    delete files_list;
    delete settings;
    delete current_pos;
    delete language_translator;
}

//-------------------------------------------------------------------------
// GET PATH TO CURRENT APPLICATION
//-------------------------------------------------------------------------
QString General_Widget::get_app_path() {
    return app_path;
}

//-------------------------------------------------------------------------
// GET POSITION FOR CURRENT ACTIVE WINDOW
//-------------------------------------------------------------------------
QPoint General_Widget::get_position() {
    return *current_pos;
}

//-------------------------------------------------------------------------
// SET POSITION FOR CURRENT ACTIVE WINDOW
//-------------------------------------------------------------------------
void General_Widget::set_position(QPoint widg_pos) {
    current_pos->setX(widg_pos.x());
    current_pos->setY(widg_pos.y());
}

//-------------------------------------------------------------------------
// LOAD VALUE OF SEETING
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// SAVE SEETING WITH VALUE
//-------------------------------------------------------------------------
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

//-------------------------------------------------------------------------
// CREATING OF INITIAL SETTING (IF SETTINGS-FILE OR CERTAIN SETTING DOESN'T EXIST)
//-------------------------------------------------------------------------
void General_Widget::create_base_settings() {
    check_setting_exist("settings/ENABLE_PINS_CHEKING", 1);
    check_setting_exist("settings/MANUALY_LOAD_FIRMWARE", 0);
    check_setting_exist("settings/LANGUAGE", 0);
    check_setting_exist("settings/CURRENT_BOARD", 0);
    check_setting_exist("settings/PATH_TO_DATA", "data/");
    check_setting_exist("settings/BOARDS_LIST_FILENAME", "Boards_List.xml");
    check_setting_exist("settings/SERVER_IP", "192.168.1.10");
    check_setting_exist("settings/SERVER_PORT", 3425);
    check_setting_exist("settings/VERSION", "1.0");
    check_setting_exist("settings/DEBUG_DISCRETENESS_TIME", 1);
    check_setting_exist("settings/DEBUG_DISCRETENESS_TIME_TYPE", 0);
    check_setting_exist("settings/START_DEBUG_AFTER_FPGA_FLASHING", 0);
    check_setting_exist("settings/START_SEQUENCE_OF_SIGNALS_WITH_DEBUG", 0);
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_ATTCH_CRSR", 0);
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_FIT_SIZE", 0);
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_DISCRETENESS_TIME", 1);
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE", 0);
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_AXIS_LABELS_COLOR", "#FFFFFFFF");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_DIAGRAM_GRID_COLOR", "#FF646464");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_DIAGRAM_BACKGROUND_COLOR", "#FF000000");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_SELECTION_COLOR", "#FFFF0000");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_CURSOR_LINE_COLOR", "#FFFFFF00");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_CURSOR_TIME_LABEL_BORDER_COLOR", "#FFFFFFFF");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_CURSOR_TIME_LABEL_FILL_COLOR", "#FFFFFF00");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_GRAPH_COLOR", "#FF00FF00");
    check_setting_exist("waveform_viewer_settings/WVFRM_VWR_AXIS_LABELS_FONT_SIZE", 10);
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_ATTCH_CRSR", 0);
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_DISCRETENESS_TIME", 1);
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE", 0);
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_AXIS_LABELS_COLOR", "#FFFFFFFF");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_DIAGRAM_GRID_COLOR", "#FF646464");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_DIAGRAM_BACKGROUND_COLOR", "#FF000000");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_SELECTION_COLOR", "#FFFF0000");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_CURSOR_LINE_COLOR", "#FFFFFF00");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_CURSOR_TIME_LABEL_BORDER_COLOR", "#FFFFFFFF");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_CURSOR_TIME_LABEL_FILL_COLOR", "#FFFFFF00");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_GRAPH_COLOR", "#FF00FF00");
    check_setting_exist("waveform_viewer_standalone_settings/WVFRM_VWR_AXIS_LABELS_FONT_SIZE", 10);
}

//-------------------------------------------------------------------------
// CHECK EXISTANCE OF CERTAIN SETTING IN SETTIGNS-FILE
//-------------------------------------------------------------------------
void General_Widget::check_setting_exist(QString type, QVariant val) {
    if(!settings->contains(type)) {
        save_setting(type, val);
    }
}

//-------------------------------------------------------------------------
// CHECK EXISTANCE OF FILE-ICON (FOR GNU/LINUX OLNY)
//-------------------------------------------------------------------------
void General_Widget::check_is_icon_exist(QString path) {
    QFile r_icon_file(path);
    if(!r_icon_file.exists()) {
        if(r_icon_file.open(QIODevice::WriteOnly)) {
            QFile i_icon_file(":/icons/1.png");
            if(i_icon_file.open(QIODevice::ReadOnly)) {
                QByteArray arr = i_icon_file.readAll();
                r_icon_file.write(arr, arr.size());
                i_icon_file.close();
            }
        }
        r_icon_file.close();
    }
}

//-------------------------------------------------------------------------
// GET STYLE_SHEET TO CUSTOMIZE SOME WINDOWS
//-------------------------------------------------------------------------
QString General_Widget::get_style_sheet(QString pattern_1, QString pattern_2) {
    QString str = style_sheet;
    return str.replace("pattern_1", pattern_1).replace("pattern_2", pattern_2);
}

//-------------------------------------------------------------------------
// LOAD FILE/FILES
//-------------------------------------------------------------------------
QStringList* General_Widget::load_files(QWidget *prnt, QString title, QString filter, bool files, bool path) {
    QFileDialog dialog(prnt, title, QDir::current().path(), filter);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.resize(800, 600);
    dialog.setOption(QFileDialog::HideNameFilterDetails);
    if(files) {
        dialog.setFileMode(QFileDialog::ExistingFiles);
    } else if(path) {
        dialog.setFileMode(QFileDialog::Directory);
    } else {
        dialog.setFileMode(QFileDialog::ExistingFile);
    }
    dialog.setStyleSheet(get_style_sheet("QFileDialog", "QFileDialog QLineEdit { background-color: #FFFFFF; color: #000000; }"
                                                        "QFileDialog QLabel { color: #000000; }"
                                                        "QFileDialog QToolButton { background-color: darkGrey; color: #000000; }"));
    QList<QWidget*> widgets = dialog.findChildren<QWidget*>();
    for(int i = 0; i < widgets.count(); i++) {
        widgets.at(i)->setPalette(palette);
    }
    if(dialog.exec() == QDialog::Accepted) {
        files_list->clear();
        files_list->append(dialog.selectedFiles());
    } else {
        return nullptr;
    }
    if(files_list->count() == 0) {
        return nullptr;
    }
    return files_list;
}

//-------------------------------------------------------------------------
// SAVE/APPEND/REWRITE FILE
//-------------------------------------------------------------------------
void General_Widget::save_file(QWidget *prnt, QString title, QString filter, QString *data, QString *file_name, bool re_write, bool fl_nm_exist) {
    QString fileName;
    if(fl_nm_exist) {
        fileName.append(*file_name);
    } else {
        QFileDialog dialog(prnt, title, QDir::current().path(), filter);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setOption(QFileDialog::HideNameFilterDetails);
        dialog.resize(800, 600);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setStyleSheet(get_style_sheet("QFileDialog", "QFileDialog QLineEdit { background-color: #FFFFFF; color: #000000; }"
                                                            "QFileDialog QLabel { color: #000000; }"
                                                            "QFileDialog QToolButton { background-color: darkGrey; color: #000000; }"));
        QList<QWidget*> widgets = dialog.findChildren<QWidget*>();
        for(int i = 0; i < widgets.count(); i++) {
            widgets.at(i)->setPalette(palette);
        }
        if(dialog.exec() == QDialog::Accepted) {
            fileName = dialog.selectedFiles().at(0);
            QStringList lst_1 = filter.split("*");
            QStringList lst_2 = lst_1.at(lst_1.count() - 1).split(")");
            if(!fileName.contains(lst_2.at(0))) {
                fileName.append(lst_2.at(0));
            }
        }
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

//-------------------------------------------------------------------------
// GET CHECKSUM OF FILE
//-------------------------------------------------------------------------
QByteArray General_Widget::get_file_checksum(QString file_path, QCryptographicHash::Algorithm hash_algorithm) {
    QFile file(file_path);
    if(file.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hash_algorithm);
        if(hash.addData(&file)) {
            return hash.result();
        }
    } else {
        qDebug() << "Cant open file";
    }
    return QByteArray();
}

//-------------------------------------------------------------------------
// GET PATH OF FILE LOADED WITH 'LOAD_FILES' METHOD
//-------------------------------------------------------------------------
QString General_Widget::load_file_path(QWidget *prnt, QString title, QString filter) {
    QStringList *lst = load_files(prnt, title, filter, false, false);
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

//-------------------------------------------------------------------------
// DISPLAYING WARNING/QUESTION/INFO MESSAGES
//-------------------------------------------------------------------------
int General_Widget::show_message_box(QString title, QString message, int type, QPoint position) {
    QMessageBox msgBox(QMessageBox::Warning, title, QString("\n").append(message));
    if(title.count() == 0) {
        msgBox.setWindowTitle(tr("Warning"));
    }
    msgBox.setParent(this);
    if(type == 1) {
        if(title.count() == 0) {
            msgBox.setWindowTitle(tr("Question"));
        }
        msgBox.setIcon(QMessageBox::Question);
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
    } else if(type == 2) {
        if(title.count() == 0) {
            msgBox.setWindowTitle(tr("Information"));
        }
        msgBox.setIcon(QMessageBox::Information);
    }
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setModal(true);
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
    msgBox.setStyleSheet(get_style_sheet("QMessageBox", "QMessageBox QLabel { color: #000000; }"));
    //hack to place message window at centre of window with 'position' coordinates
    msgBox.show();
    msgBox.hide();
    //end hack
    msgBox.move((position.x() - (msgBox.width() / 2)), (position.y() - (msgBox.height() / 2)));
    return msgBox.exec();
}

//-------------------------------------------------------------------------
// CHANGING LANGUAGE OF APPLICATION
//-------------------------------------------------------------------------
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
