// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/render/displaycolorspace.h>
#include <QDebug>

namespace flipman::sdk::render {

namespace {

    QColorSpace::Primaries toPrimaries(ColorSpace colorSpace)
    {
        switch (colorSpace) {
        case ColorSpace::Unknown:
        case ColorSpace::Raw:
        case ColorSpace::Rec709: return QColorSpace::Primaries::SRgb;

        case ColorSpace::DisplayP3:
        case ColorSpace::DCIP3: return QColorSpace::Primaries::DciP3D65;

        case ColorSpace::Rec2020: return QColorSpace::Primaries::Bt2020;

        case ColorSpace::Rec601:
            qWarning() << "displaycolorspace: Rec.601 primaries cannot be represented by QColorSpace::Primaries,"
                       << "falling back to sRGB primaries for display tagging";
            return QColorSpace::Primaries::SRgb;

        case ColorSpace::ACEScg:
            qWarning() << "displaycolorspace: ACEScg/AP1 primaries cannot be represented by QColorSpace::Primaries,"
                       << "falling back to sRGB primaries for display tagging";
            return QColorSpace::Primaries::SRgb;
        }

        qWarning() << "displaycolorspace: unknown color space for display primaries,"
                   << "falling back to sRGB primaries"
                   << "colorSpace:" << int(colorSpace);

        return QColorSpace::Primaries::SRgb;
    }

    QColorSpace toDisplayColorSpace(ColorSpace colorSpace, TransferFunction transferFunction)
    {
        if (colorSpace == ColorSpace::DisplayP3 && transferFunction == TransferFunction::SRGB) {
            return QColorSpace(QColorSpace::NamedColorSpace::DisplayP3);
        }

        if ((colorSpace == ColorSpace::Rec709 || colorSpace == ColorSpace::Unknown)
            && transferFunction == TransferFunction::SRGB) {
            return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
        }

        if ((colorSpace == ColorSpace::Rec709 || colorSpace == ColorSpace::Unknown)
            && transferFunction == TransferFunction::Linear) {
            return QColorSpace(QColorSpace::NamedColorSpace::SRgbLinear);
        }

        const QColorSpace::Primaries primaries = toPrimaries(colorSpace);

        switch (transferFunction) {
        case TransferFunction::Gamma22: return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.2f);

        case TransferFunction::Gamma24: return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.4f);

        case TransferFunction::Gamma25: return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.5f);

        case TransferFunction::Gamma26: return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.6f);

        case TransferFunction::Linear: return QColorSpace(primaries, QColorSpace::TransferFunction::Linear);

        case TransferFunction::SRGB: return QColorSpace(primaries, QColorSpace::TransferFunction::SRgb);

        case TransferFunction::Unknown:
        case TransferFunction::Raw:
            qWarning() << "displaycolorspace: transfer function cannot be represented as a display color space,"
                       << "falling back to sRGB"
                       << "transferFunction:" << int(transferFunction);
            return QColorSpace(QColorSpace::NamedColorSpace::SRgb);

        case TransferFunction::ACEScc:
        case TransferFunction::ACEScct:
        case TransferFunction::ADX10:
        case TransferFunction::ADX16:
        case TransferFunction::Cineon:
        case TransferFunction::AppleLog:
        case TransferFunction::ArriLogC3:
        case TransferFunction::ArriLogC4:
        case TransferFunction::BmdFilmGen5:
        case TransferFunction::DaVinciIntermediate:
        case TransferFunction::CanonLog2:
        case TransferFunction::CanonLog3:
        case TransferFunction::PanasonicVLog:
        case TransferFunction::RedLog3G10:
        case TransferFunction::SonySLog3:
            qWarning() << "displaycolorspace: log/camera transfer function cannot be used as a display color space tag,"
                       << "falling back to sRGB"
                       << "transferFunction:" << int(transferFunction);
            return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
        }

        qWarning() << "displaycolorspace: unknown transfer function for display color space,"
                   << "falling back to sRGB"
                   << "transferFunction:" << int(transferFunction);

        return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
    }

}  // namespace

DisplayColorSpace::DisplayColorSpace()
    : DisplayColorSpace(QColorSpace(QColorSpace::SRgb))
{}

DisplayColorSpace::DisplayColorSpace(const QColorSpace& colorSpace)
    : m_colorSpace(colorSpace)
    , m_iccProfile(colorSpace.iccProfile())
{}

DisplayColorSpace::DisplayColorSpace(const DisplayTransform& transform)
    : DisplayColorSpace(toDisplayColorSpace(transform.colorSpace, transform.transferFunction))
{}

DisplayColorSpace
DisplayColorSpace::fromIccProfile(const QByteArray& profile)
{
    return DisplayColorSpace(QColorSpace::fromIccProfile(profile));
}

bool
DisplayColorSpace::isValid() const
{
    return m_colorSpace.isValid();
}

QString
DisplayColorSpace::description() const
{
    return m_colorSpace.description();
}

QByteArray
DisplayColorSpace::iccProfile() const
{
    return m_iccProfile;
}

QColorSpace
DisplayColorSpace::qtColorSpace() const
{
    return m_colorSpace;
}

void
DisplayColorSpace::applyTo(QSurfaceFormat& format) const
{
    format.setColorSpace(m_colorSpace);
}

bool
DisplayColorSpace::operator==(const DisplayColorSpace& other) const
{
    return m_colorSpace == other.m_colorSpace;
}

}  // namespace flipman::sdk::render
