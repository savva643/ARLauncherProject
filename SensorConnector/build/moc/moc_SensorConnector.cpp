/****************************************************************************
** Meta object code from reading C++ file 'SensorConnector.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../include/SensorConnector.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SensorConnector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS = QtMocHelpers::stringData(
    "SensorConnector::SensorConnectorCore",
    "dataReceived",
    "",
    "SensorData",
    "data",
    "frameDecoded",
    "frame",
    "sequenceNumber",
    "statisticsUpdated",
    "ConnectionStats",
    "stats",
    "connectionStatusChanged",
    "status",
    "clientsCountChanged",
    "count"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS_t {
    uint offsetsAndSizes[30];
    char stringdata0[37];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[5];
    char stringdata5[13];
    char stringdata6[6];
    char stringdata7[15];
    char stringdata8[18];
    char stringdata9[16];
    char stringdata10[6];
    char stringdata11[24];
    char stringdata12[7];
    char stringdata13[20];
    char stringdata14[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS_t qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS = {
    {
        QT_MOC_LITERAL(0, 36),  // "SensorConnector::SensorConnec..."
        QT_MOC_LITERAL(37, 12),  // "dataReceived"
        QT_MOC_LITERAL(50, 0),  // ""
        QT_MOC_LITERAL(51, 10),  // "SensorData"
        QT_MOC_LITERAL(62, 4),  // "data"
        QT_MOC_LITERAL(67, 12),  // "frameDecoded"
        QT_MOC_LITERAL(80, 5),  // "frame"
        QT_MOC_LITERAL(86, 14),  // "sequenceNumber"
        QT_MOC_LITERAL(101, 17),  // "statisticsUpdated"
        QT_MOC_LITERAL(119, 15),  // "ConnectionStats"
        QT_MOC_LITERAL(135, 5),  // "stats"
        QT_MOC_LITERAL(141, 23),  // "connectionStatusChanged"
        QT_MOC_LITERAL(165, 6),  // "status"
        QT_MOC_LITERAL(172, 19),  // "clientsCountChanged"
        QT_MOC_LITERAL(192, 5)   // "count"
    },
    "SensorConnector::SensorConnectorCore",
    "dataReceived",
    "",
    "SensorData",
    "data",
    "frameDecoded",
    "frame",
    "sequenceNumber",
    "statisticsUpdated",
    "ConnectionStats",
    "stats",
    "connectionStatusChanged",
    "status",
    "clientsCountChanged",
    "count"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x06,    1 /* Public */,
       5,    2,   47,    2, 0x06,    3 /* Public */,
       8,    1,   52,    2, 0x06,    6 /* Public */,
      11,    1,   55,    2, 0x06,    8 /* Public */,
      13,    1,   58,    2, 0x06,   10 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QImage, QMetaType::ULongLong,    6,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::Int,   14,

       0        // eod
};

Q_CONSTINIT const QMetaObject SensorConnector::SensorConnectorCore::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SensorConnectorCore, std::true_type>,
        // method 'dataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const SensorData &, std::false_type>,
        // method 'frameDecoded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'statisticsUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ConnectionStats &, std::false_type>,
        // method 'connectionStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientsCountChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void SensorConnector::SensorConnectorCore::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SensorConnectorCore *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dataReceived((*reinterpret_cast< std::add_pointer_t<SensorData>>(_a[1]))); break;
        case 1: _t->frameDecoded((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 2: _t->statisticsUpdated((*reinterpret_cast< std::add_pointer_t<ConnectionStats>>(_a[1]))); break;
        case 3: _t->connectionStatusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->clientsCountChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SensorConnectorCore::*)(const SensorData & );
            if (_t _q_method = &SensorConnectorCore::dataReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SensorConnectorCore::*)(const QImage & , quint64 );
            if (_t _q_method = &SensorConnectorCore::frameDecoded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SensorConnectorCore::*)(const ConnectionStats & );
            if (_t _q_method = &SensorConnectorCore::statisticsUpdated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SensorConnectorCore::*)(const QString & );
            if (_t _q_method = &SensorConnectorCore::connectionStatusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SensorConnectorCore::*)(int );
            if (_t _q_method = &SensorConnectorCore::clientsCountChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *SensorConnector::SensorConnectorCore::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SensorConnector::SensorConnectorCore::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSensorConnectorSCOPESensorConnectorCoreENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SensorConnector::SensorConnectorCore::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SensorConnector::SensorConnectorCore::dataReceived(const SensorData & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SensorConnector::SensorConnectorCore::frameDecoded(const QImage & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SensorConnector::SensorConnectorCore::statisticsUpdated(const ConnectionStats & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SensorConnector::SensorConnectorCore::connectionStatusChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SensorConnector::SensorConnectorCore::clientsCountChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
