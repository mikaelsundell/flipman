// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "core/session.h"
#include "metadataview.h"
#include "ui_metadataview.h"

#include <QHeaderView>
#include <QPointer>

namespace flipman {

class MetadataViewPrivate {
public:
    void refresh();

    struct Data {
        Ui::MetadataView ui;
        QPointer<MetadataView> view;
    };
    Data d;
};

void
MetadataViewPrivate::refresh()
{
    d.ui.properties->clear();
    const auto metadata = flipman::session()->metadata();
    if (!flipman::session()->isLoaded())
        return;

    auto* source = new QTreeWidgetItem(d.ui.properties, { "Source" });
    new QTreeWidgetItem(source, { "File", flipman::session()->filename() });
    new QTreeWidgetItem(source, { "Title", flipman::session()->title() });
    new QTreeWidgetItem(source, { "Resolution", QString("%1 x %2")
                                                    .arg(flipman::session()->image().displayWindow().width())
                                                    .arg(flipman::session()->image().displayWindow().height()) });
    new QTreeWidgetItem(source, { "Frames", QString::number(flipman::session()->frameCount()) });
    new QTreeWidgetItem(source,
                        { "Source FPS", QString::number(flipman::session()->timeline()->fps().real(), 'g', 7) });
    new QTreeWidgetItem(source, { "Source timecode",
                                  sdk::av::SmpteTime(flipman::session()->timeline()->startTime()).toString() });
    if (!metadata.keys().isEmpty()) {
        auto* common = new QTreeWidgetItem(d.ui.properties, { "Metadata" });
        for (const auto& key : metadata.keys()) {
            const auto value = metadata.value(key).toString();
            auto* property = new QTreeWidgetItem(common, { key, value });
            property->setToolTip(1, value);
        }
    }
    for (const auto group : metadata.groups()) {
        auto* item = new QTreeWidgetItem(d.ui.properties, { sdk::core::MetaData::convert(group) });
        for (const auto& key : metadata.keys(group)) {
            const auto value = metadata.value(group, key);
            auto* property = new QTreeWidgetItem(item, { key, value.toString() });
            property->setToolTip(1, value.toString());
        }
    }
    d.ui.properties->expandToDepth(0);
}

MetadataView::MetadataView(QWidget* parent)
    : QWidget(parent)
    , p(new MetadataViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    p->d.ui.properties->setColumnCount(2);
    p->d.ui.properties->setHeaderLabels({ "Name", "Value" });
    p->d.ui.properties->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    bindSession();
}

MetadataView::~MetadataView() = default;

void
MetadataView::bindSession()
{
    auto* session = flipman::session();
    connect(session, &Session::mediaChanged, this, [this]() { p->refresh(); });
    p->refresh();
}

}  // namespace flipman
