#ifndef GENERAL_WIDGET_H
    #define GENERAL_WIDGET_H

    #include <QFile>
    #include <QDebug>
    #include <QWidget>
    #include <QSettings>
    #include <QTranslator>
    #include <QMessageBox>
    #include <QFileDialog>
    #include <QApplication>

    class General_Widget : public QObject {
        Q_OBJECT
        public:
            General_Widget();
            ~General_Widget() override;

            void create_base_settings();
            QVariant get_setting(QString type);
            void save_setting(QString type, QVariant val);
            QStringList* load_files(bool files, bool path, QString title, QString filter);
//            QFile* load_file(QString title, QString filter);
            QString load_file_path(QString title, QString filter);
            int show_message_box(QString str1, QString str2, int type);
            void change_current_locale();

        private:
            QStringList *files_list = nullptr;
            QSettings *settings = nullptr;
            QTranslator *language_translator = nullptr;
            QLocale cur_locale;

        signals:
            void re_translate_signal();
    };

#endif // GENERAL_WIDGET_H
