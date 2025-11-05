QT += core network concurrent

CONFIG += c++17

TARGET = SensorConnector
TEMPLATE = lib
CONFIG += staticlib

# Пути для исходников и заголовков
INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/src

# 🔹 LIBJPEG-TURBO CONFIGURATION
# ВАЖНО: Если библиотеки установлены не в стандартных местах,
# измените пути ниже на ваши пути
win32 {
    # Libjpeg-turbo
    # Путь по умолчанию: C:/libjpeg-turbo64
    # Если установлено в другом месте, измените эту строку:
    LIBJPEG_TURBO_DIR = C:/libjpeg-turbo64
    
    # Проверка существования
    !exists($$LIBJPEG_TURBO_DIR/include/turbojpeg.h) {
        warning("libjpeg-turbo not found at $$LIBJPEG_TURBO_DIR")
        warning("Please install libjpeg-turbo or change LIBJPEG_TURBO_DIR in SensorConnector.pro")
    }
    
    INCLUDEPATH += $$LIBJPEG_TURBO_DIR/include
    LIBS += -L$$LIBJPEG_TURBO_DIR/lib
    LIBS += -lturbojpeg

    # FFmpeg libraries
    # Путь по умолчанию: C:/ffmpeg
    # Если установлено в другом месте, измените эту строку:
    FFMPEG_DIR = C:/ffmpeg
    
    # Проверка существования
    !exists($$FFMPEG_DIR/include/libavcodec/avcodec.h) {
        warning("FFmpeg not found at $$FFMPEG_DIR")
        warning("Please install FFmpeg or change FFMPEG_DIR in SensorConnector.pro")
    }
    
    INCLUDEPATH += $$FFMPEG_DIR/include
    LIBS += -L$$FFMPEG_DIR/lib
    LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample
}

# Для Linux
unix:!macx {
    LIBS += -lturbojpeg -ljpeg
    LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample
}

# SOURCES - только файлы для SensorConnector
SOURCES += \
    src/SensorConnector.cpp \
    src/NetworkServerSimplified.cpp \
    src/UsbManager.cpp \
    src/NetworkConfigurator.cpp \
    src/TcpServer.cpp \
    src/TurboJPEGDecoder.cpp \
    src/FFmpegDecoder.cpp \
    src/FastJPEGDecoder.cpp

HEADERS += \
    include/SensorConnector.h \
    include/SensorDataTypes.h \
    include/NetworkServerSimplified.h \
    include/UsbManager.h \
    include/NetworkConfigurator.h \
    include/TcpServer.h \
    include/TurboJPEGDecoder.h \
    include/FFmpegDecoder.h \
    include/FastJPEGDecoder.h

# 🔹 КОПИРУЕМ DLL В ПАПКУ СБОРКИ
win32 {
    # TurboJPEG
    turbojpeg_dll.files = $$LIBJPEG_TURBO_DIR/bin/turbojpeg.dll
    turbojpeg_dll.path = $$OUT_PWD
    INSTALLS += turbojpeg_dll

    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$LIBJPEG_TURBO_DIR/bin/turbojpeg.dll) $$shell_path($$OUT_PWD) $$escape_expand(\\n\\t)
}

# Выходные файлы
DESTDIR = $$PWD/lib
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc

