#ifndef GENERAL_WIDGET_H
    #define GENERAL_WIDGET_H

    #include <QFile>
    #include <QIcon>
    #include <QDebug>
    #include <QWidget>
    #include <QSettings>
    #include <QTranslator>
    #include <QMessageBox>
    #include <QFileDialog>
    #include <QApplication>
    #include <QScreen>

    class General_Widget : public QWidget {
        Q_OBJECT
        public:
            General_Widget();
            ~General_Widget() override;

            QPoint get_position();
            void set_position(QPoint widg_pos);
            QVariant get_setting(QString type);
            void save_setting(QString type, QVariant val);
            QString get_style_sheet(QString pattern);
            QStringList* load_files(bool files, bool path, QString title, QString filter, QWidget *prnt);
            void save_file(QWidget *prnt = nullptr, QString title = "", QString filter = "",  QString *data = nullptr, QString *file_name = nullptr, bool re_write = false, bool fl_nm_exist = false);
            QString load_file_path(QString title, QString filter, QWidget *prnt);
            int show_message_box(QString str1, QString str2, int type, QPoint position);
            void change_current_locale();

        private:
            void create_base_settings();
            void check_setting_exist(QString type, QVariant val);
            void check_is_icon_exist(QString path);

            QStringList *files_list = nullptr;
            QSettings *settings = nullptr;
            QTranslator *language_translator = nullptr;
            QPoint *current_pos = nullptr;
            QString style_sheet = "";
            QLocale cur_locale;  

        signals:
            void re_translate_signal();
    };

#endif // GENERAL_WIDGET_H
