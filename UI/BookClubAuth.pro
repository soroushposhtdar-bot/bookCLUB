QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET   = BookClubAuth
TEMPLATE = app

INCLUDEPATH += $$PWD/include

SOURCES += \
    main.cpp \
    src/auth/AuthManager.cpp \
    src/windows/MainWindow.cpp \
    src/windows/LoginPage.cpp \
    src/windows/RegisterPage.cpp \
    src/windows/ResetPasswordPage.cpp

HEADERS += \
    include/auth/AuthManager.h \
    include/windows/MainWindow.h \
    include/windows/LoginPage.h \
    include/windows/RegisterPage.h \
    include/windows/ResetPasswordPage.h

RESOURCES += resources.qrc

# Default deployment rules
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
