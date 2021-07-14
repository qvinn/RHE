#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QThread>
    #include <QMenuBar>
    #include <QComboBox>
    #include <QMainWindow>
    #include <QWidgetAction>
    #include "registration_widget.h"
    #include "rhe_widget.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class MainWindow;
    }
    QT_END_NAMESPACE

    class MainWindow : public QMainWindow {
        Q_OBJECT
        public:
            MainWindow(QWidget *parent = nullptr);
            ~MainWindow() override;

        private:
            void initialize_ui();
            void set_ui_text();
            void load_settings();
            void set_enable_login_buttons(bool flg);
            void login();
            void logout();
            void logined(bool flg);
            void data_updated(bool flg);

            Ui::MainWindow *ui;
            General_Widget *gen_widg = nullptr;
            Send_Receive_Module *snd_rcv_module;
            Data_Transfer_Module *data_transfer_module = nullptr;
            RegistrationWidget *ptr_registration_widg = nullptr;
            RHE_Widget *ptr_RHE_widg = nullptr;
            Waveform_Viewer_Widget *wvfrm_vwr = nullptr;
            QComboBox *cmbBx_lng_chs = nullptr;
            QWidgetAction *cmbBx_lng_chs_actn = nullptr;
            QTimer *tmr_waveform_viewer = nullptr;
            QTimer *tmr_progress_bar = nullptr;
            QList<QString> state_strs;
            QThread thread_data_trnsfr_mod;
            QThread thread_send_recv_mod;

            bool ui_initialized = false;
            bool language_changed = false;
            bool shw_at_cntr = true;

            int crrnt_state_strs = 1;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void moveEvent(QMoveEvent *) override;
            void closeEvent(QCloseEvent *) override;
            void on_pshBttn_login_logout_clicked();
            void on_pshBttn_register_clicked();
            void pshBttn_wvfrm_vwr();
            void pshBttn_exit();
            void chkBx_fls_chckng_state_changed();
            void chkBx_pins_chckng_state_changed();
            void chkBx_ld_mnl_frmwr_state_changed();
            void pshBttn_set_srvr_IP();
            void cmbBx_lng_chs_current_index_changed(int index);
            void slot_re_size();
            void slot_waveform_viewer_closed();
            void slot_timer_waveform_viewer_timeout();
            void slot_timer_progress_bar_timeout();

        signals:
            void update_data_signal();
            void set_disconnected_signal();      
    };

    //////////////////////////////////////////////////DIALOG SET SERVER IP///////////////////////////////////////////////////
    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Dialog_Set_Server_IP;
    }
    QT_END_NAMESPACE

    class Dialog_Set_Server_IP : public QDialog {
        Q_OBJECT
        public:
            Dialog_Set_Server_IP(General_Widget *widg, QWidget *parent = nullptr);
            ~Dialog_Set_Server_IP() override;

        private:
            void spinBox_roll_value(QSpinBox *spnBx, int val);
            void change_IP_item_focus(int val, int crrnt_item, int next_item);

            Ui::Dialog_Set_Server_IP *ui;
            General_Widget *gen_widg = nullptr;
            QList<QSpinBox *> *octt_lst = nullptr;

            bool ui_initialized = false;

        private slots:
            void on_pshBttn_ok_clicked();
            void on_pshBttn_cncl_clicked();
            void on_spnBx_frst_octt_valueChanged(int val);
            void on_spnBx_scnd_octt_valueChanged(int val);
            void on_spnBx_thrd_octt_valueChanged(int val);
            void on_spnBx_frth_octt_valueChanged(int val);
            void lineEdit_frst_octet_text_edited(const QString &val);
            void lineEdit_scnd_octet_text_edited(const QString &val);
            void lineEdit_thrd_octet_text_edited(const QString &val);
            void lineEdit_frth_octet_text_edited(const QString &val);
            void on_lnEdt_port_textEdited(const QString &val);
    };

#endif // MAINWINDOW_H
