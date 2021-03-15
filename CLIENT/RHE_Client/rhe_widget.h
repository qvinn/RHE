#ifndef RHE_WIDGET_H
    #define RHE_WIDGET_H

    #include <QDir>
    #include <QPainter>
    #include <QDirIterator>
    #include <QResizeEvent>
    #include <QXmlStreamReader>
    #include "waveform_viewer_widget.h"
    #include "send_recieve_module.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RHE_Widget;
    }
    QT_END_NAMESPACE

    class RHE_Widget : public QWidget {
        Q_OBJECT
        public:
            RHE_Widget(QWidget *parent = nullptr, General_Widget *widg = nullptr, Send_Recieve_Module *snd_rcv_mod = nullptr);
            ~RHE_Widget() override;

            void pshBttn_set_path_to_proj_set_visible(bool flag);
            void pshBttn_chk_prj_stat_set_visible(bool flag);
            void pshBttn_ld_frmwr_set_visible(bool flag);
            void pshBttn_chk_prj_stat_set_enabled(bool flag);
            void pshBttn_ld_frmwr_set_enabled(bool flag);
            void pshBttn_snd_frmwr_set_enabled(bool flag);
            void set_fname_lname(QString str);
            void initialize_ui();

            bool sof_exist = false;
            bool svf_exist = false;
            bool pins_chk = false;

        private:
            void pre_initialize_ui();
            void post_initialize_ui();
            void set_ui_text();
            void change_board_pixmap();
            void set_button_state_debug(bool flg);
            void check_is_proj_folder(bool folder_exist);
            bool check_fpga_connections(QString path_to_fit_rprtr);
            bool read_xml_file(bool type, QString *cur_fpga = nullptr, QList<QString> *pins_numb = nullptr, QList<QString> *pins_typ = nullptr, QList<QString> *pins_io_stndrt = nullptr);
            void add_data_to_qpoint(QList<QPoint> *lst, int val, bool is_x);

            Ui::RHE_Widget *ui;
            General_Widget *gen_widg = nullptr;
            Send_Recieve_Module *snd_rcv_module = nullptr;
            Waveform_Viewer_Widget *wvfrm_vwr = nullptr;
            QString *path_to_proj = nullptr;
            QString *prev_path_to_proj = nullptr;
            QFile *svf_file = nullptr;
            QFile *csv_file = nullptr;
            QList<QString> *pixmp_names = nullptr;
            QList<QString> *jtag_id_codes = nullptr;
            QList<QString> *led_colors = nullptr;
            QList<QPoint> *led_x_y = nullptr;
            QList<QPoint> *led_width_height = nullptr;
            QList<int> *prev_vals = nullptr;
            QList<QHBoxLayout*> *inpt_hbxs = nullptr;
            QList<QLabel*> *inpt_lbls = nullptr;
            QList<QSpacerItem*> *inpt_spcrs = nullptr;
            QList<QSlider*> *inpt_sldrs = nullptr;
            QList<QLCDNumber*> *inpt_stts = nullptr;
            QString lname_fname;
            QPixmap pixmp_brd;

            bool qpf_exist = false;
            bool fit_exist = false;
            bool path_exist = false;
            bool ui_initialized = false;
            bool language_changed = false;
            bool board_is_on = false;
            bool new_debug = false;
            bool clr_trnsprnt = true;

            int prev_board_index;
            int dbg_tm_tp_lmt = 0;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void paintEvent(QPaintEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void on_pushButton_clicked();
            void on_pushButton_2_clicked();
            void on_pushButton_3_clicked();
            void on_pushButton_strt_drw_clicked();
            void on_pushButton_stp_drw_clicked();
            void on_pshBttn_chs_sgnls_sqnc_clicked();
            void on_pshBttn_snd_sgnls_sqnc_clicked();
            void on_pshBttn_strt_sgnls_sqnc_clicked();
            void on_cmbBx_chs_brd_currentIndexChanged(int index);
            void on_hrzntlSldr_cnt_dbg_pins_valueChanged(int value);
            void on_spnBx_dbg_tm_valueChanged(int value);
            void on_cmbBx_dbg_tm_tp_currentIndexChanged(int index);
            void on_pshBttn_set_path_to_proj_clicked();
            void on_pshBttn_snd_frmwr_clicked();
            void on_pshBttn_chk_prj_stat_clicked();
            void on_pshBttn_ld_frmwr_clicked();
            void pshBttn_strt_sgnls_sqnc_set_enabled(bool flag);
            void slot_input_val_changed(int val);
            void slot_choose_board(QString jtag_code);
            void slot_accept_board(bool flg);
            void slot_accept_debug_time_limit(int time, int time_type);
            void slot_accept_debug_data(QByteArray debug_data);
            void slot_accept_input_data_table(QByteArray input_data_table);
            void slot_accept_output_data_table(QByteArray output_data_table);
            void slot_as_window(bool as_window); 

        signals:
            void resize_signal();
    };

#endif // RHE_WIDGET_H
