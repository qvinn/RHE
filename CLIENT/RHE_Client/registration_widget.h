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

            bool login();
            bool register_user();

            QString get_user_fname();
            QString get_user_lname();

        private:
            Ui::RegistrationWidget *ui;
            General_Widget *gen_widg = nullptr;
            Send_Recieve_Module *snd_rcv_module = nullptr;
            QSettings *account_info = nullptr;
            QString user_lname;
            QString user_fname;

        public slots:
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;
    };

#endif // REGISTRATION_H
