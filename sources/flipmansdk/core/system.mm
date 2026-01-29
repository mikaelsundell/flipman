// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman


#include <QCoreApplication>
#include <QDir>
#include <QPointer>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/IOMessage.h>

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>

#include <core/system.h>

namespace flipman::sdk::core {
class SystemPrivate
{
public:
    SystemPrivate();
    ~SystemPrivate();
    void init();
    static void powerCallback(void* refcon, io_service_t service, natural_t message_type, void* message);
    struct Data
    {
        id activity = nullptr;
        IONotificationPortRef notificationport = nullptr;
        io_object_t notifier = 0;
        QPointer<System> object;
    };
    Data d;
};

SystemPrivate::SystemPrivate()
{
}

SystemPrivate::~SystemPrivate()
{
    if (d.notificationport) {
        IONotificationPortDestroy(d.notificationport);
    }
    if (d.notifier) {
        IODeregisterForSystemPower(&d.notifier);
    }
}

void
SystemPrivate::init()
{
    io_connect_t rootport = IORegisterForSystemPower(this, &d.notificationport, powerCallback, &d.notifier);
    if (rootport == MACH_PORT_NULL) {
        qWarning() << "failed to register for system power notifications.";
        return;
    }
    CFRunLoopSourceRef runloopsource = IONotificationPortGetRunLoopSource(d.notificationport);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runloopsource, kCFRunLoopDefaultMode);
}

void
SystemPrivate::powerCallback(void* refcon, io_service_t service, natural_t message_type, void* message)
{
    SystemPrivate* priv = static_cast<SystemPrivate*>(refcon);
    if (!priv || !priv->d.object) {
        return;
    }
    switch (message_type) {
        case kIOMessageSystemWillPowerOff:
            Q_EMIT priv->d.object->powerStateChanged(System::PowerOff);
            break;
        case kIOMessageSystemWillRestart:
            Q_EMIT priv->d.object->powerStateChanged(System::Restart);
            break;
        case kIOMessageSystemWillSleep:
            Q_EMIT priv->d.object->powerStateChanged(System::Sleep);
            IOAllowPowerChange(service, (long)message);
            break;
        default:
            break;
    }
}

System::System()
: p(new SystemPrivate())
{
    p->d.object = this;
    p->init();
}

System::~System()
{
}

void
System::setStayAwake(bool stayAwake)
{
    static IOPMAssertionID assertionid = kIOPMNullAssertionID;
    if (stayAwake) {
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

bool
System::isStayAwake() const
{
}

}
