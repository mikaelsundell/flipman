// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/core/system.h>

#include <QPointer>

namespace flipman::sdk::core {
class ApplicationPrivate {
public:
    ApplicationPrivate();
    ~ApplicationPrivate();
    void init();
    struct Data {
        QScopedPointer<Environment> environment;
        QScopedPointer<Style> style;
        QScopedPointer<System> system;
    };
    Data d;
};

ApplicationPrivate::ApplicationPrivate() {}

ApplicationPrivate::~ApplicationPrivate() {}

void
ApplicationPrivate::init()
{
    d.environment.reset(new Environment());
    d.style.reset(new Style());
    d.system.reset(new System());
}

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
    , p(new ApplicationPrivate())
{
    p->init();
}

Application::~Application() {}

Environment*
Application::environment() const
{
    return p->d.environment.data();
}

Style*
Application::style() const
{
    return p->d.style.data();
}

System*
Application::system() const
{
    return p->d.system.data();
}

Application*
Application::instance()
{
    auto* app = qobject_cast<Application*>(QCoreApplication::instance());
    if (!app) {
        qFatal("Fatal Error in [core::Application::instance()]:\n"
               "The global application instance is missing or is not a core::Application. "
               "Ensure you have instantiated flipman::sdk::core::Application in your main() function.");
    }
    return app;
}
}  // namespace flipman::sdk::core
