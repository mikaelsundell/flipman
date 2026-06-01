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
 * linear, display gamma, sRGB, log encodings, and scene/display-referred transfer
 * functions. It is separated from ColorSpace so combinations such as Rec.709
 * gamma 2.4, Display P3 gamma 2.2, or Rec.709 linear can be represented without
 * creating one enum value per combination.
 */
enum class TransferFunction {
    Auto,       ///< Infer the transfer function from metadata or defaults.
    Raw,        ///< Do not apply transfer interpretation; pass values through unchanged.
    Linear,     ///< Scene-linear or display-linear values.
    SRGB,       ///< sRGB transfer function.
    Gamma22,    ///< Pure gamma 2.2 transfer.
    Gamma24,    ///< Pure gamma 2.4 transfer.
    Gamma25,    ///< Pure gamma 2.5 transfer.
    Gamma26,    ///< Pure gamma 2.6 transfer.
    Cineon,     ///< Cineon/log film scan encoding.
    ArriLogC3,  ///< ARRI LogC3 encoding.
    ArriLogC4,  ///< ARRI LogC4 encoding.
    SonySLog3,  ///< Sony S-Log3 encoding.
    ACEScct     ///< ACEScct encoding.
};

/**
 * @enum WorkingSpace
 * @brief Describes the internal color space used for rendering operations.
 *
 * WorkingSpace describes the linear rendering space used by effects,
 * compositing, grading, and shader operations. Working spaces are intentionally
 * kept separate from display and export transforms because most rendering
 * operations should operate in a predictable linear space.
 */
enum class WorkingSpace {
    LinearRec709,     ///< Linear Rec.709 / BT.709 working space.
    LinearDisplayP3,  ///< Linear Display P3 working space.
    LinearRec2020,    ///< Linear Rec.2020 / BT.2020 working space.
    ACEScg            ///< ACEScg linear working space.
};

/**
 * @enum TransformRole
 * @brief Describes where a color space and transfer function pair is used.
 *
 * TransformRole can be used by UI, render settings, or color-management code to
 * distinguish between source interpretation, viewer output, and file export.
 */
enum class TransformRole {
    Input,    ///< Source/media interpretation before rendering.
    Display,  ///< Viewer/display encoding after rendering.
    Export    ///< File/output encoding when writing media.
};

}  // namespace flipman::sdk::render

Q_DECLARE_METATYPE(flipman::sdk::render::ColorSpace)
Q_DECLARE_METATYPE(flipman::sdk::render::TransferFunction)
Q_DECLARE_METATYPE(flipman::sdk::render::WorkingSpace)
Q_DECLARE_METATYPE(flipman::sdk::render::TransformRole)
