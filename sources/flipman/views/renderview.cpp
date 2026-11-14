// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "core/session.h"
#include "renderview.h"
#include "ui_renderview.h"

#include <QColorDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QSignalBlocker>
#include <cmath>

namespace flipman {

class RenderViewPrivate : public QObject {
public:
    bool eventFilter(QObject* object, QEvent* event) override;
    void refresh();

    struct Data {
        Ui::RenderView ui;
        QPointer<sdk::widgets::Viewer> viewport;
        QPointer<RenderView> view;
    };
    Data d;
};

bool
RenderViewPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (object == d.viewport && (event->type() == QEvent::DragEnter || event->type() == QEvent::Drop)) {
        const QMimeData* mime = event->type() == QEvent::Drop ? static_cast<QDropEvent*>(event)->mimeData()
                                                              : static_cast<QDragEnterEvent*>(event)->mimeData();
        if (mime && mime->urls().size() == 1 && mime->urls().first().isLocalFile()
            && QFileInfo(mime->urls().first().toLocalFile()).isFile()) {
            if (event->type() == QEvent::Drop) {
                auto* drop = static_cast<QDropEvent*>(event);
                flipman::session()->openFile(mime->urls().first().toLocalFile());
                drop->acceptProposedAction();
            }
            else
                static_cast<QDragEnterEvent*>(event)->acceptProposedAction();
        }
        return true;
    }
    return QObject::eventFilter(object, event);
}

void
RenderViewPrivate::refresh()
{
    if (!flipman::session())
        return;

    d.viewport->setDisplayTransform(flipman::session()->displayTransform());
    d.viewport->update();
}

RenderView::RenderView(QWidget* parent)
    : QWidget(parent)
    , p(new RenderViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    p->d.viewport = new sdk::widgets::Viewer(p->d.ui.viewportContainer);
    p->d.viewport->setObjectName("metalViewport");
    p->d.ui.viewportLayout->addWidget(p->d.viewport);
    p->d.viewport->setAcceptDrops(true);
    p->d.viewport->installEventFilter(p.data());
    connect(p->d.ui.fitButton, &QPushButton::clicked, this, &RenderView::fitToView);
    for (const auto& entry : QList<QPair<QPushButton*, float>> {
             { p->d.ui.z25, .25f }, { p->d.ui.z50, .5f }, { p->d.ui.z75, .75f }, { p->d.ui.z100, 1.f } })
        connect(entry.first, &QPushButton::clicked, this,
                [this, zoom = entry.second]() { p->d.viewport->setZoom(zoom); });
    connect(p->d.viewport, &sdk::widgets::Viewer::zoomChanged, this,
            [this](float zoom) { p->d.ui.zoomValue->setText(QString("%1%").arg(int(std::round(zoom * 100)))); });
    p->d.ui.projectResolution->addItem("Auto (source)", QSize());
    for (const QSize size :
         { QSize(1280, 720), QSize(1920, 1080), QSize(2048, 1080), QSize(3840, 2160), QSize(4096, 2160) })
        p->d.ui.projectResolution->addItem(QString("%1 x %2").arg(size.width()).arg(size.height()), size);
    p->d.viewport->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(p->d.viewport, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        QMenu menu;
        auto* background = menu.addMenu("Timeline background");
        for (const auto& preset : QList<QPair<QString, QColor>> {
                 { "Black", Qt::black }, { "Dark gray", QColor(46, 46, 46) }, { "Gray", Qt::gray } }) {
            auto* action = background->addAction(preset.first);
            connect(action, &QAction::triggered, this,
                    [this, color = preset.second]() { flipman::session()->setBackground(color); });
        }
        auto* custom = background->addAction("Custom…");
        connect(custom, &QAction::triggered, this, [this]() {
            const auto color = QColorDialog::getColor(flipman::session()->renderEngine()->background(), this,
                                                      "Timeline background");
            if (color.isValid())
                flipman::session()->setBackground(color);
        });
        menu.exec(p->d.viewport->mapToGlobal(position));
    });
    bindSession();
}

RenderView::~RenderView()
{
    p->d.viewport->removeEventFilter(p.data());
    p->d.viewport->setRenderEngine(nullptr);
}

sdk::widgets::Viewer*
RenderView::viewport() const
{
    return p->d.viewport;
}

void
RenderView::fitToView()
{
    p->d.viewport->setZoomMode(sdk::widgets::Viewer::FitToView);
}

void
RenderView::bindSession()
{
    auto* session = flipman::session();
    p->d.viewport->setRenderEngine(session->renderEngine());
    connect(session, &Session::frameChanged, p->d.viewport, qOverload<>(&QWidget::update));
    connect(session, &Session::colorChanged, p->d.viewport, qOverload<>(&QWidget::update));
    connect(session, &Session::displayChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::mediaChanged, this, [this]() {
        fitToView();
        p->refresh();
    });
    connect(p->d.ui.projectResolution, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, session]() {
        session->setProjectResolution(p->d.ui.projectResolution->currentData().toSize());
        fitToView();
    });
    p->refresh();
}

}  // namespace flipman
