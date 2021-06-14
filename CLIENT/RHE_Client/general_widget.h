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
    #include <QCryptographicHash>

    class General_Widget : public QWidget {
        Q_OBJECT
        public:
            General_Widget();
            ~General_Widget() override;

            QPoint get_position();

            /*
             * QPoint widg_pos - coordinates(x, y) of current active window
             */
            void set_position(QPoint widg_pos);

            /*
             * QString type - name of needed setting
             */
            QVariant get_setting(QString type);

            /*
             * QString type - name of needed setting
             * QVariant val - value of needed setting
             */
            void save_setting(QString type, QVariant val);

            /*
             * QString pattern - name of window class which needed to be customize
             */
            QString get_style_sheet(QString pattern);

            /*
             * QWidget *prnt - pointer to parental widget
             * QString title - title of file-loading dialog
             * QString filter - needed file extension
             * bool files - if true -> loading multiple files
             * bool path - if true -> choosing-directory mode
             */
            QStringList* load_files(QWidget *prnt, QString title, QString filter, bool files, bool path);

            /*
             * QWidget *prnt - pointer to parental widget
             * QString title - title of file-saving dialog
             * QString filter - needed file extension
             * QString *data - pointer to saved data
             * QString *file_name - pointer to saved file-name
             * bool re_write - if true -> rewrite exiting file
             * bool fl_nm_exist - if true -> appending existing file
             */
            void save_file(QWidget *prnt = nullptr, QString title = "", QString filter = "", QString *data = nullptr, QString *file_name = nullptr, bool re_write = false, bool fl_nm_exist = false);

            /*
             * QWidget *prnt - pointer to parental widget
             * QString title - title of file-loading dialog
             * QString filter - needed file extension
             */
            QString load_file_path(QWidget *prnt, QString title, QString filter);

            /*
             * QString file_path - path to file, which checksum will be calculated
             * QCryptographicHash::Algorithm hash_algorithm - type of algorithm for checksum
             */
            QByteArray get_file_checksum(QString file_path, QCryptographicHash::Algorithm hash_algorithm);

            /*
             * QString title - title of message dialog
             * QString message - text of message
             * QPoint position - coordinates(x, y) of current active window
             */
            int show_message_box(QString title, QString message, int type, QPoint position);

            void change_current_locale();

        private:
            void create_base_settings();

            /*
             * QString type - name of needed setting
             * QVariant val - value of needed setting
             */
            void check_setting_exist(QString type, QVariant val);

            /*
             * QString path - path to file-icon
             */
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
