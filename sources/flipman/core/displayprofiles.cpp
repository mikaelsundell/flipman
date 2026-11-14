// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "displayprofiles.h"

namespace flipman {

QList<DisplayProfile>
displayProfiles()
{
    using C = sdk::render::ColorSpace;
    using T = sdk::render::TransferFunction;
    return { { "sRGB", { C::Rec709, T::SRGB } },
             { "Rec.709 Gamma 2.2", { C::Rec709, T::Gamma22 } },
             { "Rec.709 Gamma 2.4", { C::Rec709, T::Gamma24 } },
             { "Display P3 sRGB", { C::DisplayP3, T::SRGB } },
             { "Display P3 Gamma 2.4", { C::DisplayP3, T::Gamma24 } },
             { "DCI-P3 Gamma 2.6", { C::DCIP3, T::Gamma26 } },
             { "Rec.2020 Gamma 2.4", { C::Rec2020, T::Gamma24 } } };
}

bool
findDisplayProfile(const QString& name, sdk::render::DisplayTransform& transform)
{
    for (const auto& profile : displayProfiles())
        if (profile.name == name) {
            transform = profile.transform;
            return true;
        }
    return false;
}

}  // namespace flipman
