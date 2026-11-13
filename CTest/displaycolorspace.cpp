// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/render/displaycolorspace.h>
#include <QCoreApplication>
#include <QDebug>
#include <stdexcept>

using namespace flipman::sdk::render;

namespace {

void
require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

}  // namespace

int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        const DisplayColorSpace srgb;
        require(srgb.isValid() && srgb.qtColorSpace() == QColorSpace(QColorSpace::SRgb), "default profile is not sRGB");
        require(!srgb.iccProfile().isEmpty(), "default ICC data missing");
        require(DisplayColorSpace::fromIccProfile(srgb.iccProfile()) == srgb, "ICC roundtrip changed sRGB");
        const auto copy = srgb;
        require(copy == srgb && copy.iccProfile() == srgb.iccProfile(), "profile value copy changed ICC data");
        for (const auto& item : QList<QPair<TransferFunction, float>> { { TransferFunction::Gamma22, 2.2f },
                                                                        { TransferFunction::Gamma24, 2.4f },
                                                                        { TransferFunction::Gamma25, 2.5f },
                                                                        { TransferFunction::Gamma26, 2.6f } }) {
            const DisplayColorSpace profile(DisplayTransform { ColorSpace::Rec709, item.first });
            require(profile.qtColorSpace()
                        == QColorSpace(QColorSpace::Primaries::SRgb, QColorSpace::TransferFunction::Gamma, item.second),
                    "SDR gamma mapping changed");
            require(DisplayColorSpace::fromIccProfile(profile.iccProfile()) == profile,
                    "gamma ICC roundtrip changed profile");
        }
        const DisplayColorSpace p3(DisplayTransform { ColorSpace::DisplayP3, TransferFunction::SRGB });
        require(p3.qtColorSpace() == QColorSpace(QColorSpace::DisplayP3), "P3 mapping changed");
        require(p3 != srgb, "different display profiles compare equal");
        const DisplayColorSpace linear(DisplayTransform { ColorSpace::Rec709, TransferFunction::Linear });
        require(linear.qtColorSpace() == QColorSpace(QColorSpace::SRgbLinear), "linear mapping changed");
        const DisplayColorSpace wide(DisplayTransform { ColorSpace::Rec2020, TransferFunction::Gamma24 });
        require(wide.qtColorSpace()
                    == QColorSpace(QColorSpace::Primaries::Bt2020, QColorSpace::TransferFunction::Gamma, 2.4f),
                "Rec.2020 mapping changed");
        const DisplayColorSpace fallback(DisplayTransform { ColorSpace::Rec709, TransferFunction::ArriLogC3 });
        require(fallback == srgb, "unsupported camera display curve changed its fallback");
        require(!DisplayColorSpace::fromIccProfile("not an ICC profile").isValid(), "invalid ICC data accepted");
        QColorSpace named(QColorSpace::DisplayP3);
        named.setDescription("Custom display");
        require(DisplayColorSpace(named).description() == "Custom display", "profile description lost");
        const auto global = QSurfaceFormat::defaultFormat();
        auto format = global;
        format.setSamples(8);
        format.setDepthBufferSize(24);
        p3.applyTo(format);
        require(format.colorSpace() == p3.qtColorSpace() && format.samples() == 8 && format.depthBufferSize() == 24,
                "profile application modified unrelated format fields");
        require(QSurfaceFormat::defaultFormat() == global, "profile value modified the global surface format");
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
