#ifndef GENERAL_WIDGET_H
    #define GENERAL_WIDGET_H

    #include <QFile>
    #include <QFlags>
    #include <QDebug>
    #include <QWidget>
    #include <QSettings>
    #include <QTranslator>
    #include <QMessageBox>
    #include <QFileDialog>
    #include <QApplication>
    #include "qcustomplot.h"

    class General_Widget : public QWidget {
        Q_OBJECT
        public:
            General_Widget();
            ~General_Widget() override;

            QVariant get_setting(QString type);
            void save_setting(QString type, QVariant val);
            QStringList* load_files(bool files, bool path, QString title, QString filter);
//            QFile* load_file(QString title, QString filter);
            void save_file(QWidget *widg = nullptr, QString title = "", QString filter = "",  QString *data = nullptr, QString *file_name = nullptr, bool re_write = false, bool fl_nm_exist = false);
            QString load_file_path(QString title, QString filter);
            int show_message_box(QString str1, QString str2, int type);
            void change_current_locale();

        private:
            void create_base_settings();
            void check_setting_exist(QString type, QVariant val);

            QStringList *files_list = nullptr;
            QSettings *settings = nullptr;
            QTranslator *language_translator = nullptr;
            QLocale cur_locale;

        signals:
            void re_translate_signal();
    };

    //////////////////////////////////////////////////WAVEFORM VIEWER///////////////////////////////////////////////////
    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Waveform_Viewer;
    }
    QT_END_NAMESPACE

    class Waveform_Viewer_Widget : public QWidget {
        Q_OBJECT
        public:
            Waveform_Viewer_Widget(QWidget* parent = nullptr, General_Widget *widg = nullptr, bool stndln = false);
            ~Waveform_Viewer_Widget();

            void pshBttn_open_save_wvfrm_set_enabled(bool flg);
            void initialize_ui();
            void add_graphs_to_plot();
            void remove_graphs_form_plot();
            void re_scale_graph();
            void add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void remove_data_from_graph();

            bool plot_re_scale = false;
            int graph_count;

        private:
            void limitAxisRange(QCPAxis *axis, const QCPRange &newRange, const QCPRange &limitRange);

            Ui::Waveform_Viewer *ui;
            General_Widget *gen_widg = nullptr;
            QList<QCPGraph *> *graph_list = nullptr;
            QSharedPointer<QCPAxisTickerText> *textTicker = nullptr;
            QCPItemLine *curs_ver_line = nullptr;
            QCPItemText *curs_time = nullptr;
            Qt::WindowFlags flags;

            bool zoom_pressed = false;
            bool standalone;

        private slots:
            void showEvent(QShowEvent *);
            void resizeEvent(QResizeEvent *);
            void on_pshBttn_fl_scl_clicked();
            void on_pshBttn_clr_clicked();
            void on_chckBx_as_wndw_stateChanged(int state);
            void on_pshBttn_open_save_wvfrm_clicked();
            void slot_mouse_move(QMouseEvent *event);
            void slot_mouse_pressed(QMouseEvent *event);
            void slot_mouse_unpressed(QMouseEvent *event);
            void slot_xAxisChanged(const QCPRange &newRange);
            void slot_yAxisChanged(const QCPRange &newRange);

        signals:
            void as_window_signal(bool as_window);
    };

#endif // GENERAL_WIDGET_H
