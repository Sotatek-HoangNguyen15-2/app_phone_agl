TEMPLATE = app
TARGET = phone
QT = qml quickcontrols2
CONFIG += c++11 link_pkgconfig

PKGCONFIG += qtappfw-phone qtappfw-bt-pbap gstreamer-1.0

SOURCES = main.cpp phone.cpp fileplayer.cpp
HEADERS = phone.h numbertype.h fileplayer.h

RESOURCES += \
    phone.qrc \
    images/images.qrc

target.path = /usr/bin
target.files += $${OUT_PWD}/$${TARGET}
target.CONFIG = no_check_exist executable

ringtone.path = /usr/share/sounds/ringtones
ringtone.files = $$_PRO_FILE_PWD_/Phone.wav
ringtone.CONFIG = no_check_exist

INSTALLS += target ringtone
