// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class SideCarPrivate;

/**
 * @class Sidecar
 * @brief Represents an external metadata container associated with a media resource.
 *
 * The Sidecar class manages auxiliary data that is stored separately from the
 * primary media file. This is commonly used for non-destructive metadata,
 * sidecar captions, or application-specific markers (e.g., .xmp or .xml files).
 * * @note This class uses implicit sharing via QExplicitlySharedDataPointer for
 * efficient copying and resource management.
 */
class FLIPMANSDK_EXPORT SideCar {
public:
    /**
     * @brief Constructs an empty Sidecar object.
     */
    SideCar();

    /**
     * @brief Copy constructor. Performs a shallow copy of the sidecar data.
     */
    SideCar(const SideCar& other);

    /**
     * @brief Destroys the Sidecar object.
     */
    virtual ~SideCar();

    /**
     * @brief Returns true if the sidecar contains valid metadata and is linked correctly.
     */
    bool isValid() const;

    /**
     * @brief Clears all metadata and resets the sidecar to an uninitialized state.
     */
    void reset();

    /** @name Operators */
    ///@{
    SideCar& operator=(const SideCar& other);
    ///@}

private:
    QExplicitlySharedDataPointer<SideCarPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::SideCar)
