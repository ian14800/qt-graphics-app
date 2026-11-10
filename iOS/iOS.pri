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

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

ios_icon.files = $$files($$PWD/Images.xcassets/AppIcon.appiconset/Icon_*.png)
QMAKE_BUNDLE_DATA += ios_icon

OTHER_FILES +=     $$PWD/Info.plist     $$PWD/Images.xcassets/AppIcon.appiconset/Contents.json

QMAKE_INFO_PLIST = $$PWD/Info.plist

# add_ios_ffmpeg_libraries is available in Qt 6.8.4+ (6.8.x) and 6.9.1+ (6.9.x)
versionAtLeast(QT_VERSION, 6.8.4):!versionAtLeast(QT_VERSION, 6.9.0) {
  CONFIG += add_ios_ffmpeg_libraries
} else:versionAtLeast(QT_VERSION, 6.9.1):!versionAtLeast(QT_VERSION, 6.10.0) {
  CONFIG += add_ios_ffmpeg_libraries
} else {
  # workaround for https://bugreports.qt.io/browse/QTBUG-129651
  # ArcGIS Maps SDK for Qt adds 'QMAKE_RPATHDIR = @executable_path/Frameworks'
  # and ffmpeg frameworks have embedded '@rpath/Frameworks' path.
  # so in order for them to be found, we need to add @executable_path to the
  # search path.
  FFMPEG_LIB_DIR = $$absolute_path($$replace(QMAKE_QMAKE, "qmake6", "../../ios/lib/ffmpeg"))
  FFMPEG_LIB_DIR = $$absolute_path($$replace(FFMPEG_LIB_DIR, "qmake", "../../ios/lib/ffmpeg"))
  QMAKE_LFLAGS += -F$${FFMPEG_LIB_DIR} -Wl,-rpath,@executable_path
  versionAtLeast(QT_VERSION, 6.8.3) {
    FRAMEWORK = "xcframework"
  } else {
    FRAMEWORK = "framework"
  }
  LIBS += -framework libavcodec           -framework libavformat           -framework libavutil           -framework libswresample           -framework libswscale
  ffmpeg.files = $${FFMPEG_LIB_DIR}/libavcodec.$${FRAMEWORK}                  $${FFMPEG_LIB_DIR}/libavformat.$${FRAMEWORK}                  $${FFMPEG_LIB_DIR}/libavutil.$${FRAMEWORK}                  $${FFMPEG_LIB_DIR}/libswresample.$${FRAMEWORK}                  $${FFMPEG_LIB_DIR}/libswscale.$${FRAMEWORK}
  ffmpeg.path = Frameworks
  QMAKE_BUNDLE_DATA += ffmpeg
}
