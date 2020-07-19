#include "rhe_widget.h"
#include "ui_rhe_widget.h"

RHE_Widget::RHE_Widget(QWidget *parent) : QWidget(parent), ui(new Ui::RHE_Widget)  {
    ui->setupUi(this);
    this->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    srm = new Send_Recieve_Module();
}

RHE_Widget::~RHE_Widget() {
    delete ui;
    delete srm;
}

void RHE_Widget::on_pushButton_clicked() {
    QString str = ui->textEdit->toPlainText();
    if(str.length() != 0) {
        srm->send_data(new QByteArray(str.toLatin1()));
    }
}

void RHE_Widget::set_fname_lname(QString str) {
    ui->labe_FName_LName->setText(tr("Hello, ") + str);
}
