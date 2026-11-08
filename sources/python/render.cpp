// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "bindings.h"
#include <QSize>
#include <flipmansdk/core/perfscope.h>
#include <flipmansdk/core/profile.h>
#include <flipmansdk/render/imageeffect.h>
#include <flipmansdk/render/imagelayer.h>
#include <flipmansdk/render/render.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>
#include <flipmansdk/render/shaderparser.h>

namespace flipman::sdk::python {
void
bindRender(nb::module_& module)
{
    auto colorspace = nb::enum_<render::ColorSpace>(module, "ColorSpace");
    colorspace.value("Unknown", render::ColorSpace::Unknown);
    colorspace.value("Raw", render::ColorSpace::Raw);
    colorspace.value("Rec601", render::ColorSpace::Rec601);
    colorspace.value("Rec709", render::ColorSpace::Rec709);
    colorspace.value("DisplayP3", render::ColorSpace::DisplayP3);
    colorspace.value("DCIP3", render::ColorSpace::DCIP3);
    colorspace.value("Rec2020", render::ColorSpace::Rec2020);
    colorspace.value("ACEScg", render::ColorSpace::ACEScg);
    auto transferfunction = nb::enum_<render::TransferFunction>(module, "TransferFunction");
    transferfunction.value("Unknown", render::TransferFunction::Unknown);
    transferfunction.value("Raw", render::TransferFunction::Raw);
    transferfunction.value("Linear", render::TransferFunction::Linear);
    transferfunction.value("SRGB", render::TransferFunction::SRGB);
    transferfunction.value("Gamma22", render::TransferFunction::Gamma22);
    transferfunction.value("Gamma24", render::TransferFunction::Gamma24);
    transferfunction.value("Gamma25", render::TransferFunction::Gamma25);
    transferfunction.value("Gamma26", render::TransferFunction::Gamma26);
    transferfunction.value("ACEScc", render::TransferFunction::ACEScc);
    transferfunction.value("ACEScct", render::TransferFunction::ACEScct);
    transferfunction.value("ADX10", render::TransferFunction::ADX10);
    transferfunction.value("ADX16", render::TransferFunction::ADX16);
    transferfunction.value("Cineon", render::TransferFunction::Cineon);
    transferfunction.value("AppleLog", render::TransferFunction::AppleLog);
    transferfunction.value("ArriLogC3", render::TransferFunction::ArriLogC3);
    transferfunction.value("ArriLogC4", render::TransferFunction::ArriLogC4);
    transferfunction.value("BmdFilmGen5", render::TransferFunction::BmdFilmGen5);
    transferfunction.value("DaVinciIntermediate", render::TransferFunction::DaVinciIntermediate);
    transferfunction.value("CanonLog2", render::TransferFunction::CanonLog2);
    transferfunction.value("CanonLog3", render::TransferFunction::CanonLog3);
    transferfunction.value("PanasonicVLog", render::TransferFunction::PanasonicVLog);
    transferfunction.value("RedLog3G10", render::TransferFunction::RedLog3G10);
    transferfunction.value("SonySLog3", render::TransferFunction::SonySLog3);
    auto workingspace = nb::enum_<render::WorkingSpace>(module, "WorkingSpace");
    workingspace.value("Unknown", render::WorkingSpace::Unknown);
    workingspace.value("Rec601", render::WorkingSpace::Rec601);
    workingspace.value("Rec709", render::WorkingSpace::Rec709);
    workingspace.value("DisplayP3", render::WorkingSpace::DisplayP3);
    workingspace.value("DCIP3", render::WorkingSpace::DCIP3);
    workingspace.value("Rec2020", render::WorkingSpace::Rec2020);
    workingspace.value("ACES2065_1", render::WorkingSpace::ACES2065_1);
    workingspace.value("ACEScg", render::WorkingSpace::ACEScg);
    workingspace.value("ArriWideGamut3", render::WorkingSpace::ArriWideGamut3);
    workingspace.value("ArriWideGamut4", render::WorkingSpace::ArriWideGamut4);
    workingspace.value("BmdWideGamutGen5", render::WorkingSpace::BmdWideGamutGen5);
    workingspace.value("DaVinciWideGamut", render::WorkingSpace::DaVinciWideGamut);
    workingspace.value("CinemaGamutD55", render::WorkingSpace::CinemaGamutD55);
    workingspace.value("VGamut", render::WorkingSpace::VGamut);
    workingspace.value("RedWideGamutRGB", render::WorkingSpace::RedWideGamutRGB);
    workingspace.value("SGamut3", render::WorkingSpace::SGamut3);
    workingspace.value("SGamut3Cine", render::WorkingSpace::SGamut3Cine);
    workingspace.value("VeniceSGamut3", render::WorkingSpace::VeniceSGamut3);
    workingspace.value("VeniceSGamut3Cine", render::WorkingSpace::VeniceSGamut3Cine);
    nb::class_<render::ShaderDefinition>(module, "ShaderDefinition")
        .def("isValid", &render::ShaderDefinition::isValid)
        .def("shaderCode", &render::ShaderDefinition::shaderCode)
        .def("uniformBlock", &render::ShaderDefinition::uniformBlock, "binding"_a = 0,
             "uniformName"_a = QString("Params"))
        .def("error", &render::ShaderDefinition::error);
    module.def(
        "parseShader",
        [](const QString& source) {
            render::ShaderParser parser;
            const auto definition = parser.parse(source);
            if (!parser.isValid() || !definition.isValid())
                throw std::runtime_error(parser.error().message().toStdString());
            return definition;
        },
        "source"_a);
    nb::class_<render::ImageEffect>(module, "ImageEffect")
        .def(nb::init<>())
        .def("name", &render::ImageEffect::name)
        .def("setName", &render::ImageEffect::setName, "name"_a)
        .def("group", &render::ImageEffect::group)
        .def("setGroup", &render::ImageEffect::setGroup, "group"_a)
        .def("isValid", &render::ImageEffect::isValid)
        .def("error", &render::ImageEffect::error)
        .def("shaderDefinition", &render::ImageEffect::shaderDefinition)
        .def("setShaderDefinition", &render::ImageEffect::setShaderDefinition, "definition"_a);
    nb::class_<render::ImageLayer>(module, "ImageLayer")
        .def(nb::init<>())
        .def("image", &render::ImageLayer::image)
        .def("setImage", &render::ImageLayer::setImage, "image"_a)
        .def("imageEffect", &render::ImageLayer::imageEffect)
        .def("setImageEffect", &render::ImageLayer::setImageEffect, "effect"_a)
        .def("error", &render::ImageLayer::error);
    module.def(
        "renderImage",
        [](Runtime& runtime, const core::ImageBuffer& image, int width, int height) {
            runtime.check();
            if (!image.isValid() || !image.isAllocated())
                throw nb::value_error("image must be allocated");
            if (width < 0 || height < 0)
                throw nb::value_error("render dimensions must be nonnegative");
            const QSize size(width ? width : image.displayWindow().width(),
                             height ? height : image.displayWindow().height());
            if (size.isEmpty())
                throw nb::value_error("render dimensions must be positive");
            FM_PROFILE_SCOPE("render", "python.renderImage");
            core::PerfScope phase("render.python_image");
            // Destroy GPU pipelines before the device they reference.
            render::RenderOffscreen renderer;
            render::RenderEngine engine;
            if (!renderer.initialize(size))
                throw std::runtime_error("could not initialize offscreen render device");
            render::ImageLayer layer;
            layer.setImage(image);
            engine.setResolution(size);
            engine.setImageLayers({ layer });
            renderer.setRenderEngine(&engine);
            core::ImageBuffer result = renderer.render();
            if (!result.isValid() || !result.isAllocated())
                throw std::runtime_error("offscreen rendering failed");
            return result;
        },
        "runtime"_a, "image"_a, "width"_a = 0, "height"_a = 0);
}
}  // namespace flipman::sdk::python
