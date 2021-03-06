# disabling the webkit component on windows, since I can't static link to it
# disabled webkit on every platform - Enrico 20090410
# DEFINES += USE_QTWEBKIT
# QT += webkit
# HEADERS += Canvas/BrowserItem.h
# SOURCES += Canvas/BrowserItem.cpp
HEADERS += \
    Canvas/AbstractConfig.h \
    Canvas/AbstractContent.h \
    Canvas/AbstractDisposeable.h \
    Canvas/BezierCubicItem.h \
    Canvas/ButtonItem.h \
    Canvas/CanvasModeInfo.h \
    Canvas/CanvasViewContent.h \
    Canvas/Canvas.h \
    Canvas/CornerItem.h \
    Canvas/HelpItem.h \
    Canvas/HighlightItem.h \
    Canvas/MirrorItem.h \
    Canvas/PictureConfig.h \
    Canvas/PictureContent.h \
    Canvas/PictureProperties.h \
    Canvas/PictureSearchItem.h \
    Canvas/SelectionProperties.h \
    Canvas/StyledButtonItem.h \
    Canvas/TextConfig.h \
    Canvas/TextContent.h \
    Canvas/TextProperties.h \
    Canvas/WebcamContent.h \
    Canvas/WordcloudContent.h

SOURCES += \
    Canvas/AbstractConfig.cpp \
    Canvas/AbstractContent.cpp \
    Canvas/AbstractDisposeable.cpp \
    Canvas/BezierCubicItem.cpp \
    Canvas/ButtonItem.cpp \
    Canvas/CanvasModeInfo.cpp \
    Canvas/CanvasViewContent.cpp \
    Canvas/Canvas.cpp \
    Canvas/CornerItem.cpp \
    Canvas/HelpItem.cpp \
    Canvas/HighlightItem.cpp \
    Canvas/MirrorItem.cpp \
    Canvas/PictureConfig.cpp \
    Canvas/PictureContent.cpp \
    Canvas/PictureProperties.cpp \
    Canvas/PictureSearchItem.cpp \
    Canvas/SelectionProperties.cpp \
    Canvas/StyledButtonItem.cpp \
    Canvas/TextConfig.cpp \
    Canvas/TextContent.cpp \
    Canvas/TextProperties.cpp \
    Canvas/WebcamContent.cpp \
    Canvas/WordcloudContent.cpp

FORMS += \
    Canvas/AbstractConfig.ui \
    Canvas/PictureConfig.ui \
    Canvas/PictureProperties.ui \
    Canvas/PictureSearchItem.ui \
    Canvas/TextProperties.ui
