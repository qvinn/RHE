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
            void login();
            void logout();
            void logined(bool flg);

            Ui::MainWindow *ui;
            General_Widget *gen_widg = nullptr;
            Send_Recieve_Module *snd_rcv_module = nullptr;
            RegistrationWidget *ptr_registration_widg = nullptr;
            RHE_Widget *ptr_RHE_widg = nullptr;
            Waveform_Viewer_Widget *wvfrm_vwr = nullptr;
            QMenuBar *menu_bar = nullptr;
            QComboBox *cmbBx_lng_chs = nullptr;
            QAction *chkBx_fls_chckng_actn = nullptr;
            QAction *chkBx_pins_chckng_actn = nullptr;
            QAction *chkBx_ld_mnl_frmwr_actn = nullptr;
            QAction *wvfrm_vwr_actn = nullptr;
            QAction *ext_actn = nullptr;
            QMenu *menu_file = nullptr;
            QMenu *menu_settngs = nullptr;
            QWidgetAction *cmbBx_lng_chs_actn = nullptr;
            QTimer *tmr_waveform_viewer = nullptr;
            QTimer *tmr_progress_bar = nullptr;
            QThread thread;

            bool ui_initialized = false;
            bool language_changed = false;
            bool shw_at_cntr = true;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void moveEvent(QMoveEvent *) override;
            void closeEvent(QCloseEvent *) override;
            void on_button_login_logout_clicked();
            void on_button_register_clicked();
            void onPshBttnWvfrmVwr();
            void onPshBttnExt();
            void onChkBxFlsChckngStateChanged();
            void onChkBxPinsChckngStateChanged();
            void onChkBxLdMnlFrmwrStateChanged();
            void onCmbBxLngChsCurrentIndexChanged(int index);
            void slot_re_size();
            void slot_waveform_viewer_closed();
            void slot_timer_waveform_viewer_timeout();
            void slot_timer_progress_bar_timeout();

        signals:
            void set_disconnected_signal();
    };

#endif // MAINWINDOW_H
