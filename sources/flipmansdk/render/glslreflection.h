// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell
#pragma once

#include <QString>
#include <QVector>
#include <flipmansdk/render/shaderfunction.h>

namespace flipman::sdk::render {
// Reflects definitions, not prototypes. The shader compiler validates bodies.
bool
reflectGlslFunctions(const QString& source, QVector<ShaderFunction>& functions, QString& error);
}  // namespace flipman::sdk::render
