TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

INCLUDEPATH += $$PWD/../../libusb-1.0.27/include
LIBS += -L$$PWD/../../libusb-1.0.27/MinGW32/static
LIBS += -llibusb-1.0 -llibusb-1.0.dll
