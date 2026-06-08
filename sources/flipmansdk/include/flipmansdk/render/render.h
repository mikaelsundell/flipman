// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QMetaType>

namespace flipman::sdk::render {

/**
 * @enum ColorSpace
 * @brief Describes the color primaries or color volume of an image signal.
 *
 * ColorSpace describes the chromaticity basis of the image values. It does not
 * describe whether the values are linear, gamma encoded, log encoded, or
 * display-referred. That is described separately by TransferFunction.
 */
enum class ColorSpace {
    Auto,       ///< Infer the color space from metadata or defaults.
    Raw,        ///< Do not interpret the color space; pass values through unchanged.
    Rec709,     ///< Rec.709 / BT.709 color primaries.
    DisplayP3,  ///< Display P3 color primaries.
    DCIP3,      ///< DCI-P3 color primaries.
    Rec2020,    ///< Rec.2020 / BT.2020 color primaries.
    ACEScg      ///< ACEScg / AP1 color primaries.
};

/**
 * @enum TransferFunction
 * @brief Describes the encoding curve of an image signal.
 *
 * TransferFunction describes how numeric image values are encoded. This includes
 * linear, display gamma, sRGB, log encodings, ACES grading encodings, and
 * camera/vendor transfer curves. It is separated from ColorSpace so combinations
 * such as Rec.709 gamma 2.4, Display P3 gamma 2.2, ARRI Wide Gamut LogC4, or
 * S-Gamut3.Cine S-Log3 can be represented without creating one enum value per
 * complete color encoding.
 */
enum class TransferFunction {
    Auto,                 ///< Infer the transfer function from metadata or defaults.
    Raw,                  ///< Do not apply transfer interpretation; pass values through unchanged.
    Linear,               ///< Scene-linear or display-linear values.
    SRGB,                 ///< sRGB transfer function.
    Gamma22,              ///< Pure gamma 2.2 transfer.
    Gamma24,              ///< Pure gamma 2.4 transfer.
    Gamma25,              ///< Pure gamma 2.5 transfer.
    Gamma26,              ///< Pure gamma 2.6 transfer.
    ACEScc,               ///< ACEScc logarithmic grading encoding.
    ACEScct,              ///< ACEScct logarithmic grading encoding with toe.
    ADX10,                ///< Academy Density Exchange 10-bit encoding.
    ADX16,                ///< Academy Density Exchange 16-bit encoding.
    Cineon,               ///< Cineon/log film scan encoding.
    AppleLog,             ///< Apple Log encoding.
    ArriLogC3,            ///< ARRI LogC3 encoding.
    ArriLogC4,            ///< ARRI LogC4 encoding.
    BmdFilmGen5,          ///< Blackmagic Design Film Gen5 encoding.
    DaVinciIntermediate,  ///< DaVinci Intermediate encoding.
    CanonLog2,            ///< Canon Log 2 encoding.
    CanonLog3,            ///< Canon Log 3 encoding.
    PanasonicVLog,        ///< Panasonic V-Log encoding.
    RedLog3G10,           ///< RED Log3G10 encoding.
    SonySLog3             ///< Sony S-Log3 encoding.
};

/****
 * @enum WorkingSpace
 * @brief Describes the internal color space used for rendering operations.
 *
 * WorkingSpace describes the scene-linear color space used by effects,
 * compositing, grading, and shader operations. Working spaces are
 * intentionally limited to rendering-oriented color spaces and do not
 * represent display-referred or log-encoded image encodings.
 */
enum class WorkingSpace {
    Rec709,            ///< Rec.709 / BT.709.
    DisplayP3,         ///< Display P3 / D65.
    DCIP3,             ///< DCI-P3.
    Rec2020,           ///< Rec.2020 / BT.2020.
    ACES2065_1,        ///< ACES2065-1 / AP0 interchange space.
    ACEScg,            ///< ACEScg / AP1 rendering space.
    ArriWideGamut3,    ///< ARRI Wide Gamut 3.
    ArriWideGamut4,    ///< ARRI Wide Gamut 4.
    BmdWideGamutGen5,  ///< Blackmagic Design Wide Gamut Gen5.
    DaVinciWideGamut,  ///< DaVinci Wide Gamut.
    CinemaGamutD55,    ///< Canon Cinema Gamut D55.
    VGamut,            ///< Panasonic V-Gamut.
    RedWideGamutRGB,   ///< REDWideGamutRGB.
    SGamut3,           ///< Sony S-Gamut3.
    SGamut3Cine,       ///< Sony S-Gamut3.Cine.
    VeniceSGamut3,     ///< Sony Venice S-Gamut3.
    VeniceSGamut3Cine  ///< Sony Venice S-Gamut3.Cine.
};

/**
 * @struct ColorTransform
 * @brief Describes a color transform between an encoded image signal and a working space.
 *
 * ColorTransform is used for source interpretation and output encoding. It
 * combines color primaries/gamut with a transfer function, which together
 * describe a complete image encoding such as Rec.709 Gamma 2.4, ARRI Wide
 * Gamut LogC4, or S-Gamut3.Cine S-Log3.
 */
struct ColorTransform {
    ColorSpace colorSpace = ColorSpace::Rec709;
    TransferFunction transferFunction = TransferFunction::Gamma24;
};

/**
 * @struct RenderTransform
 * @brief Describes the color pipeline used by the render engine.
 *
 * RenderTransform defines how input pixels are interpreted, which scene-linear
 * working space render operations use, and how the rendered result is encoded
 * before presentation or export.
 */
struct RenderTransform {
    ColorTransform input;
    WorkingSpace workingSpace = WorkingSpace::ACEScg;
    ColorTransform output;
};

/**
 * @struct DisplayTransform
 * @brief Describes the display/surface tag used for presenting rendered pixels.
 *
 * DisplayTransform does not perform color conversion. It describes the final
 * framebuffer to the window system, for example when tagging a CAMetalLayer.
 * The render engine must already have produced pixels matching this transform.
 */
struct DisplayTransform {
    ColorSpace colorSpace = ColorSpace::Rec709;
    TransferFunction transferFunction = TransferFunction::Gamma24;
};

}  // namespace flipman::sdk::render

Q_DECLARE_METATYPE(flipman::sdk::render::ColorSpace)
Q_DECLARE_METATYPE(flipman::sdk::render::TransferFunction)
Q_DECLARE_METATYPE(flipman::sdk::render::WorkingSpace)
