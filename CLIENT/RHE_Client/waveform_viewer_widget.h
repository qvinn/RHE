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
            void add_data_to_graph(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
            void add_data_to_graph_rltm(QList<int> val, QList<int> *prev_vals, double time, bool val_changed);
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
            bool as_window = false;
            bool shw_at_cntr = true;

            int graph_count;

        private:
            void post_initialize_ui();
            void set_ui_text();
            void set_measurement_label_text();
            void change_settings();
            void change_margin(int val);
            void load_waveform();
            void save_waveform();
            void select_diagram_settings();
            void select_displayable_pins();
            void draw_from_saved_vals(int val);
            void draw_line(QCPItemLine *line, double x_start, double x_end, double y_start, double y_end);
            void reset_measurement_data();
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
            QCPItemLine *frst_msr_line = nullptr;
            QCPItemLine *scnd_msr_line = nullptr;
            QCPItemLine *thrd_msr_line = nullptr;
            QCPItemText *curs_time = nullptr;
            QCPItemText *msr_time = nullptr;
            Qt::WindowFlags flags;
            QColor grph_clr;
            QColor grph_brsh_clr;
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
            bool measure = false;
            bool standalone;

            int count_of_press = 0;
            int coef;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void leaveEvent(QEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void closeEvent(QCloseEvent *) override;
            void on_chckBx_as_wndw_stateChanged(int state);
            void on_chckBx_attch_crsr_stateChanged(int state);
            void on_spnBx_wvfrm_vwr_dscrtnss_tm_valueChanged(int value);
            void on_cmbBx_wvfrm_vwr_dscrtnss_tm_tp_currentIndexChanged(int index);
            void on_pshBttn_chng_sttngs_clicked();
            void on_pshBttn_fl_scl_clicked();
            void on_pshBttn_clr_clicked();
            void on_pshBttn_slct_dsplbl_pins_clicked();
            void on_pshBttn_msr_toggled(bool checked);
            void on_pshBttn_open_save_wvfrm_clicked();
            void slot_mouse_move(QMouseEvent *event);
            void slot_mouse_pressed(QMouseEvent *event);
            void slot_mouse_unpressed(QMouseEvent *event);
            void slot_x_axis_changed(const QCPRange &new_range);
            void slot_y_axis_changed(const QCPRange &new_range);

        signals:
            void as_window_signal();
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
            void on_pshBttn_ok_clicked();
            void on_pshBttn_cncl_clicked();
            void on_pshBttn_add_clicked();
            void on_pshBttn_dlt_clicked();
            void displayable_pins_selection_changed(const QItemSelection &sel);
            void available_pins_selection_changed(const QItemSelection &sel);
    };

    //////////////////////////////////////////////////DIALOG SELECT DIAGRAM SETTINGS///////////////////////////////////////////////////
    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Dialog_Select_Diagram_Settings;
    }
    QT_END_NAMESPACE

    class Dialog_Select_Diagram_Settings : public QDialog {
        Q_OBJECT
        public:
            Dialog_Select_Diagram_Settings(QList<QString> _sttngs_lst, General_Widget *widg, QWidget *parent = nullptr);
            ~Dialog_Select_Diagram_Settings() override;

            QList<QString> get_diagram_settings();

        private:
            void change_color(int value);

            Ui::Dialog_Select_Diagram_Settings *ui;
            General_Widget *gen_widg = nullptr;
            QList<QPushButton*> *bttns_lst = nullptr;
            QList<QString> sttngs_lst;

        private slots:
            void on_pshBttn_axs_lbls_clr_clicked();
            void on_pshBttn_dgrm_grd_clr_clicked();
            void on_pshBttn_dgrm_bckgrnd_clr_clicked();
            void on_pshBttn_slctn_clr_clicked();
            void on_pshBttn_crsr_ln_clr_clicked();
            void on_pshBttn_crsr_tm_lbl_brdr_clr_clicked();
            void on_pshBttn_crsr_tm_lbl_fll_clr_clicked();
            void on_pshBttn_grph_clr_clicked();
            void on_spnBx_axs_lbls_fnr_sz_valueChanged(int value);
            void on_pshBttn_ok_clicked();
            void on_pshBttn_cncl_clicked();
    };

#endif // WAVEFORM_VIEWER_WIDGET_H
