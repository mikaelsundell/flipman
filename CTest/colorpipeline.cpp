// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/core/application.h>
#include <flipmansdk/render/colorpipeline.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>
#include <flipmansdk/render/shaderparser.h>
#include <OpenColorIO/OpenColorIO.h>
#include <QDebug>
#include <QFile>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>
using namespace flipman::sdk;
namespace ocio = OCIO_NAMESPACE;

static void
require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

int
main(int argc, char** argv)
{
    core::Application app(argc, argv);
    try {
        QFile file(":/flipmansdk/configs/studio-config-v4.0.0_aces-v2.0_ocio-v2.5.ocio");
        require(file.open(QIODevice::ReadOnly), "config resource missing");
        std::istringstream stream(file.readAll().toStdString());
        const auto config = ocio::Config::CreateFromStream(stream);
        require(render::ColorPipeline::colorSpaces().contains("ACEScg"), "ACEScg missing");
        render::DisplayTransform tag { render::ColorSpace::Rec2020, render::TransferFunction::Gamma26 };
        require(!render::ColorPipeline::displayTag("Rec.2100-PQ - Display", tag), "unsupported HDR tag matched");
        require(tag.colorSpace == render::ColorSpace::Rec2020, "unmatched tag changed");
        require(render::ColorPipeline::displayTag("Display P3 - Display", tag)
                    && tag.colorSpace == render::ColorSpace::DisplayP3,
                "P3 output tag mismatch");
        const QRect bounds(0, 0, 4, 2);
        core::ImageBuffer image(bounds, bounds, core::ImageFormat::Float, 4);
        image.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
        image.allocate();
        const float values[] = { 0.18f, 0.18f, 0.18f, 1, 0, 0, 0,    1, 1,      0,    0,    1, 0,    1,    0,    1,
                                 0,     0,     1,     1, 4, 2, 0.5f, 1, -0.01f, 0.1f, 0.5f, 1, 0.5f, 0.3f, 0.2f, 1 };
        std::memcpy(image.data(), values, sizeof(values));
        render::RenderOffscreen offscreen;
        require(offscreen.initialize(bounds.size()), "GPU initialization failed");
        render::RenderEngine engine;
        engine.setResolution(bounds.size());
        engine.setBackground(Qt::transparent);
        offscreen.setRenderEngine(&engine);
        render::ImageLayer layer;
        layer.setImage(image);
        engine.setImageLayers({ layer });
        render::ColorSettings settings;
        settings.enabled = true;
        auto compare = [&](const QString& source, const QString& display, const QString& view) {
            settings.display = display;
            settings.view = view;
            layer.setInputColorSpace(source);
            require(engine.setColorSettings(settings), qPrintable(engine.error().message()));
            const auto rendered = offscreen.render();
            require(rendered.isAllocated(), qPrintable(engine.error().message()));
            const auto result = core::ImageBuffer::convert(rendered, core::ImageFormat::Float, 4);
            auto transform = ocio::DisplayViewTransform::Create();
            transform->setSrc("ACEScg");
            transform->setDisplay(display.toUtf8().constData());
            transform->setView(view.toUtf8().constData());
            const auto input = config->getProcessor(source.toUtf8().constData(), "ACEScg")->getDefaultCPUProcessor();
            const auto output = config->getProcessor(transform)->getDefaultCPUProcessor();
            const auto* gpu = reinterpret_cast<const float*>(result.data());

            // CPU conversion is only a test oracle. Playback never calls applyRGB.
            for (int pixel = 0; pixel < 8; ++pixel) {
                float reference[3] = { values[4 * pixel], values[4 * pixel + 1], values[4 * pixel + 2] };
                input->applyRGB(reference);
                output->applyRGB(reference);
                for (int c = 0; c < 3; ++c) {
                    const float actual = gpu[4 * pixel + c];
                    if (!std::isfinite(actual)
                        || std::abs(actual - reference[c]) > 0.008f * std::max(1.0f, std::abs(reference[c]))) {
                        qCritical() << source << display << view << "pixel" << pixel << "channel" << c << actual
                                    << reference[c];
                        throw std::runtime_error("GPU color differs from OCIO CPU reference");
                    }
                }
                require(std::abs(gpu[4 * pixel + 3] - 1) < 0.001f, "alpha changed");
            }
        };
        compare("Raw", settings.display, "Un-tone-mapped");
        compare("ACEScg", settings.display, "Un-tone-mapped");
        compare("ACEScg", settings.display, "ACES 2.0 - SDR 100 nits (Rec.709)");
        compare("ARRI LogC3 (EI800)", "Display P3 - Display", "ACES 2.0 - SDR 100 nits (P3 D65)");
        compare("sRGB - Display", "sRGB - Display", "Un-tone-mapped");
        compare("ACES2065-1", "Rec.1886 Rec.709 - Display", "Un-tone-mapped");
        compare("ACEScg", "Rec.1886 Rec.709 - Display", "Un-tone-mapped");

        // Camera-encoded working space is delivered to the effect directly.
        settings.workingSpace = "ARRI LogC3 (EI800)";
        settings.view = "ACES 2.0 - SDR 100 nits (Rec.709)";
        require(engine.setColorSettings(settings), "log working space rejected");
        render::ShaderParser parser;
        render::ImageEffect effect;
        effect.setShaderDefinition(
            parser.parse("vec4 effect(vec4 c, ivec2 p, ivec2 s) { return vec4(c.rgb + vec3(0.02), c.a); }"));
        require(parser.isValid(), "log effect shader invalid");
        layer.setImageEffect(effect);
        layer.setInputColorSpace("ACEScg");
        const auto logResult = core::ImageBuffer::convert(offscreen.render(), core::ImageFormat::Float, 4);
        require(logResult.isAllocated(), qPrintable(engine.error().message()));
        auto logView = ocio::DisplayViewTransform::Create();
        logView->setSrc("ARRI LogC3 (EI800)");
        logView->setDisplay(settings.display.toUtf8().constData());
        logView->setView(settings.view.toUtf8().constData());
        const auto toLog = config->getProcessor("ACEScg", "ARRI LogC3 (EI800)")->getDefaultCPUProcessor();
        const auto fromLog = config->getProcessor(logView)->getDefaultCPUProcessor();
        const auto* logPixels = reinterpret_cast<const float*>(logResult.data());
        for (int pixel = 0; pixel < 8; ++pixel) {
            float reference[3] = { values[4 * pixel], values[4 * pixel + 1], values[4 * pixel + 2] };
            toLog->applyRGB(reference);
            for (auto& c : reference)
                c += 0.02f;
            fromLog->applyRGB(reference);
            for (int c = 0; c < 3; ++c)
                require(std::isfinite(logPixels[4 * pixel + c])
                            && std::abs(logPixels[4 * pixel + c] - reference[c]) < 0.015f,
                        "fx did not operate in the selected log working encoding");
        }
        layer.setImageEffect(render::ImageEffect());
        settings.workingSpace = "ACEScg";

        // A nonlinear display transform must run after scene-linear compositing.
        auto bottom = image;
        bottom.detach();
        auto top = image;
        top.detach();
        auto* bottomPixels = reinterpret_cast<float*>(bottom.data());
        auto* topPixels = reinterpret_cast<float*>(top.data());
        for (int pixel = 0; pixel < 8; ++pixel) {
            for (int c = 0; c < 3; ++c) {
                bottomPixels[4 * pixel + c] = 0.1f;
                topPixels[4 * pixel + c] = 0.8f;
            }
            bottomPixels[4 * pixel + 3] = 1.0f;
            topPixels[4 * pixel + 3] = 0.5f;
        }
        render::ImageLayer bottomLayer, topLayer;
        bottomLayer.setImage(bottom);
        topLayer.setImage(top);
        bottomLayer.setInputColorSpace("ACEScg");
        topLayer.setInputColorSpace("ACEScg");
        engine.setImageLayers({ bottomLayer, topLayer });
        settings.view = "ACES 2.0 - SDR 100 nits (Rec.709)";
        require(engine.setColorSettings(settings), "composite settings failed");
        const auto composite = core::ImageBuffer::convert(offscreen.render(), core::ImageFormat::Float, 4);
        require(composite.isAllocated(), "composite readback failed");
        auto transform = ocio::DisplayViewTransform::Create();
        transform->setSrc("ACEScg");
        transform->setDisplay(settings.display.toUtf8().constData());
        transform->setView(settings.view.toUtf8().constData());
        const auto output = config->getProcessor(transform)->getDefaultCPUProcessor();
        float reference[3] = { 0.45f, 0.45f, 0.45f };
        output->applyRGB(reference);
        const auto* compositePixels = reinterpret_cast<const float*>(composite.data());
        require(std::abs(compositePixels[0] - reference[0]) < 0.005f, "display transform ran before compositing");
        engine.setImageLayers({ topLayer });
        const auto translucent = core::ImageBuffer::convert(offscreen.render(), core::ImageFormat::Float, 4);
        require(translucent.isAllocated(), "translucent readback failed");
        float straight[3] = { 0.8f, 0.8f, 0.8f };
        output->applyRGB(straight);
        const auto* translucentPixels = reinterpret_cast<const float*>(translucent.data());
        require(std::abs(translucentPixels[0] - straight[0] * 0.5f) < 0.005f
                    && std::abs(translucentPixels[3] - 0.5f) < 0.001f,
                "premultiplied output was transformed incorrectly");
        engine.setImageLayers({ layer });
        const auto valid = engine.colorSettings();
        settings.workingSpace = "not-a-space";
        require(!engine.setColorSettings(settings) && engine.colorSettings() == valid,
                "invalid setting replaced state");
        settings = valid;
        require(engine.setColorSettings(settings), "settings did not recover");
        layer.setInputColorSpace("not-a-source");
        require(!offscreen.render().isAllocated() && engine.error().hasError(), "invalid source rendered silently");
        compare("ACEScg", "Rec.1886 Rec.709 - Display", "Un-tone-mapped");

        // Independent AWG3/gamma combinations: compare GPU math to a matrix and
        // explicit scalar power, rather than using the generated profile as oracle.
        const auto awg24 = render::ColorPipeline::combinedColorSpace("ARRI Wide Gamut 3", "Gamma 2.4");
        const auto toAwg = config->getProcessor("ACEScg", "Linear ARRI Wide Gamut 3")->getDefaultCPUProcessor();
        layer.setInputColorSpace("ACEScg");
        settings.workingSpace = awg24;
        settings.display = "Raw";
        settings.view = "Un-tone-mapped";
        require(engine.setColorSettings(settings), "independent working pair rejected");
        const auto checkAwg = [&]() {
            const auto result = core::ImageBuffer::convert(offscreen.render(), core::ImageFormat::Float, 4);
            require(result.isAllocated(), qPrintable(engine.error().message()));
            const auto* pixels = reinterpret_cast<const float*>(result.data());
            for (int pixel = 0; pixel < 8; ++pixel) {
                float rgb[] { values[4 * pixel], values[4 * pixel + 1], values[4 * pixel + 2] };
                toAwg->applyRGB(rgb);
                for (int c = 0; c < 3; ++c) {
                    const float expected = rgb[c] < 0 ? rgb[c] : std::pow(rgb[c], 1.0f / 2.4f);
                    require(std::abs(pixels[4 * pixel + c] - expected) < 0.008f,
                            "independent AWG3 / Gamma 2.4 differs from reference");
                }
            }
        };
        checkAwg();
        settings.workingSpace = "ACEScg";
        settings.display = awg24;
        require(engine.setColorSettings(settings), "independent output pair rejected");
        checkAwg();

        // Switching the project aspect after initialization must resize the scene
        // texture. Fit it into a matching offscreen target to inspect its mattes.
        render::RenderOffscreen project;
        require(project.initialize(QSize(8, 8)), "project target initialization failed");
        project.setRenderEngine(&engine);
        engine.setResolution(QSize(8, 8));
        engine.setBackground(Qt::black);
        settings.workingSpace = "ARRI LogC3 (EI800)";
        settings.display = "Rec.1886 Rec.709 - Display";
        settings.view = "ACES 2.0 - SDR 100 nits (Rec.709)";
        require(engine.setColorSettings(settings), "matte settings rejected");
        const auto matte = core::ImageBuffer::convert(project.render(), core::ImageFormat::Float, 4);
        require(matte.isAllocated(), "project resize render failed");
        const auto* mattePixels = reinterpret_cast<const float*>(matte.data());
        for (int c = 0; c < 3; ++c)
            require(std::abs(mattePixels[c]) < 0.0001f, "color processing brightened the project matte");

        // Same GPU device, resolution only: a 16x8 project fills this 8x8
        // viewport. A stale 8x8 scene would retain transparent top/bottom mattes.
        engine.setBackground(Qt::transparent);
        engine.setResolution(QSize(16, 8));
        const auto wide = core::ImageBuffer::convert(project.render(), core::ImageFormat::Float, 4);
        require(wide.isAllocated(), "same-device project resize failed");
        require(reinterpret_cast<const float*>(wide.data())[3] > 0.99f, "scene texture retained its previous size");

        // Render again on the original device after a resolution-only change.
        engine.setResolution(bounds.size());
        settings.workingSpace = "ACEScg";
        settings.view = "Un-tone-mapped";
        require(engine.setColorSettings(settings), "project settings did not recover");
        const auto resized = core::ImageBuffer::convert(offscreen.render(), core::ImageFormat::Float, 4);
        require(resized.isAllocated(), "resolution-only change failed");
        const auto* resizedPixels = reinterpret_cast<const float*>(resized.data());
        require(resizedPixels[3] > 0.99f && resizedPixels[0] > 0.1f, "old project texture survived resize");
        return 0;
    } catch (const std::exception& error) {
        qCritical() << error.what();
        return 1;
    }
}
