// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "bindings.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/profile.h>
#include <flipmansdk/plugins/mediawriter.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/quicktime/quicktimewriter.h>
#include <QCoreApplication>
#include <QPointer>
#include <QThread>
#ifdef __APPLE__
#    include <pthread.h>
#endif

namespace flipman::sdk::python {

class RuntimePrivate {
public:
    struct Data {
        // argv storage must outlive owned; preserve this dependency order.
        int argc = 1;
        char name[8] = "flipman";
        char* argv[2] = { name, nullptr };
        std::unique_ptr<core::Application> owned;
        QPointer<core::Application> application;
    };
    Data d;
};

Runtime::Runtime()
{
#if defined(Q_OS_MACOS) && defined(QT_DEBUG)
    if (qEnvironmentVariable("DYLD_IMAGE_SUFFIX") != "_debug")
        throw std::runtime_error("Debug Qt requires DYLD_IMAGE_SUFFIX=_debug at process startup. "
                                 "Use the generated flipman-python launcher or the configured Python interpreter; "
                                 "the macOS /usr/bin/python3 launcher may strip DYLD settings.");
#endif
#ifdef __APPLE__
    if (!pthread_main_np())
        throw std::runtime_error("Runtime must be created on the process main thread");
#endif
    static std::weak_ptr<RuntimePrivate> shared;
    p = shared.lock();
    if (!p) {
        p = std::make_shared<RuntimePrivate>();
        if (QCoreApplication::instance()) {
            p->d.application = qobject_cast<core::Application*>(QCoreApplication::instance());
            if (!p->d.application)
                throw std::runtime_error("an existing Qt application must be a Flipman SDK Application");
        }
        else {
            p->d.owned = std::make_unique<core::Application>(p->d.argc, p->d.argv);
            p->d.application = p->d.owned.get();
        }
        shared = p;
    }
    check();
}

Runtime::~Runtime() = default;

void
Runtime::check() const
{
    if (!p->d.application)
        throw std::runtime_error("SDK application has been destroyed");
    if (QThread::currentThread() != p->d.application->thread())
        throw std::runtime_error("Qt-dependent SDK objects must be used on the application thread");
}

void
Runtime::processEvents()
{
    check();
    QCoreApplication::processEvents();
}

namespace {

    class Media {
    public:
        explicit Media(Runtime& runtime)
            : runtime(runtime)
        {
            runtime.check();
        }

        void check() const
        {
            runtime.check();
            if (!media.isOpen())
                throw std::runtime_error("media is not open");
        }

        Runtime& runtime;
        av::Media media;
    };

}  // namespace

void
bindMedia(nb::module_& module)
{
    nb::class_<Runtime>(module, "Runtime").def(nb::init<>()).def("processEvents", &Runtime::processEvents);
    nb::class_<Media>(module, "Media")
        .def(nb::init<Runtime&>(), "runtime"_a, nb::keep_alive<1, 2>())
        .def(
            "open",
            [](Media& self, const core::File& file, int timeoutMs, nb::dict values) {
                self.runtime.check();
                if (timeoutMs < 0)
                    throw nb::value_error("timeoutMs must be nonnegative");
                plugins::MediaReader::Options options;
                options.values = toVariant(values).toMap();
                if (!self.media.open(file, options))
                    return false;

                return self.media.waitForOpened(timeoutMs);
            },
            "file"_a, "timeoutMs"_a = 5000, nb::kw_only(), "options"_a = nb::dict())
        .def("close",
             [](Media& self) {
                 self.runtime.check();
                 return self.media.close();
             })
        .def("isOpen",
             [](const Media& self) {
                 self.runtime.check();
                 return self.media.isOpen();
             })
        .def("isValid",
             [](const Media& self) {
                 self.runtime.check();
                 return self.media.isValid();
             })
        .def(
            "isSupported",
            [](const Media& self, const QString& extension) {
                self.runtime.check();
                return self.media.isSupported(extension);
            },
            "extension"_a)
        .def("error",
             [](const Media& self) {
                 self.runtime.check();
                 return self.media.error();
             })
        .def("file",
             [](const Media& self) {
                 self.runtime.check();
                 return self.media.file();
             })
        .def("read",
             [](Media& self) {
                 self.check();
                 return self.media.read();
             })
        .def(
            "seek",
            [](Media& self, const av::TimeRange& range) {
                self.check();
                if (!range.isValid())
                    throw nb::value_error("seek range is invalid");
                return self.media.seek(range);
            },
            "range"_a)
        .def("time",
             [](const Media& self) {
                 self.check();
                 return self.media.time();
             })
        .def("start",
             [](const Media& self) {
                 self.check();
                 return self.media.start();
             })
        .def("fps",
             [](const Media& self) {
                 self.check();
                 return self.media.fps();
             })
        .def("timeRange",
             [](const Media& self) {
                 self.check();
                 return self.media.timeRange();
             })
        .def("image",
             [](const Media& self) {
                 self.check();
                 return self.media.image();
             })
        .def("metaData", [](const Media& self) {
            self.check();
            return self.media.metaData();
        });
    module.def(
        "writeImage",
        [](Runtime& runtime, const core::ImageBuffer& image, const core::File& file) {
            runtime.check();
            if (!image.isAllocated() || image.requiresDecode()
                || image.packing() != core::ImageBuffer::Packing::Interleaved)
                throw nb::value_error("writeImage requires allocated interleaved RGB pixels");
            QScopedPointer<plugins::MediaWriter> writer(
                core::pluginRegistry()->getPlugin<plugins::MediaWriter>(file.extension()));
            if (!writer)
                throw nb::value_error("no image writer for this extension");
            if (!writer->open(file))
                throw std::runtime_error(writer->error().message().toStdString());
            const auto fps = av::Fps::fps24();
            writer->setTimeRange(av::TimeRange(av::Time::zero(fps), av::Time::fromFrames(1, fps)));
            const av::Time result = writer->write(image);
            if (!result.isValid() || writer->error().hasError())
                throw std::runtime_error(writer->error().message().toStdString());
            if (auto* movie = dynamic_cast<plugins::QuicktimeWriter*>(writer.data())) {
                if (!movie->close())
                    throw std::runtime_error(movie->error().message().toStdString());
                return core::File(file.filePath());
            }
            return core::File(file.fileName(0));
        },
        "runtime"_a, "image"_a, "file"_a);
}

}  // namespace flipman::sdk::python
