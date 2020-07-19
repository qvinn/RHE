#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QMainWindow>
    #include <QApplication>
    #include <QDir>
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

        private slots:
            void resizeEvent(QResizeEvent *) override;
            void on_button_login_logout_clicked();
            void on_button_register_clicked();

    private:
            Ui::MainWindow *ui;
            RegistrationWidget *ptr_registration_widg = nullptr;
            RHE_Widget *ptr_RHE_widg = nullptr;

    };

#endif // MAINWINDOW_H
