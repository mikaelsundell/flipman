// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "bindings.h"

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <cmath>

namespace flipman::sdk::python {

namespace {

    void validFps(const av::Fps& value)
    {
        if (!value.isValid() || value.numerator() <= 0)
            throw nb::value_error("frame rate must have positive numerator and denominator");
    }

    void validTime(const av::Time& value)
    {
        if (!value.isValid())
            throw nb::value_error("time must have a positive timeScale");
        validFps(value.fps());
    }

    void matchingTime(const av::Time& a, const av::Time& b)
    {
        validTime(a);
        validTime(b);
        if (a.timeScale() != b.timeScale() || a.fps() != b.fps())
            throw nb::value_error("times must use the same timeScale and fps; convert explicitly");
    }

}  // namespace

void
bindAv(nb::module_& module)
{
    auto fps = nb::class_<av::Fps>(module, "Fps")
                   .def(nb::init<>())
                   .def(nb::init<const av::Fps&>(), "other"_a)
                   .def(
                       "__init__",
                       [](av::Fps* self, qint32 numerator, qint32 denominator, bool dropFrame) {
                           av::Fps value(numerator, denominator, dropFrame);
                           validFps(value);
                           new (self) av::Fps(value);
                       },
                       "numerator"_a, "denominator"_a = 1, "dropFrame"_a = false)
                   .def("isValid", &av::Fps::isValid)
                   .def("numerator", &av::Fps::numerator)
                   .def("denominator", &av::Fps::denominator)
                   .def("dropFrame", &av::Fps::dropFrame)
                   .def("real",
                        [](const av::Fps& self) {
                            validFps(self);
                            return self.real();
                        })
                   .def("seconds",
                        [](const av::Fps& self) {
                            validFps(self);
                            return self.seconds();
                        })
                   .def("frameQuanta",
                        [](const av::Fps& self) {
                            validFps(self);
                            return self.frameQuanta();
                        })
                   .def("frameScale",
                        [](const av::Fps& self) {
                            validFps(self);
                            return self.frameScale();
                        })
                   .def("__float__",
                        [](const av::Fps& self) {
                            validFps(self);
                            return self.real();
                        })
                   .def("toString",
                        [](const av::Fps& self) { return self.isValid() ? self.toString() : QString("invalid"); })
                   .def(nb::self == nb::self)
                   .def(nb::self != nb::self)
                   .def_static(
                       "guess",
                       [](double value) {
                           if (!std::isfinite(value) || value <= 0 || value > 1000000)
                               throw nb::value_error("fps must be finite, positive and at most 1000000");
                           return av::Fps::guess(value);
                       },
                       "fps"_a);
    fps.def_static("fps23_976", &av::Fps::fps23_976);
    fps.def_static("fps24", &av::Fps::fps24);
    fps.def_static("fps25", &av::Fps::fps25);
    fps.def_static("fps29_97", &av::Fps::fps29_97);
    fps.def_static("fps30", &av::Fps::fps30);
    fps.def_static("fps47_952", &av::Fps::fps47_952);
    fps.def_static("fps48", &av::Fps::fps48);
    fps.def_static("fps50", &av::Fps::fps50);
    fps.def_static("fps59_94", &av::Fps::fps59_94);
    fps.def_static("fps60", &av::Fps::fps60);
    nb::class_<av::Time>(module, "Time")
        .def(nb::init<>())
        .def(nb::init<const av::Time&>(), "other"_a)
        .def(
            "__init__",
            [](av::Time* self, qint64 ticks, qint32 timeScale, const av::Fps& fps) {
                av::Time value(ticks, timeScale, fps);
                validTime(value);
                new (self) av::Time(value);
            },
            "ticks"_a, "timeScale"_a, "fps"_a)
        .def("isValid", &av::Time::isValid)
        .def("fps", &av::Time::fps)
        .def("ticks", nb::overload_cast<>(&av::Time::ticks, nb::const_))
        .def("timeScale", &av::Time::timeScale)
        .def("frames",
             [](const av::Time& self) {
                 validTime(self);
                 return self.frames();
             })
        .def("seconds",
             [](const av::Time& self) {
                 validTime(self);
                 return self.seconds();
             })
        .def("toString", [](const av::Time& self) { return self.isValid() ? self.toString() : QString("invalid"); })
        .def("__str__", [](const av::Time& self) { return self.isValid() ? self.toString() : QString("invalid"); })
        .def(
            "__add__",
            [](const av::Time& self, const av::Time& other) {
                matchingTime(self, other);
                return self + other;
            },
            nb::is_operator())
        .def(
            "__sub__",
            [](const av::Time& self, const av::Time& other) {
                matchingTime(self, other);
                return self - other;
            },
            nb::is_operator())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def_static(
            "zero",
            [](const av::Fps& fps) {
                validFps(fps);
                return av::Time::zero(fps);
            },
            "fps"_a)
        .def_static(
            "fromFrames",
            [](qint64 frame, const av::Fps& fps) {
                validFps(fps);
                return av::Time::fromFrames(frame, fps);
            },
            "frame"_a, "fps"_a)
        .def_static(
            "fromSeconds",
            [](double seconds, const av::Fps& fps) {
                validFps(fps);
                if (!std::isfinite(seconds) || std::abs(seconds * fps.real()) > 1e15)
                    throw nb::value_error("seconds must be finite and within the supported frame range");
                return av::Time::fromSeconds(seconds, fps);
            },
            "seconds"_a, "fps"_a)
        .def_static(
            "convert",
            [](const av::Time& time, const av::Fps& fps) {
                validTime(time);
                validFps(fps);
                return av::Time::convert(time, fps);
            },
            "time"_a, "fps"_a)
        .def_static(
            "convert",
            [](const av::Time& time, qint32 timeScale) {
                validTime(time);
                if (timeScale <= 0)
                    throw nb::value_error("timeScale must be positive");
                return av::Time::convert(time, timeScale);
            },
            "time"_a, "timeScale"_a);
    nb::class_<av::TimeRange>(module, "TimeRange")
        .def(nb::init<>())
        .def(nb::init<const av::TimeRange&>(), "other"_a)
        .def(
            "__init__",
            [](av::TimeRange* self, const av::Time& start, const av::Time& duration) {
                matchingTime(start, duration);
                if (duration.ticks() <= 0)
                    throw nb::value_error("duration must be positive");
                new (self) av::TimeRange(start, duration);
            },
            "start"_a, "duration"_a)
        .def("isValid", &av::TimeRange::isValid)
        .def("start", &av::TimeRange::start)
        .def("duration", &av::TimeRange::duration)
        .def("end",
             [](const av::TimeRange& self) {
                 if (!self.isValid())
                     throw nb::value_error("range is invalid");
                 return self.end();
             })
        .def(
            "bound",
            [](av::TimeRange& self, const av::Time& time, bool loop) {
                if (!self.isValid())
                    throw nb::value_error("range is invalid");
                matchingTime(self.start(), time);
                if (self.duration().ticks() < time.tpf())
                    throw nb::value_error("range must contain at least one frame");
                return self.bound(time, loop);
            },
            "time"_a, "loop"_a = false)
        .def(
            "intersects",
            [](const av::TimeRange& self, const av::TimeRange& other) {
                if (!self.isValid() || !other.isValid())
                    throw nb::value_error("ranges must be valid");
                matchingTime(self.start(), other.start());
                return self.intersects(other);
            },
            "other"_a)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self);
    nb::class_<av::SmpteTime>(module, "SmpteTime")
        .def(
            "__init__",
            [](av::SmpteTime* self, const av::Time& time) {
                validTime(time);
                new (self) av::SmpteTime(time);
            },
            "time"_a)
        .def("hours", &av::SmpteTime::hours)
        .def("minutes", &av::SmpteTime::minutes)
        .def("seconds", &av::SmpteTime::seconds)
        .def("frames", &av::SmpteTime::frames)
        .def("frame", &av::SmpteTime::frame)
        .def("time", &av::SmpteTime::time)
        .def("isValid", &av::SmpteTime::isValid)
        .def("toString", &av::SmpteTime::toString)
        .def("__str__", &av::SmpteTime::toString);
}

}  // namespace flipman::sdk::python
