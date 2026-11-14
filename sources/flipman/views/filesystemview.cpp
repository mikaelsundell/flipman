// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "core/session.h"
#include "filesystemview.h"
#include "ui_filesystemview.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QPointer>

namespace flipman {

class FileSystemViewPrivate {
public:
    struct Data {
        Ui::FileSystemView ui;
        QPointer<QFileSystemModel> model;
        QPointer<FileSystemView> view;
    };
    Data d;
};

FileSystemView::FileSystemView(QWidget* parent)
    : QWidget(parent)
    , p(new FileSystemViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    p->d.model = new QFileSystemModel(this);
    p->d.model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    p->d.ui.files->setModel(p->d.model);
    for (int column = 1; column < p->d.model->columnCount(); ++column)
        p->d.ui.files->hideColumn(column);
    p->d.ui.files->header()->hide();
    p->d.ui.directory->setText(QDir::homePath());
    auto browse = [this]() {
        const auto path = p->d.ui.directory->text();
        if (QFileInfo(path).isDir())
            p->d.ui.files->setRootIndex(p->d.model->setRootPath(path));
    };
    connect(p->d.ui.directory, &QLineEdit::returnPressed, this, browse);
    browse();
    bindSession();
}

FileSystemView::~FileSystemView() = default;

void
FileSystemView::bindSession()
{
    auto* session = flipman::session();
    connect(p->d.ui.files, &QTreeView::doubleClicked, this, [this, session](const QModelIndex& index) {
        const auto file = p->d.model->fileInfo(index);
        if (file.isFile())
            session->openFile(file.absoluteFilePath());
    });
    connect(session, &Session::opened, this, [this](const QString& filename) {
        const auto directory = QFileInfo(filename).absolutePath();
        p->d.ui.directory->setText(directory);
        p->d.ui.files->setRootIndex(p->d.model->setRootPath(directory));
    });
}

}  // namespace flipman
