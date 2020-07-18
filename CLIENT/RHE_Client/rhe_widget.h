#ifndef RHE_WIDGET_H
    #define RHE_WIDGET_H

    #include <QWidget>
    #include "send_recieve_module.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RHE_Widget;
    }
    QT_END_NAMESPACE

    class RHE_Widget : public QWidget {
        Q_OBJECT
        public:
            RHE_Widget(QWidget *parent = nullptr);
            ~RHE_Widget();

            void set_fname_lname(QString str);

        private:
                Ui::RHE_Widget *ui;
                Send_Recieve_Module *srm = nullptr;

        private slots:
                void on_pushButton_clicked();

    };

#endif // RHE_WIDGET_H
