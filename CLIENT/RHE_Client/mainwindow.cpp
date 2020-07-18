#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    QString ApplicationDataPath;
    ApplicationDataPath.append(qApp->applicationDirPath());
    QDir::setCurrent(QDir::toNativeSeparators(qApp->applicationDirPath()));
    ui->horizontalLayoutWidget->setGeometry(0, (this->height() - ui->horizontalLayoutWidget->height()), this->width(), ui->horizontalLayoutWidget->height());
    ui->stackedWidget->setGeometry(0, 0, this->width(), (this->height() - ui->horizontalLayoutWidget->height()));
    ptr_registration_widg = new RegistrationWidget(this);
    ptr_RHE_widg = new RHE_Widget(this);
    ui->stackedWidget->addWidget(ptr_registration_widg);
    ui->stackedWidget->addWidget(ptr_RHE_widg);
    ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
}

MainWindow::~MainWindow() {
    delete ptr_registration_widg;
    delete ptr_RHE_widg;
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *) {
    ui->stackedWidget->setGeometry(0, 0, this->width(), (this->height() - ui->horizontalLayoutWidget->height()));
    ui->horizontalLayoutWidget->setGeometry(0, (this->height() - ui->horizontalLayoutWidget->height()), this->width(), ui->horizontalLayoutWidget->height());
}

void MainWindow::on_button_login_logout_clicked() {
    if(ui->stackedWidget->currentWidget() == ptr_registration_widg) {
        if(ptr_registration_widg->login()) {
            ptr_RHE_widg->set_fname_lname(ptr_registration_widg->get_user_fname() + " " + ptr_registration_widg->get_user_lname());
            ui->button_register->hide();
            ui->stackedWidget->setCurrentWidget(ptr_RHE_widg);
            ui->button_login_logout->setText(tr("Logout"));
        }
    } else {
        ui->button_login_logout->setText(tr("Login"));
        ui->button_register->show();
        ui->stackedWidget->setCurrentWidget(ptr_registration_widg);
    }
}

void MainWindow::on_button_register_clicked() {
    if(ptr_registration_widg->register_user()) {
        ui->button_register->hide();
        on_button_login_logout_clicked();
    }
}
