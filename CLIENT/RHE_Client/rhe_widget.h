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
            void pshBttn_chs_frmwr_set_visible(bool flag);
            void pshBttn_chk_prj_stat_set_enabled(bool flag);
            void pshBttn_chs_frmwr_set_enabled(bool flag);
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
            void change_cnt_of_dbg_pins(int value);
            void change_board_pixmap();
            void set_enable_board_power_led(bool flg);
            void set_button_state_debug(bool flg);
            void set_buttons_state_enabled(bool flg);
            void add_horizontal_spacer();
            void remove_horizontal_spacer();
            void check_is_proj_folder(bool folder_exist);
            bool check_fpga_connections(QString path_to_fit_rprtr);

            /*
             * bool read_board_params - flag, which indicates that be reads parameters needed only to check pins
             * QString *cur_fpga - pointer to string, in which writes fpga-name from xml-file
             * QList<QString> *pins_numb - pointer to list, in which writes pins numbers from xml-file
             * QList<QString> *pins_typ - pointer to list, in which writes pins types from xml-file
             * QList<QString> *pins_io_stndrt - pointer to list, in which writes pins I/O standarts from xml-file
             */
            bool read_xml_file(bool read_board_params, QString *cur_fpga = nullptr, QList<QString> *pins_numb = nullptr, QList<QString> *pins_typ = nullptr, QList<QString> *pins_io_stndrt = nullptr);
            void add_data_to_qpoint(QList<QPoint> *lst, int val, bool is_x);

            Ui::RHE_Widget *ui;
            General_Widget *gen_widg = nullptr;
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
            QList<int> *pi_pins_nums = nullptr;
            QList<QHBoxLayout *> *inpt_hbxs = nullptr;
            QList<QLabel *> *inpt_lbls = nullptr;
            QList<QSpacerItem *> *inpt_spcrs = nullptr;
            QList<QSlider *> *inpt_sldrs = nullptr;
            QList<QLCDNumber *> *inpt_stts = nullptr;
            QTimer *send_file_status = nullptr;
            QSpacerItem *hrzntl_spcr = nullptr;
            QList<QString> state_strs;
            QString lname_fname;
            QPixmap pixmp_brd;
            QByteArray input_pins_states;

            bool qpf_exist = false;
            bool fit_exist = false;
            bool path_exist = false;
            bool csv_exist = false;
            bool csv_sended = false;
            bool ui_initialized = false;
            bool language_changed = false;
            bool board_is_on = false;
            bool clr_trnsprnt = true;
            bool dbg_strtd = false;
            bool sqnc_of_sgnls_strtd = false;

            int prev_board_index = -1;
            int crrnt_state_strs = 7;
            int dbg_tm_tp_lmt = 0;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void paintEvent(QPaintEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void on_pshBttn_strt_dbg_clicked();
            void on_pshBttn_stp_dbg_clicked();
            void on_chckBx_strt_dbg_aftr_flsh_stateChanged(int state);
            void on_pshBttn_chs_sgnls_sqnc_clicked();
            void on_pshBttn_snd_sgnls_sqnc_clicked();
            void on_pshBttn_strt_sgnls_sqnc_clicked();
            void on_chckBx_strt_sqnc_of_sgn_with_dbg_stateChanged(int state);
            void on_cmbBx_chs_brd_currentIndexChanged(int index);
            void on_spnBx_dbg_tm_valueChanged(int value);
            void on_cmbBx_dbg_tm_tp_currentIndexChanged(int index);
            void on_pshBttn_set_path_to_proj_clicked();
            void on_pshBttn_snd_frmwr_clicked();
            void on_pshBttn_chk_prj_stat_clicked();
            void on_pshBttn_chs_frmwr_clicked();
            void pshBttn_strt_sgnls_sqnc_set_enabled(bool flag);
            void scrll_area_sgnls_set_enabled(bool flag);
            void slot_input_val_changed(int val);
            void slot_slider_pressed();
            void slot_choose_board(QString jtag_code);
            void slot_accept_board(bool flg);
            void slot_accept_debug_time_limit(int time, int time_type);
            void slot_accept_debug_data(QByteArray debug_data);
            void slot_accept_input_data_table(QByteArray input_data_table);
            void slot_accept_output_data_table(QByteArray output_data_table);
            void slot_firmware_file_sended();
            void slot_sequence_of_signals_file_sended(bool flg);
            void slot_fpga_flashed();
            void slot_end_sequence_of_signals();
            void slot_as_window();
            void slot_timer_timeout();

        signals:
            void resize_signal();
            void set_FPGA_id_signal(QString jtag_id_code);
            void flash_FPGA_signal();
            void send_file_to_ss_signal(QByteArray File_byteArray, int strt_sndng_val, int cntns_sndng_val, int end_sndng_val);
            void start_debug_signal(quint16 dscrt_tm, quint8 dscrt_tm_tp, int flag);
            void stop_debug_signal();
            void start_sequence_of_signals_signal();
            void send_swtches_states_signal(QByteArray arr);
    };

#endif // RHE_WIDGET_H
