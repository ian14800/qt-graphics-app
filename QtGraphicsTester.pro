#-------------------------------------------------
#  Copyright 2026 ESRI
#
#  All rights reserved under the copyright laws of the United States
#  and applicable international laws, treaties, and conventions.
#
#  You may freely redistribute and use this sample code, with or
#  without modification, provided you include the original copyright
#  notice and use restrictions.
#
#  See the Sample code usage restrictions document for further information.
#-------------------------------------------------

TEMPLATE = app

CONFIG += c++17

# additional modules are pulled in via arcgisruntime.pri
QT += qml quick

TARGET = QtGraphicsTester

!versionAtLeast(QT_VERSION, 6.8.2) {
    error("$$TARGET requires Qt 6.8.2 or newer")
}

ARCGIS_RUNTIME_VERSION = 300.2.0
include($$PWD/arcgisruntime.pri)

HEADERS += \
    QtGraphicsTester.h

SOURCES += \
    main.cpp \
    QtGraphicsTester.cpp

RESOURCES += \
    qml/qml.qrc \
    Resources/Resources.qrc \
    credentials.qrc

#-------------------------------------------------------------------------------

win32 {
    include (Win/Win.pri)
}

macx {
    include (Mac/Mac.pri)
}

ios {
    include (iOS/iOS.pri)
}

android {
    include (Android/Android.pri)
}
