// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/renderengine.h>

namespace flipman::sdk::av {
class RenderEnginePrivate : public QSharedData {
public:
    struct Data {};
    Data d;
};

RenderEngine::RenderEngine(QObject* parent)
    : p(new RenderEnginePrivate())
{}

RenderEngine::~RenderEngine() {}
}  // namespace flipman::sdk::av
