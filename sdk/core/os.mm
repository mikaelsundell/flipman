// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/IOMessage.h>

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <QCoreApplication>
#include <QDir>
#include <QPointer>
#include <core/os.h>

namespace core {
class OSPrivate
{
public:
    OSPrivate();
    ~OSPrivate();
    void init();
    static void power_callback(void* refcon, io_service_t service, natural_t message_type, void* message);
    struct Data
    {
        id activity = nullptr;
        IONotificationPortRef notificationport = nullptr;
        io_object_t notifier = 0;
        core::Error error;
    };
    Data d;
    QPointer<OS> object;
};

OSPrivate::OSPrivate()
{
}

OSPrivate::~OSPrivate()
{
    if (d.notificationport) {
        IONotificationPortDestroy(d.notificationport);
    }
    if (d.notifier) {
        IODeregisterForSystemPower(&d.notifier);
    }
}

void
OSPrivate::init()
{
    io_connect_t rootport = IORegisterForSystemPower(this, &d.notificationport, power_callback, &d.notifier);
    if (rootport == MACH_PORT_NULL) {
        qWarning() << "failed to register for system power notifications.";
        return;
    }
    CFRunLoopSourceRef runloopsource = IONotificationPortGetRunLoopSource(d.notificationport);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runloopsource, kCFRunLoopDefaultMode);
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]]; // force dark aque appearance
}

void
OSPrivate::power_callback(void* refcon, io_service_t service, natural_t message_type, void* message)
{
    OSPrivate* osprivate = static_cast<OSPrivate*>(refcon);
    if (!osprivate || !osprivate->object) {
        return;
    }
    switch (message_type) {
        case kIOMessageSystemWillPowerOff:
            osprivate->object->power_changed(OS::PowerOff);
            break;
        case kIOMessageSystemWillRestart:
            osprivate->object->power_changed(OS::Restart);
            break;
        case kIOMessageSystemWillSleep:
            osprivate->object->power_changed(OS::Sleep);
            IOAllowPowerChange(service, (long)message);
            break;
        default:
            break;
    }
}

#include "os.moc"

OS::OS()
: p(new OSPrivate())
{
    p->object = this;
    p->init();
}

OS::~OS()
{
}

void
OS::stayawake(bool activity)
{
    static IOPMAssertionID assertionid = kIOPMNullAssertionID;
    if (activity) {
        if (assertionid == kIOPMNullAssertionID) {
            IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                        kIOPMAssertionLevelOn,
                                        CFSTR("Preventing idle sleep"),
                                        &assertionid);
        }
    } else {
        if (assertionid != kIOPMNullAssertionID) {
            IOPMAssertionRelease(assertionid);
            assertionid = kIOPMNullAssertionID;
        }
    }
}

QString
OS::programpath()
{
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return QFileInfo(QString::fromUtf8(path)).absolutePath();
    }
}

QString
OS::applicationpath()
{
    QString path = OS::programpath();
    QDir bundle(path);
    bundle.cdUp();
    bundle.cdUp();
    return bundle.absolutePath();
}

QString
OS::resourcepath(const QString& resource)
{
    return QDir(core::OS::applicationpath()).filePath(resource);
}

core::Error
OS::error() const
{
    return core::Error();
}

void
OS::reset()
{
    p.reset(new OSPrivate());
    p->init();
}

}
