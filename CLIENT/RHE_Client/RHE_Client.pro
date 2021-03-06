QT += core widgets gui network printsupport

CONFIG += app c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    data_transfer_module.cpp \
    general_widget.cpp \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \
    registration_widget.cpp \
    rhe_widget.cpp \
    send_receive_module.cpp \
    waveform_viewer_widget.cpp

FORMS += \
    dialog_select_diagram_settings.ui \
    dialog_select_displayable_pins.ui \
    dialog_set_server_ip.ui \
    mainwindow.ui \
    registration_widget.ui \
    rhe_widget.ui \
    waveform_viewer.ui

HEADERS += \
    data_transfer_module.h \
    general_widget.h \
    mainwindow.h \
    qcustomplot.h \
    registration_widget.h \
    rhe_widget.h \
    send_receive_module.h \
    waveform_viewer_widget.h

TRANSLATIONS += \
    Language/RHE_Client_en.ts \
    Language/RHE_Client_ua.ts \
    Language/RHE_Client_ru.ts

CODECFORSRC = UTF-8

win32:RC_ICONS += icons/1.ico

RESOURCES += \
    RHE_Client_Languages.qrc \
    icons.qrc \
    fonts.qrc

system(lrelease \"$$_PRO_FILE_\")
tr.commands = lupdate \"$$_PRO_FILE_\" && lrelease \"$$_PRO_FILE_\"
    PRE_TARGETDEPS += tr
    QMAKE_EXTRA_TARGETS += tr

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
