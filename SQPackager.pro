QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        basestuff.cpp \
        debian/debian.cpp \
        flatpak.cpp \
        main.cpp \
        print.cpp \
        runner.cpp \
        Windows/windows.cpp \
        desktoprc.cpp

HEADERS += \
    basestuff.h \
    print.h \
    projectdefinition.h \
    runner.h \
    desktoprc.h \
    sqpackager.h

static {
    LIBS += -lWindowsApp
}

RESOURCES += templates.qrc

DISTFILES += \
    debian/control_template.tt \
    debian/rules_template.tt \
    flatpak/installenv.sh

