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
            Waveform_Viewer_Widget(QWidget *parent = nullptr, General_Widget *widg = nullptr, bool stndln = false);
            ~Waveform_Viewer_Widget() override;

            void pshBttn_slct_dsplbl_pins_set_enabled(bool flg);
            void pshBttn_open_save_wvfrm_set_enabled(bool flg);
            void initialize_ui();
            void add_graphs_to_plot();
            QList<QCPGraph *>* get_graphs_list();
            void remove_graphs_from_plot();
            void re_scale_graph();
            void add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, QList<bool> *drw_lst = nullptr);
            void add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, QList<bool> *drw_lst = nullptr);
            void remove_data_from_graph();
            void add_pin_names(QString pin_name);
            QList<QString>* get_pin_names();
            int get_all_pins_count();
            void remove_pin_names();
            void change_pin_names();
            void add_saved_vals_list(int cnt);
            void remove_saved_vals_list();
            void add_data_to_saved_vals_list(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void remove_data_from_saved_vals_list();
            void remove_all_data();

            bool plot_re_scale = false;
            bool debugging = false;
            bool debug_show = true;

            int graph_count;

        private:
            void post_initialize_ui();
            void set_ui_text();
            void load_waveform();
            void save_waveform();
            void draw_data_on_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed, int i);
            void select_displayable_pins();
            void limit_axis_range(QCPAxis *axis, const QCPRange &new_range, const QCPRange &limit_range);
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
            QList<QList<int> *> *svd_vals = nullptr;
            QList<double> *svd_dbg_time = nullptr;

            double time_coef;
            double x_tckr_step;

            bool zoom_pressed = false;
            bool drag_pressed = false;
            bool drag_graph = false;
            bool language_changed = false;
            bool ui_initialized = false;
            bool add_names = true;
            bool standalone;

            int coef;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void leaveEvent(QEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void closeEvent(QCloseEvent *) override;
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
            void slot_x_axis_changed(const QCPRange &new_range);
            void slot_y_axis_changed(const QCPRange &new_range);

            void on_pshBttn_slct_dsplbl_pins_clicked();

    signals:
            void as_window_signal(bool as_window);
            void waveform_viewer_closed_signal();
    };

    //////////////////////////////////////////////////DIALOG SELECT DISPLAYABLE PINS///////////////////////////////////////////////////
    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Dialog_Select_Displayable_Pins;
    }
    QT_END_NAMESPACE

    class Dialog_Select_Displayable_Pins : public QDialog {
        Q_OBJECT
        public:
            Dialog_Select_Displayable_Pins(QStringList avlbl_pins, QStringList dsplbl_pins,  QWidget *parent = nullptr);
            ~Dialog_Select_Displayable_Pins() override;

            QStringList get_displayable_pins();
            QStringList get_available_pins();

        private:
            void replace_selected_pin(QStringListModel *recv_model, QStringListModel *sndr_model, QListView *recv_view, QListView *sndr_view, QPushButton *sndr_button);

            Ui::Dialog_Select_Displayable_Pins *ui;
            QStringListModel *available_pins_model;
            QStringListModel *displayable_pins_model;

        private slots:
            void on_pushButton_Ok_clicked();
            void on_pushButton_Cancell_clicked();
            void on_pushButton_Add_clicked();
            void on_pushButton_Del_clicked();
            void displayable_pins_selection_changed(const QItemSelection &sel);
            void available_pins_selection_changed(const QItemSelection &sel);
    };

#endif // WAVEFORM_VIEWER_WIDGET_H
