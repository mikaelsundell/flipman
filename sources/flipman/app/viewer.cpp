// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "application.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/session.h"
#include "core/settings.h"
#include "mcp/mcpserver.h"
#include "python/pythondialog.h"
#include "ui_viewer.h"
#include "viewer.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <algorithm>

namespace flipman {

class ViewerPrivate {
public:
    void init();
    void menus();
    void recentFiles();

    struct Data {
        Ui::ViewerWindow ui;
        QPointer<PythonDialog> python;
        QPointer<QMenu> recent;
        QPointer<QAction> undo;
        QPointer<QAction> redo;
        QPointer<QAction> files;
        QPointer<QAction> metadata;
        QPointer<Viewer> viewer;
    };
    Data d;
};

void
ViewerPrivate::init()
{
    d.ui.setupUi(d.viewer);
    d.viewer->setAcceptDrops(true);
    d.viewer->resize(1600, 950);
    d.ui.mainLayout->setStretch(3, 1);
    d.ui.viewsSplitter->setCollapsible(0, true);
    d.ui.viewsSplitter->setCollapsible(1, false);
    d.ui.viewsSplitter->setCollapsible(2, true);
    d.ui.viewsSplitter->setStretchFactor(0, 0);
    d.ui.viewsSplitter->setStretchFactor(1, 1);
    d.ui.viewsSplitter->setStretchFactor(2, 0);
    d.ui.viewsSplitter->setSizes({ 250, 1050, 280 });
    menus();
    QObject::connect(flipman::session(), &Session::statusChanged, d.viewer,
                     [this](const QString& text) { d.viewer->statusBar()->showMessage(text); });
    QObject::connect(flipman::session(), &Session::opened, d.viewer, [this](const QString& filename) {
        d.viewer->setWindowTitle("Flipman — " + flipman::session()->title());
        auto* preferences = flipman::settings();
        auto recent = preferences->value("recentFiles").toStringList();
        recent.removeAll(filename);
        recent.prepend(filename);
        while (recent.size() > 10)
            recent.removeLast();
        preferences->setValue("recentFiles", recent);
        recentFiles();
    });
    QObject::connect(flipman::session(), &Session::mediaChanged, d.viewer, [this]() {
        if (!flipman::session()->isLoaded())
            d.viewer->setWindowTitle("Flipman");
    });
    d.viewer->statusBar()->showMessage(flipman::session()->status());
    auto* preferences = flipman::settings();
    if (!preferences->value("viewer/geometry").isNull())
        d.viewer->restoreGeometry(preferences->value("viewer/geometry").toByteArray());
    if (!preferences->value("viewer/splitter").isNull())
        d.ui.viewsSplitter->restoreState(preferences->value("viewer/splitter").toByteArray());
}

void
ViewerPrivate::recentFiles()
{
    d.recent->clear();
    for (const auto& filename : flipman::settings()->value("recentFiles").toStringList()) {
        auto* action = d.recent->addAction(QFileInfo(filename).fileName());
        action->setToolTip(filename);
        QObject::connect(action, &QAction::triggered, d.viewer,
                         [this, filename]() { flipman::session()->openFile(filename); });
    }
    d.recent->setEnabled(!d.recent->actions().isEmpty());
}

void
ViewerPrivate::menus()
{
    auto* file = d.ui.menuBar->addMenu("File");
    auto* open = file->addAction("Open…");
    open->setObjectName("actionOpen");
    open->setShortcut(QKeySequence::Open);
    QObject::connect(open, &QAction::triggered, d.viewer, [this]() {
        const auto filename = QFileDialog::getOpenFileName(d.viewer, "Open movie or image");
        if (!filename.isEmpty())
            flipman::session()->openFile(filename);
    });
    d.recent = file->addMenu("Open Recent");
    recentFiles();
    auto* close = file->addAction("Close media");
    QObject::connect(close, &QAction::triggered, flipman::session(), &Session::closeFile);
    file->addSeparator();
    auto* quit = file->addAction("Quit Flipman");
    quit->setShortcut(QKeySequence::Quit);
    QObject::connect(quit, &QAction::triggered, d.viewer, &QWidget::close);
    auto* edit = d.ui.menuBar->addMenu("Edit");
    d.undo = edit->addAction("Undo");
    d.undo->setObjectName("actionUndo");
    d.undo->setShortcut(QKeySequence::Undo);
    d.redo = edit->addAction("Redo");
    d.redo->setObjectName("actionRedo");
    d.redo->setShortcut(QKeySequence::Redo);
    QObject::connect(d.undo, &QAction::triggered, flipman::session()->commands(), &CommandStack::undo);
    QObject::connect(d.redo, &QAction::triggered, flipman::session()->commands(), &CommandStack::redo);
    auto refresh = [this]() {
        d.undo->setEnabled(flipman::session()->commands()->canUndo());
        d.redo->setEnabled(flipman::session()->commands()->canRedo());
    };
    QObject::connect(flipman::session()->commands(), &CommandStack::changed, d.viewer, refresh);
    refresh();
    auto* view = d.ui.menuBar->addMenu("View");
    d.files = view->addAction("Files");
    d.files->setCheckable(true);
    d.files->setChecked(true);
    d.metadata = view->addAction("Metadata");
    d.metadata->setCheckable(true);
    d.metadata->setChecked(true);
    QObject::connect(d.files, &QAction::toggled, d.viewer, [this](bool visible) {
        auto sizes = d.ui.viewsSplitter->sizes();
        sizes[0] = visible ? 250 : 0;
        d.ui.viewsSplitter->setSizes(sizes);
    });
    QObject::connect(d.metadata, &QAction::toggled, d.viewer, [this](bool visible) {
        auto sizes = d.ui.viewsSplitter->sizes();
        sizes[2] = visible ? 280 : 0;
        d.ui.viewsSplitter->setSizes(sizes);
    });
    QObject::connect(d.ui.viewsSplitter, &QSplitter::splitterMoved, d.viewer, [this]() {
        const QSignalBlocker files(d.files), metadata(d.metadata);
        const auto sizes = d.ui.viewsSplitter->sizes();
        d.files->setChecked(sizes[0] > 0);
        d.metadata->setChecked(sizes[2] > 0);
    });
    auto* python = view->addAction("Python Interpreter…");
    python->setObjectName("actionPython");
    QObject::connect(python, &QAction::triggered, d.viewer, &Viewer::showPython);
    auto* mcp = view->addAction("MCP server");
    mcp->setObjectName("actionMcpServer");
    mcp->setCheckable(true);
    mcp->setChecked(flipman::mcpServer()->isRunning());
    QObject::connect(mcp, &QAction::toggled, d.viewer, [this, mcp](bool enabled) {
        if (enabled && !flipman::mcpServer()->start()) {
            const QSignalBlocker blocker(mcp);
            mcp->setChecked(false);
            d.viewer->statusBar()->showMessage(flipman::mcpServer()->errorString());
            return;
        }
        if (!enabled)
            flipman::mcpServer()->stop();
        flipman::settings()->setValue("mcp/enabled", enabled);
    });
    QObject::connect(flipman::mcpServer(), &McpServer::runningChanged, mcp, [mcp](bool enabled) {
        const QSignalBlocker blocker(mcp);
        mcp->setChecked(enabled);
        mcp->setToolTip(enabled ? flipman::mcpServer()->url().toString() : "Start the local MCP endpoint");
    });
    auto* playback = d.ui.menuBar->addMenu("Playback");
    auto* play = playback->addAction("Play / Pause");
    play->setShortcut(Qt::Key_Space);
    QObject::connect(play, &QAction::triggered, flipman::session(), &Session::togglePlaying);
    auto step = [this](qint64 offset) {
        if (!flipman::session()->isLoaded())
            return;

        auto* t = flipman::session()->timeline();
        auto next = sdk::av::Time::fromFrames(std::clamp<qint64>(t->time().frames() + offset, 0,
                                                                 flipman::session()->frameCount() - 1),
                                              t->fps());
        flipman::session()->commands()->execute(timeCommand(t->time(), next));
    };
    auto* previous = playback->addAction("Previous frame");
    previous->setShortcut(Qt::Key_Left);
    auto* next = playback->addAction("Next frame");
    next->setShortcut(Qt::Key_Right);
    QObject::connect(previous, &QAction::triggered, d.viewer, [step]() { step(-1); });
    QObject::connect(next, &QAction::triggered, d.viewer, [step]() { step(1); });
}

Viewer::Viewer(QWidget* parent)
    : QMainWindow(parent)
    , p(new ViewerPrivate)
{
    p->d.viewer = this;
    p->init();
}

Viewer::~Viewer() = default;

Session*
Viewer::session() const
{
    return flipman::session();
}

void
Viewer::setArguments(const QStringList& arguments)
{
    if (arguments.size() > 1)
        QTimer::singleShot(0, this, [this, path = arguments.at(1)]() { openFile(path); });
}

void
Viewer::openFile(const QString& filename)
{
    flipman::session()->openFile(filename);
}

void
Viewer::showPython()
{
    if (!p->d.python)
        p->d.python = new PythonDialog(this);
    p->d.python->show();
    p->d.python->raise();
    p->d.python->activateWindow();
}

void
Viewer::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->urls().size() == 1 && event->mimeData()->urls().first().isLocalFile())
        event->acceptProposedAction();
}

void
Viewer::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->urls().size() == 1 && event->mimeData()->urls().first().isLocalFile()) {
        openFile(event->mimeData()->urls().first().toLocalFile());
        event->acceptProposedAction();
    }
}

void
Viewer::closeEvent(QCloseEvent* event)
{
    flipman::session()->timeline()->stop();
    auto* preferences = flipman::settings();
    preferences->setValue("viewer/geometry", saveGeometry());
    preferences->setValue("viewer/splitter", p->d.ui.viewsSplitter->saveState());
    QMainWindow::closeEvent(event);
}

}  // namespace flipman
