/****************************************************************************
** Meta object code from reading C++ file 'UsbManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../include/UsbManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UsbManager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSUsbManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSUsbManagerENDCLASS = QtMocHelpers::stringData(
    "UsbManager",
    "usbClientConnected",
    "",
    "usbClientDisconnected",
    "usbStatusChanged",
    "status",
    "usbDataReceived",
    "data",
    "sequenceNumber",
    "usbLidarDataReceived",
    "usbSensorDataReceived",
    "usbRawLidarPointCloudReceived",
    "usbLidarConfidenceMapReceived",
    "sendUsbData",
    "handleUsbConnection",
    "handleUsbDisconnection",
    "processUsbData",
    "checkUsbConnection",
    "handleNetworkStatus",
    "handleUsbInterfaceDetected",
    "detected"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSUsbManagerENDCLASS_t {
    uint offsetsAndSizes[42];
    char stringdata0[11];
    char stringdata1[19];
    char stringdata2[1];
    char stringdata3[22];
    char stringdata4[17];
    char stringdata5[7];
    char stringdata6[16];
    char stringdata7[5];
    char stringdata8[15];
    char stringdata9[21];
    char stringdata10[22];
    char stringdata11[30];
    char stringdata12[30];
    char stringdata13[12];
    char stringdata14[20];
    char stringdata15[23];
    char stringdata16[15];
    char stringdata17[19];
    char stringdata18[20];
    char stringdata19[27];
    char stringdata20[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSUsbManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSUsbManagerENDCLASS_t qt_meta_stringdata_CLASSUsbManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 10),  // "UsbManager"
        QT_MOC_LITERAL(11, 18),  // "usbClientConnected"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 21),  // "usbClientDisconnected"
        QT_MOC_LITERAL(53, 16),  // "usbStatusChanged"
        QT_MOC_LITERAL(70, 6),  // "status"
        QT_MOC_LITERAL(77, 15),  // "usbDataReceived"
        QT_MOC_LITERAL(93, 4),  // "data"
        QT_MOC_LITERAL(98, 14),  // "sequenceNumber"
        QT_MOC_LITERAL(113, 20),  // "usbLidarDataReceived"
        QT_MOC_LITERAL(134, 21),  // "usbSensorDataReceived"
        QT_MOC_LITERAL(156, 29),  // "usbRawLidarPointCloudReceived"
        QT_MOC_LITERAL(186, 29),  // "usbLidarConfidenceMapReceived"
        QT_MOC_LITERAL(216, 11),  // "sendUsbData"
        QT_MOC_LITERAL(228, 19),  // "handleUsbConnection"
        QT_MOC_LITERAL(248, 22),  // "handleUsbDisconnection"
        QT_MOC_LITERAL(271, 14),  // "processUsbData"
        QT_MOC_LITERAL(286, 18),  // "checkUsbConnection"
        QT_MOC_LITERAL(305, 19),  // "handleNetworkStatus"
        QT_MOC_LITERAL(325, 26),  // "handleUsbInterfaceDetected"
        QT_MOC_LITERAL(352, 8)   // "detected"
    },
    "UsbManager",
    "usbClientConnected",
    "",
    "usbClientDisconnected",
    "usbStatusChanged",
    "status",
    "usbDataReceived",
    "data",
    "sequenceNumber",
    "usbLidarDataReceived",
    "usbSensorDataReceived",
    "usbRawLidarPointCloudReceived",
    "usbLidarConfidenceMapReceived",
    "sendUsbData",
    "handleUsbConnection",
    "handleUsbDisconnection",
    "processUsbData",
    "checkUsbConnection",
    "handleNetworkStatus",
    "handleUsbInterfaceDetected",
    "detected"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUsbManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,    1 /* Public */,
       3,    0,  105,    2, 0x06,    2 /* Public */,
       4,    1,  106,    2, 0x06,    3 /* Public */,
       6,    2,  109,    2, 0x06,    5 /* Public */,
       9,    2,  114,    2, 0x06,    8 /* Public */,
      10,    2,  119,    2, 0x06,   11 /* Public */,
      11,    2,  124,    2, 0x06,   14 /* Public */,
      12,    2,  129,    2, 0x06,   17 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      13,    1,  134,    2, 0x0a,   20 /* Public */,
      14,    0,  137,    2, 0x08,   22 /* Private */,
      15,    0,  138,    2, 0x08,   23 /* Private */,
      16,    0,  139,    2, 0x08,   24 /* Private */,
      17,    0,  140,    2, 0x08,   25 /* Private */,
      18,    1,  141,    2, 0x08,   26 /* Private */,
      19,    1,  144,    2, 0x08,   28 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::ULongLong,    7,    8,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::ULongLong,    7,    8,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::ULongLong,    7,    8,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::ULongLong,    7,    8,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::ULongLong,    7,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::QByteArray,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool,   20,

       0        // eod
};

Q_CONSTINIT const QMetaObject UsbManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUsbManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUsbManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUsbManagerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UsbManager, std::true_type>,
        // method 'usbClientConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usbClientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usbStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'usbDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'usbLidarDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'usbSensorDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'usbRawLidarPointCloudReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'usbLidarConfidenceMapReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'sendUsbData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'handleUsbConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleUsbDisconnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'processUsbData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkUsbConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleNetworkStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleUsbInterfaceDetected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void UsbManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UsbManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->usbClientConnected(); break;
        case 1: _t->usbClientDisconnected(); break;
        case 2: _t->usbStatusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->usbDataReceived((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 4: _t->usbLidarDataReceived((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 5: _t->usbSensorDataReceived((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 6: _t->usbRawLidarPointCloudReceived((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 7: _t->usbLidarConfidenceMapReceived((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 8: _t->sendUsbData((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 9: _t->handleUsbConnection(); break;
        case 10: _t->handleUsbDisconnection(); break;
        case 11: _t->processUsbData(); break;
        case 12: _t->checkUsbConnection(); break;
        case 13: _t->handleNetworkStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->handleUsbInterfaceDetected((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UsbManager::*)();
            if (_t _q_method = &UsbManager::usbClientConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)();
            if (_t _q_method = &UsbManager::usbClientDisconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QString & );
            if (_t _q_method = &UsbManager::usbStatusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QByteArray & , quint64 );
            if (_t _q_method = &UsbManager::usbDataReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QByteArray & , quint64 );
            if (_t _q_method = &UsbManager::usbLidarDataReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QByteArray & , quint64 );
            if (_t _q_method = &UsbManager::usbSensorDataReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QByteArray & , quint64 );
            if (_t _q_method = &UsbManager::usbRawLidarPointCloudReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UsbManager::*)(const QByteArray & , quint64 );
            if (_t _q_method = &UsbManager::usbLidarConfidenceMapReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
    }
}

const QMetaObject *UsbManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UsbManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSUsbManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UsbManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void UsbManager::usbClientConnected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UsbManager::usbClientDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UsbManager::usbStatusChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void UsbManager::usbDataReceived(const QByteArray & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void UsbManager::usbLidarDataReceived(const QByteArray & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void UsbManager::usbSensorDataReceived(const QByteArray & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void UsbManager::usbRawLidarPointCloudReceived(const QByteArray & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void UsbManager::usbLidarConfidenceMapReceived(const QByteArray & _t1, quint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
