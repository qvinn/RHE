

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QLocale ua("uk_UA");
    QLocale::setDefault(ua);
    QApplication a(argc, argv);
    a.setStyle("windowsvista");
    MainWindow Mw;
    Mw.show();
    return a.exec();
}
