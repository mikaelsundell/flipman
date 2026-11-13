// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "private/ociogpu.h"

#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace flipman::sdk::render {

namespace {

    // Decode-only curves supplied by the embedded config. Gamut matrices remain
    // independent; these transforms are assembled once, never evaluated on CPU pixels.
    const QMap<QString, QString> curveNames { { "Apple Log", "Apple Log - Curve" },
                                              { "LogC3 (EI800)", "ARRI LogC3 - Curve (EI800)" },
                                              { "LogC4", "ARRI LogC4 - Curve" },
                                              { "BMDFilm Gen5 Log", "BMDFilm Gen5 Log - Curve" },
                                              { "DaVinci Intermediate", "DaVinci Intermediate Log - Curve" },
                                              { "Canon Log2", "C-Log2 - Curve" },
                                              { "Canon Log3", "C-Log3 - Curve" },
                                              { "D-Log", "D-Log - Curve" },
                                              { "V-Log", "V-Log - Curve" },
                                              { "Log3G10", "Log3G10 - Curve" },
                                              { "S-Log3", "S-Log3 - Curve" },
                                              { "sRGB", "sRGB - Curve" },
                                              { "Rec.709", "Rec.709 - Curve" },
                                              { "PQ (ST 2084)", "ST-2084 - Curve" } };

    QString combinedName(const QString& linear, const QString& encoding)
    {
        return "Flipman/" + linear + "/" + encoding;
    }

    void addCombinations(const ocio::ConfigRcPtr& config)
    {
        // Only the bundled Studio config has the catalogued curve/linear definitions.
        QStringList linears { "ACES2065-1", "ACEScg" };
        for (int i = 0; i < config->getNumColorSpaces(); ++i) {
            const QString name = QString::fromUtf8(config->getColorSpaceNameByIndex(i));
            if (name.startsWith("Linear "))
                linears.append(name);
        }
        QMap<QString, ocio::ConstTransformRcPtr> curves;
        curves.insert("Linear", ocio::MatrixTransform::Create());
        for (auto it = curveNames.begin(); it != curveNames.end(); ++it) {
            const auto named = config->getNamedTransform(it.value().toUtf8().constData());
            auto curve = named->getTransform(ocio::TRANSFORM_DIR_FORWARD);
            if (!curve) {
                auto inverse = named->getTransform(ocio::TRANSFORM_DIR_INVERSE)->createEditableCopy();
                inverse->setDirection(ocio::GetInverseTransformDirection(inverse->getDirection()));
                curve = inverse;
            }
            curves.insert(it.key(), curve);
        }
        for (double gamma : { 1.8, 2.2, 2.4, 2.6 }) {
            auto curve = ocio::ExponentTransform::Create();
            const double values[] { gamma, gamma, gamma, 1.0 };
            curve->setValue(values);
            curve->setNegativeStyle(ocio::NEGATIVE_PASS_THRU);
            curves.insert(QString("Gamma %1").arg(gamma), curve);
        }
        for (const auto& encoding : { "ACEScc", "ACEScct" })
            curves.insert(encoding, config->getProcessor(encoding, "ACEScg")->createGroupTransform());
        for (const auto& linear : linears) {
            for (auto it = curves.begin(); it != curves.end(); ++it) {
                auto transform = ocio::GroupTransform::Create();
                transform->appendTransform(it.value()->createEditableCopy());
                auto matrix = ocio::ColorSpaceTransform::Create();
                matrix->setSrc(linear.toUtf8().constData());
                matrix->setDst("ACES2065-1");
                transform->appendTransform(matrix);
                auto space = ocio::ColorSpace::Create(ocio::REFERENCE_SPACE_SCENE);
                space->setName(combinedName(linear, it.key()).toUtf8().constData());
                space->setFamily(config->getColorSpace(linear.toUtf8().constData())->getFamily());
                space->setTransform(transform, ocio::COLORSPACE_DIR_TO_REFERENCE);
                config->addColorSpace(space);
            }
        }
    }

}  // namespace

bool
ColorSettings::operator==(const ColorSettings& other) const
{
    return enabled == other.enabled && configFile == other.configFile && workingSpace == other.workingSpace
           && display == other.display && view == other.view;
}

ocio::ConstConfigRcPtr
ocioConfig(const QString& filename)
{
    static std::mutex mutex;
    static std::map<QString, ocio::ConstConfigRcPtr> configs;
    const std::lock_guard<std::mutex> lock(mutex);
    const QString path = filename.isEmpty() ? QString() : QFileInfo(filename).absoluteFilePath();
    const auto found = configs.find(path);
    if (found != configs.end())
        return found->second;

    ocio::ConfigRcPtr config;
    if (path.isEmpty()) {
        QFile file(":/flipmansdk/configs/studio-config-v4.0.0_aces-v2.0_ocio-v2.5.ocio");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("embedded OCIO config unavailable");
        std::istringstream stream(file.readAll().toStdString());
        config = ocio::Config::CreateFromStream(stream)->createEditableCopy();
    }
    else {
        config = ocio::Config::CreateFromFile(path.toUtf8().constData())->createEditableCopy();
    }
    if (path.isEmpty())
        addCombinations(config);
    config->validate();
    configs.emplace(path, config);
    return config;
}

QStringList
ColorPipeline::colorSpaces(const QString& filename)
{
    auto config = ocioConfig(filename);
    QStringList result;
    for (int i = 0; i < config->getNumColorSpaces(); ++i)
        result << QString::fromUtf8(config->getColorSpaceNameByIndex(i));
    return result;
}

QList<ColorProfile>
ColorPipeline::colorProfiles(const QString& filename)
{
    // Known gamut/curve labels; processing always uses the exact OCIO name.
    static const struct {
        const char* name;
        const char* gamut;
        const char* encoding;
    } labels[] = {

        { "ACES2065-1", "ACES AP0", "Linear" },
        { "ap0-linear", "ACES AP0", "Linear" },
        { "rec709-gamma24", "Rec.709", "Gamma 2.4" },
        { "ACEScg", "ACES AP1", "Linear" },
        { "ACEScc", "ACES AP1", "ACEScc" },
        { "ACEScct", "ACES AP1", "ACEScct" },
        { "ADX10", "ADX density", "ADX10" },
        { "ADX16", "ADX density", "ADX16" },
        { "Apple Log", "Rec.2020", "Apple Log" },
        { "ARRI LogC3 (EI800)", "ARRI Wide Gamut 3", "LogC3 (EI800)" },
        { "ARRI LogC4", "ARRI Wide Gamut 4", "LogC4" },
        { "BMDFilm WideGamut Gen5", "BMD WideGamut Gen5", "BMDFilm Gen5 Log" },
        { "DaVinci Intermediate WideGamut", "DaVinci WideGamut", "DaVinci Intermediate" },
        { "CanonLog2 CinemaGamut D55", "CinemaGamut D55", "Canon Log2" },
        { "CanonLog3 CinemaGamut D55", "CinemaGamut D55", "Canon Log3" },
        { "D-Log D-Gamut", "D-Gamut", "D-Log" },
        { "V-Log V-Gamut", "V-Gamut", "V-Log" },
        { "Log3G10 REDWideGamutRGB", "REDWideGamutRGB", "Log3G10" },
        { "Camera Rec.709", "Rec.709", "Rec.709" },
        { "CIE XYZ-D65 - Scene-referred", "CIE XYZ D65", "Linear" },
        { "Raw", "Raw / data", "Untransformed" },
        { "CIE XYZ-D65 - Display-referred", "CIE XYZ D65", "Linear" },
        { "sRGB - Display", "Rec.709", "sRGB" },
        { "Gamma 2.2 Rec.709 - Display", "Rec.709", "Gamma 2.2" },
        { "Rec.1886 Rec.709 - Display", "Rec.709", "Gamma 2.4" },
        { "Display P3 - Display", "P3-D65", "sRGB" },
        { "Display P3 HDR - Display", "P3-D65", "Extended sRGB (HDR)" },
        { "P3-D65 - Display", "P3-D65", "Gamma 2.6" },
        { "ST2084-P3-D65 - Display", "P3-D65", "PQ (ST 2084)" },
        { "Rec.2100-HLG - Display", "Rec.2020", "HLG" },
        { "Rec.2100-PQ - Display", "Rec.2020", "PQ (ST 2084)" },
        { "S-Log3 S-Gamut3", "S-Gamut3", "S-Log3" },
        { "S-Log3 S-Gamut3.Cine", "S-Gamut3.Cine", "S-Log3" },
        { "S-Log3 Venice S-Gamut3", "Venice S-Gamut3", "S-Log3" },
        { "S-Log3 Venice S-Gamut3.Cine", "Venice S-Gamut3.Cine", "S-Log3" },
        { "Linear ARRI Wide Gamut 3", "ARRI Wide Gamut 3", "Linear" },
        { "Linear ARRI Wide Gamut 4", "ARRI Wide Gamut 4", "Linear" },
        { "Linear BMD WideGamut Gen5", "BMD WideGamut Gen5", "Linear" },
        { "Linear DaVinci WideGamut", "DaVinci WideGamut", "Linear" },
        { "Linear CinemaGamut D55", "CinemaGamut D55", "Linear" },
        { "Linear D-Gamut", "D-Gamut", "Linear" },
        { "Linear V-Gamut", "V-Gamut", "Linear" },
        { "Linear REDWideGamutRGB", "REDWideGamutRGB", "Linear" },
        { "Linear S-Gamut3", "S-Gamut3", "Linear" },
        { "Linear S-Gamut3.Cine", "S-Gamut3.Cine", "Linear" },
        { "Linear Venice S-Gamut3", "Venice S-Gamut3", "Linear" },
        { "Linear Venice S-Gamut3.Cine", "Venice S-Gamut3.Cine", "Linear" },
        { "Linear AdobeRGB", "AdobeRGB", "Linear" },
        { "Linear P3-D65", "P3-D65", "Linear" },
        { "Linear Rec.2020", "Rec.2020", "Linear" },
        { "Linear Rec.709 (sRGB)", "Rec.709", "Linear" },
        { "sRGB Encoded Rec.709 (sRGB)", "Rec.709", "sRGB" },
        { "sRGB Encoded P3-D65", "P3-D65", "sRGB" },
        { "Gamma 2.2 Encoded AdobeRGB", "AdobeRGB", "Gamma 2.2" },
        { "sRGB Encoded AP1", "ACES AP1", "sRGB" },
        { "Gamma 2.2 Encoded AP1", "ACES AP1", "Gamma 2.2" },
        { "Gamma 1.8 Encoded Rec.709", "Rec.709", "Gamma 1.8" },
        { "Gamma 2.2 Encoded Rec.709", "Rec.709", "Gamma 2.2" },
        { "Gamma 2.4 Encoded Rec.709", "Rec.709", "Gamma 2.4" },

    };

    QList<ColorProfile> result;
    for (const auto& name : colorSpaces(filename)) {
        ColorProfile profile { name, name, "As configured", colorSpaceFamily(name, filename) };
        for (const auto& label : labels) {
            if (name == QLatin1String(label.name)) {
                profile.gamut = QString::fromUtf8(label.gamut);
                profile.encoding = QString::fromUtf8(label.encoding);
                break;
            }
        }
        profile.gamut.remove(" - Display");
        result.append(profile);
    }

    // Generated identifiers retain the same explicit gamut labels as their
    // linear base. The shipped config and its original identifiers stay intact.
    for (auto& profile : result) {
        if (!profile.name.startsWith("Flipman/"))
            continue;
        const auto parts = profile.name.split('/');
        for (const auto& base : result) {
            if (base.name == parts.value(1)) {
                profile.gamut = base.gamut;
                break;
            }
        }
        profile.encoding = parts.value(2);
    }
    return result;
}

QString
ColorPipeline::combinedColorSpace(const QString& gamut, const QString& encoding, const QString& filename)
{
    const auto config = ocioConfig(filename);
    QString fallback;
    for (const auto& profile : colorProfiles(filename)) {
        if (profile.gamut != gamut || profile.encoding != encoding)
            continue;
        const auto space = config->getColorSpace(profile.name.toUtf8().constData());
        if (space->getReferenceSpaceType() == ocio::REFERENCE_SPACE_SCENE)
            return profile.name;

        fallback = profile.name;
    }
    if (!fallback.isEmpty())
        return fallback;

    throw std::runtime_error("unsupported gamut/encoding combination");
}

QString
ColorPipeline::colorSpaceFamily(const QString& name, const QString& filename)
{
    const auto space = ocioConfig(filename)->getColorSpace(name.toUtf8().constData());
    if (!space)
        throw std::runtime_error("unknown OCIO color space");
    return QString::fromUtf8(space->getFamily());
}

QStringList
ColorPipeline::workingSpaces(const QString& filename)
{
    const auto config = ocioConfig(filename);
    QStringList result;
    for (int i = 0; i < config->getNumColorSpaces(); ++i) {
        const auto name = config->getColorSpaceNameByIndex(i);
        const auto space = config->getColorSpace(name);
        if (space && !space->isData() && space->getReferenceSpaceType() == ocio::REFERENCE_SPACE_SCENE)
            result << QString::fromUtf8(name);
    }
    return result;
}

QStringList
ColorPipeline::displays(const QString& filename)
{
    auto config = ocioConfig(filename);
    QStringList result;
    for (int i = 0; i < config->getNumDisplays(); ++i)
        result << QString::fromUtf8(config->getDisplay(i));
    return result;
}

QStringList
ColorPipeline::views(const QString& display, const QString& filename)
{
    auto config = ocioConfig(filename);
    const QString target = config->getNumViews(display.toUtf8().constData()) > 0
                               ? display
                               : QStringLiteral("Rec.1886 Rec.709 - Display");
    QStringList result;
    for (int i = 0; i < config->getNumViews(target.toUtf8().constData()); ++i)
        result << QString::fromUtf8(config->getView(target.toUtf8().constData(), i));
    return result;
}

QString
ColorPipeline::inputColorSpace(const core::ImageBuffer& image)
{
    const auto gamut = image.colorSpace();
    const auto curve = image.transferFunction();
    if (curve == TransferFunction::AppleLog)
        return "Apple Log";

    if (gamut == ColorSpace::ACEScg && curve == TransferFunction::Linear)
        return "ACEScg";

    if (gamut == ColorSpace::Rec709) {
        switch (curve) {
        case TransferFunction::Linear: return "Linear Rec.709 (sRGB)";
        case TransferFunction::SRGB: return "sRGB - Display";
        case TransferFunction::Gamma22: return "Gamma 2.2 Rec.709 - Display";
        case TransferFunction::Gamma24: return "Rec.1886 Rec.709 - Display";
        default: return {};
        }
    }
    if (gamut == ColorSpace::DisplayP3 && curve == TransferFunction::SRGB)
        return "Display P3 - Display";

    if (gamut == ColorSpace::DisplayP3 && curve == TransferFunction::Linear)
        return "Linear P3-D65";

    if (gamut == ColorSpace::Rec2020 && curve == TransferFunction::Linear)
        return "Linear Rec.2020";

    return {};
}

bool
ColorPipeline::displayTag(const QString& display, DisplayTransform& tag)
{
    if (display == "sRGB - Display")
        tag = { ColorSpace::Rec709, TransferFunction::SRGB };
    else if (display == "Rec.1886 Rec.709 - Display")
        tag = { ColorSpace::Rec709, TransferFunction::Gamma24 };
    else if (display == "Gamma 2.2 Rec.709 - Display")
        tag = { ColorSpace::Rec709, TransferFunction::Gamma22 };
    else if (display == "Display P3 - Display")
        tag = { ColorSpace::DisplayP3, TransferFunction::SRGB };
    else
        return false;

    return true;
}

}  // namespace flipman::sdk::render

namespace flipman::sdk::render {

ocio::ConstProcessorRcPtr
ocioOutputProcessor(const ocio::ConstConfigRcPtr& config, const ColorSettings& settings)
{
    if (settings.display == "Raw" || settings.view == "Raw")
        return config->getProcessor(ocio::MatrixTransform::Create());

    const QString view = settings.view.isEmpty() ? QStringLiteral("Un-tone-mapped") : settings.view;
    const bool native = config->getNumViews(settings.display.toUtf8().constData()) > 0;
    if (!native && view == "Un-tone-mapped")
        return config->getProcessor(settings.workingSpace.toUtf8().constData(), settings.display.toUtf8().constData());

    auto transform = ocio::DisplayViewTransform::Create();
    transform->setSrc(settings.workingSpace.toUtf8().constData());
    transform->setDisplay(native ? settings.display.toUtf8().constData() : "Rec.1886 Rec.709 - Display");
    transform->setView(view.toUtf8().constData());
    if (native)
        return config->getProcessor(transform);

    // Use the selected ACES rendering transform, then encode its display-linear
    // result with the requested independent gamut and curve in the same GPU pass.
    auto group = ocio::GroupTransform::Create();
    group->appendTransform(transform);
    auto encode = ocio::ColorSpaceTransform::Create();
    encode->setSrc("Rec.1886 Rec.709 - Display");
    encode->setDst(settings.display.toUtf8().constData());
    group->appendTransform(encode);
    return config->getProcessor(group);
}

}  // namespace flipman::sdk::render
