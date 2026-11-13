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
    /**
     * @brief Infer the color space from metadata or defaults.
     */
    Unknown,

    /**
     * @brief Do not interpret the color space; pass values through unchanged.
     */
    Raw,

    /**
     * @brief Rec.601 / BT.601 color primaries.
     */
    Rec601,

    /**
     * @brief Rec.709 / BT.709 color primaries.
     */
    Rec709,

    /**
     * @brief Display P3 color primaries.
     */
    DisplayP3,

    /**
     * @brief DCI-P3 color primaries.
     */
    DCIP3,

    /**
     * @brief Rec.2020 / BT.2020 color primaries.
     */
    Rec2020,

    /**
     * @brief ACEScg / AP1 color primaries.
     */
    ACEScg
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
    /**
     * @brief Infer the transfer function from metadata or defaults.
     */
    Unknown,

    /**
     * @brief Do not apply transfer interpretation; pass values through unchanged.
     */
    Raw,

    /**
     * @brief Scene-linear or display-linear values.
     */
    Linear,

    /**
     * @brief sRGB transfer function.
     */
    SRGB,

    /**
     * @brief Pure gamma 2.2 transfer.
     */
    Gamma22,

    /**
     * @brief Pure gamma 2.4 transfer.
     */
    Gamma24,

    /**
     * @brief Pure gamma 2.5 transfer.
     */
    Gamma25,

    /**
     * @brief Pure gamma 2.6 transfer.
     */
    Gamma26,

    /**
     * @brief ACEScc logarithmic grading encoding.
     */
    ACEScc,

    /**
     * @brief ACEScct logarithmic grading encoding with toe.
     */
    ACEScct,

    /**
     * @brief Academy Density Exchange 10-bit encoding.
     */
    ADX10,

    /**
     * @brief Academy Density Exchange 16-bit encoding.
     */
    ADX16,

    /**
     * @brief Cineon/log film scan encoding.
     */
    Cineon,

    /**
     * @brief Apple Log encoding.
     */
    AppleLog,

    /**
     * @brief ARRI LogC3 encoding.
     */
    ArriLogC3,

    /**
     * @brief ARRI LogC4 encoding.
     */
    ArriLogC4,

    /**
     * @brief Blackmagic Design Film Gen5 encoding.
     */
    BmdFilmGen5,

    /**
     * @brief DaVinci Intermediate encoding.
     */
    DaVinciIntermediate,

    /**
     * @brief Canon Log 2 encoding.
     */
    CanonLog2,

    /**
     * @brief Canon Log 3 encoding.
     */
    CanonLog3,

    /**
     * @brief Panasonic V-Log encoding.
     */
    PanasonicVLog,

    /**
     * @brief RED Log3G10 encoding.
     */
    RedLog3G10,

    /**
     * @brief Sony S-Log3 encoding.
     */
    SonySLog3
};

/**
 * @enum WorkingSpace
 * @brief Describes the internal color space used for rendering operations.
 *
 * WorkingSpace describes the scene-linear color space used by effects,
 * compositing, grading, and shader operations. Working spaces are
 * intentionally limited to rendering-oriented color spaces and do not
 * represent display-referred or log-encoded image encodings.
 */
enum class WorkingSpace {
    /**
     * @brief Infer the working space from metadata or defaults.
     */
    Unknown,

    /**
     * @brief Rec.601 / BT.601.
     */
    Rec601,

    /**
     * @brief Rec.709 / BT.709.
     */
    Rec709,

    /**
     * @brief Display P3 / D65.
     */
    DisplayP3,

    /**
     * @brief DCI-P3.
     */
    DCIP3,

    /**
     * @brief Rec.2020 / BT.2020.
     */
    Rec2020,

    /**
     * @brief ACES2065-1 / AP0 interchange space.
     */
    ACES2065_1,

    /**
     * @brief ACEScg / AP1 rendering space.
     */
    ACEScg,

    /**
     * @brief ARRI Wide Gamut 3.
     */
    ArriWideGamut3,

    /**
     * @brief ARRI Wide Gamut 4.
     */
    ArriWideGamut4,

    /**
     * @brief Blackmagic Design Wide Gamut Gen5.
     */
    BmdWideGamutGen5,

    /**
     * @brief DaVinci Wide Gamut.
     */
    DaVinciWideGamut,

    /**
     * @brief Canon Cinema Gamut D55.
     */
    CinemaGamutD55,

    /**
     * @brief Panasonic V-Gamut.
     */
    VGamut,

    /**
     * @brief REDWideGamutRGB.
     */
    RedWideGamutRGB,

    /**
     * @brief Sony S-Gamut3.
     */
    SGamut3,

    /**
     * @brief Sony S-Gamut3.Cine.
     */
    SGamut3Cine,

    /**
     * @brief Sony Venice S-Gamut3.
     */
    VeniceSGamut3,

    /**
     * @brief Sony Venice S-Gamut3.Cine.
     */
    VeniceSGamut3Cine
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
