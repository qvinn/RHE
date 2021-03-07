#ifndef WAVEFORM_VIEWER_WIDGET_H
    #define WAVEFORM_VIEWER_WIDGET_H

    #include "general_widget.h"
    #include "qcustomplot.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Waveform_Viewer;
    }
    QT_END_NAMESPACE

    class Waveform_Viewer_Widget : public QWidget {
        Q_OBJECT
        public:
            Waveform_Viewer_Widget(QWidget* parent = nullptr, General_Widget *widg = nullptr, bool stndln = false);
            ~Waveform_Viewer_Widget() override;

            void pshBttn_open_save_wvfrm_set_enabled(bool flg);
            void initialize_ui();
            void add_graphs_to_plot();
            void remove_graphs_form_plot();
            void re_scale_graph();
            void add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void remove_data_from_graph();
            void add_pin_names(QString pin_name);
            QList<QString>* get_pin_names();
            void remove_pin_names();
            void change_pin_names();

            bool plot_re_scale = false;
            bool debugging = false;

            int graph_count;

        private:
            void post_initialize_ui();
            void set_ui_text();
            void load_waveform();
            void save_waveform();
            void limitAxisRange(QCPAxis *axis, const QCPRange &newRange, const QCPRange &limitRange);
            bool mouse_inside_object(double x_coord, double y_coord, bool graph_drawing_rect);

            Ui::Waveform_Viewer *ui;
            General_Widget *gen_widg = nullptr;
            QList<QCPGraph *> *graph_list = nullptr;
            QList<QString> *pin_names = nullptr;
            QList<QString> *pin_names_board = nullptr;
            QSharedPointer<QCPAxisTickerText> *textTicker = nullptr;
            QSharedPointer<QCPAxisTickerFixed> *dyn_tckr = nullptr;
            QCPItemLine *curs_ver_line = nullptr;
            QCPItemText *curs_time = nullptr;
            Qt::WindowFlags flags;

            double time_coef;
            double x_tckr_step;

            bool zoom_pressed = false;
            bool drag_pressed = false;
            bool drag_graph = false;
            bool language_changed = false;
            bool ui_initialized = false;
            bool standalone;

            int coef;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void leaveEvent(QEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void on_chckBx_attch_crsr_stateChanged(int state);
            void on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(int value);
            void on_cmbBx_wvfrm_vwr_dscrtnss_tm_tp_currentIndexChanged(int index);
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

#endif // WAVEFORM_VIEWER_WIDGET_H
