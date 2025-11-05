QT += core network concurrent

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = SensorConnectorExample
TEMPLATE = app

# Пути для исходников и заголовков
INCLUDEPATH += $$PWD/../include
INCLUDEPATH += $$PWD/../src

# Подключаем статическую библиотеку SensorConnector
LIBS += -L$$PWD/../lib -lSensorConnector

# Для Linux
unix:!macx {
    LIBS += -lturbojpeg -ljpeg
    LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample
}

# Исходные файлы
SOURCES += example_usage.cpp

# Выходные файлы
DESTDIR = $$PWD/../bin
OBJECTS_DIR = $$PWD/../build/obj
MOC_DIR = $$PWD/../build/moc

