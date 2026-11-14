// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "pythondialog.h"
#include "pythoninterpreter.h"
#include "ui_pythondialog.h"

#include <QPointer>

namespace flipman {

class PythonDialogPrivate {
public:
    struct Data {
        Ui::PythonDialog ui;
        QPointer<PythonDialog> dialog;
    };
    Data d;
};

PythonDialog::PythonDialog(QWidget* parent)
    : QDialog(parent)
    , p(new PythonDialogPrivate)
{
    p->d.dialog = this;
    p->d.ui.setupUi(this);
    resize(850, 600);
    p->d.ui.editorSplitter->setSizes({ 350, 200 });
    connect(p->d.ui.runButton, &QPushButton::clicked, this, [this]() {
        const auto output = flipman::pythonInterpreter()->executeScript(p->d.ui.editor->toPlainText());
        p->d.ui.output->appendPlainText(output.isEmpty() ? QStringLiteral("Done") : output);
    });
}

PythonDialog::~PythonDialog() = default;

}  // namespace flipman
