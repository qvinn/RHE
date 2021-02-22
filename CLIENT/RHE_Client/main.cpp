#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(".");
    paths.append("imageformats");
    paths.append("platforms");
    paths.append("sqldrivers");
    QCoreApplication::setLibraryPaths(paths);
    QApplication a(argc, argv);
#ifdef __linux__
    a.setWindowIcon(QIcon(":/icons/1.png"));
#endif
    a.setStyle("windowsvista");
    MainWindow Mw;
    Mw.show();
    return a.exec();
}
