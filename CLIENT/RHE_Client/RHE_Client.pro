QT += core gui

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets

CONFIG += c++11
CONFIG += app

win32:LIBS += -lws2_32 -ladvapi32

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    general_widget.cpp \
    main.cpp \
    mainwindow.cpp \
    registration_widget.cpp \
    rhe_widget.cpp \
    send_recieve_module.cpp

FORMS += \
    mainwindow.ui \
    registration_widget.ui \
    rhe_widget.ui

HEADERS += \
    general_widget.h \
    mainwindow.h \
    registration_widget.h \
    rhe_widget.h \
    send_recieve_module.h

TRANSLATIONS += Language/RHE_Client_en.ts \
                Language/RHE_Client_ua.ts \
                Language/RHE_Client_ru.ts

CODECFORSRC     = UTF-8

RESOURCES += \
    RHE_Client_Languages.qrc

system(lrelease \"$$_PRO_FILE_\")
tr.commands = lupdate \"$$_PRO_FILE_\" && lrelease \"$$_PRO_FILE_\"
    PRE_TARGETDEPS += tr
    QMAKE_EXTRA_TARGETS += tr

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
