// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderspec.h>

namespace flipman::sdk::render {

class RenderSpecPrivate : public QSharedData {
public:
    void init();
    struct Data {
        RenderSurface surface;
        QMatrix4x4 view;
        QSize size;
        QString lut;
    };
    Data d;
};

void
RenderSpecPrivate::init()
{
    d.view.setToIdentity();
}

RenderSpec::RenderSpec()
    : p(new RenderSpecPrivate())
{
    p->init();
}

RenderSpec::RenderSpec(const RenderSpec& other)
    : p(other.p)
{}

RenderSpec::~RenderSpec() {}

RenderSurface
RenderSpec::surface() const
{
    return p->d.surface;
}

void
RenderSpec::setSurface(const RenderSurface& surface)
{
    p->d.surface = surface;
}

QMatrix4x4
RenderSpec::view() const
{
    return p->d.view;
}

void
RenderSpec::setView(const QMatrix4x4& view)
{
    p->d.view = view;
}

QSize
RenderSpec::size() const
{
    return p->d.size;
}

void
RenderSpec::setSize(const QSize& size)
{
    p->d.size = size;
}

QString
RenderSpec::lut() const
{
    return p->d.lut;
}

void
RenderSpec::setLut(const QString& lut)
{
    p->d.lut = lut;
}

bool
RenderSpec::isValid() const
{
    return p->d.size.isValid();
}

void
RenderSpec::reset()
{
    p.reset(new RenderSpecPrivate());
    p->init();
}

RenderSpec&
RenderSpec::operator=(const RenderSpec& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
RenderSpec::operator==(const RenderSpec& other) const
{
    return (p->d.surface == other.p->d.surface && p->d.view == other.p->d.view && p->d.size == other.p->d.size
            && p->d.lut == other.p->d.lut);
}

bool
RenderSpec::operator!=(const RenderSpec& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::render
