// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "testplay.h"
#include <core/file.h>
#include <core/os.h>
#include <plugins/mediareader.h>
#include <plugins/pluginregistry.h>
#include <widgets/renderwidget.h>

#include <QFrame>
#include <QFuture>
#include <QPainter>
#include <QPalette>
#include <QVBoxLayout>
#include <QtConcurrent>

Testplay::Testplay(QWidget* parent)
    : QWidget(parent)
{
    QSize size(1920 / 2.0f, 1080 / 2.0f);
    qDebug() << "widget size: " << size;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    QFrame* frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setLineWidth(1);
    frame->setMidLineWidth(0);

    widgets::RenderWidget* renderWidget = new widgets::RenderWidget(frame);
    viewport->set_resolution(size / 2.0f);  // half default size
    qDebug() << "renderWidget resolution: " << viewport->resolution();

    if (1) {
        QRect datawindow(QPoint(0, 0), size * 2.0f);
        qDebug() << "datawindow: " << datawindow;
        QRect displaywindow = datawindow;

        core::ImageFormat format(core::ImageFormat::UINT8);
        core::ImageBuffer image(datawindow, displaywindow, format, 3);

        quint8* data = image.data();
        size_t stride = image.stridesize();
        size_t pixelsize = image.pixelsize();

        const int checker = 50;
        for (int y = 0; y < datawindow.height(); ++y) {
            for (int x = 0; x < datawindow.width(); ++x) {
                bool iswhite = ((x / checker) % 2) == ((y / checker) % 2);
                quint8 color = iswhite ? 255 : 0;
                quint8* pixel = data + (y * stride) + (x * pixelsize);
                for (size_t c = 0; c < image.channels(); ++c) {
                    pixel[c] = color;
                }
            }
        }

        av::RenderLayer renderlayer;
        renderlayer.set_image(image);

        // av::RenderEffect effect(av::Gaussian)
        // effect->set_parameter("kernel", 10.0f);
        // renderlayer->set_effect(effect);

        viewport->set_renderlayers(QList<av::RenderLayer>({ renderlayer }));
    }
    else {
        QString resourcepath = "../../data";
        core::File file = core::OS::resourcepath(QString("%1/23 967 fps 24 fps timecode.mp4").arg(resourcepath));
        Q_ASSERT("file does not exist" && file.exists());

        QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
            plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
            QScopedPointer<plugins::MediaReader> reader(registry->get_plugin<plugins::MediaReader>(file.extension()));
            Q_ASSERT("not reader found for extension" && reader);

            if (reader->open(file)) {
                av::Time time = reader->read();
                core::ImageBuffer image = reader->image();

                qDebug() << time.to_string();
                qDebug() << image.datawindow().width() << ", " << image.datawindow().height();
            }
            else {
                qDebug() << "could not open file: " << file << ", error: " << reader->error();
            }
        });
        future.waitForFinished();
    }

    QVBoxLayout* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addWidget(viewport);

    layout->addWidget(frame);
    resize(size);  // show half of HD
}

Testplay::~Testplay() {}
