#ifndef REGISTRATIONWIDGET_H
    #define REGISTRATIONWIDGET_H

    #include <QRegularExpression>
    #include "general_widget.h"
    #include "send_recieve_module.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RegistrationWidget;
    }
    QT_END_NAMESPACE

    class RegistrationWidget : public QWidget {
        Q_OBJECT
        public:
            RegistrationWidget(QWidget *parent = nullptr, General_Widget *widg = nullptr, Send_Recieve_Module *snd_rcv_mod = nullptr);
            ~RegistrationWidget() override;

            void login();
            bool register_user();

            QString get_user_fname();
            QString get_user_lname();

        private:
            void link_established(bool flg);
            void id_received(int state);

            Ui::RegistrationWidget *ui;
            General_Widget *gen_widg = nullptr;
            QSettings *account_info = nullptr;
            QString user_lname;
            QString user_fname;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;

        signals:
            void init_connection_signal();
            void get_id_for_client_signal();
            void logined(bool flg);
    };

#endif // REGISTRATION_H
