// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "bindings.h"
#include <flipmansdk/core/profile.h>

NB_MODULE(flipman, module)
{
    module.doc() = "Flipman SDK: media, image, timecode, rendering and profiling APIs.";
    module.attr("__version__") = PROJECT_VERSION;
    flipman::sdk::python::bindCore(module);
    flipman::sdk::python::bindAv(module);
    flipman::sdk::python::bindMedia(module);
    flipman::sdk::python::bindRender(module);
    auto profiling = module.def_submodule("profile", "Process-wide SDK profiler.");
    namespace profile = flipman::sdk::profile;
    nb::enum_<profile::Mode>(profiling, "Mode")
        .value("Off", profile::Mode::Off)
        .value("Aggregate", profile::Mode::Aggregate)
        .value("Trace", profile::Mode::Trace);
    profiling.def("setMode", &profile::setMode, "mode"_a);
    profiling.def("mode", &profile::mode);
    profiling.def("setSlowThresholdUs", &profile::setSlowThresholdUs, "microseconds"_a);
    profiling.def("setTraceCapacity", &profile::setTraceCapacity, "events"_a);
    profiling.def("beginRecording", &profile::beginRecording, "name"_a, "filename"_a = std::string());
    profiling.def("endRecording", &profile::endRecording, "id"_a);
    profiling.def("recordingActive", &profile::recordingActive, "id"_a);
    profiling.def(
        "counter", [](const std::string& name, uint64_t value) { profile::counter(name.c_str(), value); }, "name"_a,
        "value"_a);
    profiling.def(
        "event",
        [](const std::string& category, const std::string& name, const std::string& details) {
            profile::event(category.c_str(), name.c_str(), details);
        },
        "category"_a, "name"_a, "details"_a);
    profiling.def("reset", &profile::reset);
    profiling.def("summary", &profile::summary);
    profiling.def("writeSummary", &profile::writeSummary, "filename"_a);
}
