// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/metadata.h>

#include <QMap>
#include <QSharedData>
#include <QVariant>

namespace flipman::sdk::core {

class MetaDataPrivate : public QSharedData {
public:
    struct Data {
        QMap<QString, QVariant> common;
        QMap<MetaData::Group, QMap<QString, QVariant>> groups;
    };
    Data d;
};

MetaData::MetaData()
    : p(new MetaDataPrivate())
{}

MetaData::MetaData(const MetaData& other)
    : p(other.p)
{}

MetaData::~MetaData() {}

bool
MetaData::isValid() const
{
    if (!p)
        return false;
    if (!p->d.common.isEmpty())
        return true;
    for (auto it = p->d.groups.cbegin(); it != p->d.groups.cend(); ++it) {
        if (!it.value().isEmpty())
            return true;
    }
    return false;
}

QList<MetaData::Group>
MetaData::groups() const
{
    QList<Group> out;
    if (!p)
        return out;

    for (auto it = p->d.groups.cbegin(); it != p->d.groups.cend(); ++it) {
        if (!it.value().isEmpty())
            out.append(it.key());
    }
    return out;
}

QList<QString>
MetaData::keys(Group group) const
{
    if (!p)
        return {};
    return p->d.groups.value(group).keys();
}

QVariant
MetaData::value(const QString& key) const
{
    if (!p)
        return {};
    return p->d.common.value(key);
}

QVariant
MetaData::value(Group group, const QString& key) const
{
    if (!p)
        return {};
    const auto it = p->d.groups.constFind(group);
    if (it == p->d.groups.cend())
        return {};
    return it.value().value(key);
}

void
MetaData::insert(const QString& key, const QVariant& value)
{
    p.detach();
    p->d.common.insert(key, value);
}

void
MetaData::insert(Group group, const QString& key, const QVariant& value)
{
    p.detach();
    p->d.groups[group].insert(key, value);
}

void
MetaData::remove(const QString& key)
{
    p.detach();
    p->d.common.remove(key);
}

void
MetaData::remove(Group group, const QString& key)
{
    p.detach();
    auto it = p->d.groups.find(group);
    if (it == p->d.groups.end())
        return;

    it.value().remove(key);
    if (it.value().isEmpty())
        p->d.groups.erase(it);
}

void
MetaData::reset()
{
    p.reset(new MetaDataPrivate());
}

QVariant&
MetaData::operator[](const QString& key)
{
    p.detach();
    return p->d.common[key];
}

MetaData&
MetaData::operator=(const MetaData& other)
{
    if (this != &other)
        p = other.p;
    return *this;
}

QString
MetaData::convert(MetaData::Key key)
{
    switch (key) {
    case Title: return "Title";
    case Author: return "Author";
    case Command: return "Command";
    case Description: return "Description";
    default: return QString();
    }
}

QString
MetaData::convert(MetaData::Group group)
{
    switch (group) {
    case Group::Container: return "container";
    case Group::Video: return "video";
    case Group::Audio: return "audio";
    case Group::Camera: return "camera";
    case Group::Timecode: return "timecode";
    case Group::Production: return "production";
    case Group::Custom: return "custom";
    default: return QString();
    }
}

}  // namespace flipman::sdk::core
