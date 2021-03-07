#include "general_widget.h"

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
    check_setting_exist("settings/SERVER_PORT", 3425);
    check_setting_exist("settings/VERSION", "1.0");
    check_setting_exist("settings/DEBUG_PINS_NUMBER", "10");
    check_setting_exist("settings/DEBUG_DISCRETENESS_TIME", 1);
    check_setting_exist("settings/DEBUG_DISCRETENESS_TIME_TYPE", 0);
    check_setting_exist("settings/WVFRM_VWR_ATTCH_CRSR", 0);
    check_setting_exist("settings/WVFRM_VWR_DISCRETENESS_TIME", 1);
    check_setting_exist("settings/WVFRM_VWR_DISCRETENESS_TIME_TYPE", 0);
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
