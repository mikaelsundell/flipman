// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"

#include <flipmansdk/widgets/renderwidget.h>

#include <QBoxLayout>
#include <QCoreApplication>
#include <QLabel>
#include <QPointer>
#include <QScrollArea>
#include <QSlider>

namespace flipman {
class WindowPrivate : public QObject {
public:
    WindowPrivate();
    void init();

public:
    struct Data {
        QString inputFile;
        QPointer<Window> window;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        d.inputFile = args.at(1);
        qDebug() << "input file detected:" << d.inputFile;
    }
    else {
        qFatal() << "no input file provided in arguments.";
    }



    QWidget* controlsWidget = new QWidget(d.window.data());
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);

    controlsWidget->setLayout(controlsLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    //mainLayout->addWidget(controlsWidget);
    //mainLayout->addWidget(scrollArea);

    QWidget* centralWidget = new QWidget(d.window.data());
    centralWidget->setLayout(mainLayout);

    d.window->setWindowTitle("testview");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(800, 600);
}

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}
}  // namespace flipman
