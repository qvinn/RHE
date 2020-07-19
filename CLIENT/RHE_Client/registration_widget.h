#ifndef REGISTRATIONWIDGET_H
    #define REGISTRATIONWIDGET_H

    #include <QWidget>
    #include <QSettings>
    #include <QMessageBox>
    #include <QRegularExpression>

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RegistrationWidget;
    }
    QT_END_NAMESPACE

    class RegistrationWidget : public QWidget {
        Q_OBJECT
        public:
            RegistrationWidget(QWidget *parent = nullptr);
            ~RegistrationWidget();

            bool login();
            bool register_user();

            QString get_user_fname();
            QString get_user_lname();

        private:
            Ui::RegistrationWidget *ui;
            QSettings *account_info = nullptr;
            QString user_lname;
            QString user_fname;

    };

#endif // REGISTRATION_H
