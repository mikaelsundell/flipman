// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "testgamut.h"
#include "gamutwidget.h"
#include <QFrame>
#include <QFuture>
#include <QPainter>
#include <QPalette>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <core/file.h>
#include <core/os.h>
#include <plugins/mediareader.h>
#include <plugins/pluginregistry.h>
#include <widgets/viewport.h>

Testgamut::Testgamut(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("testgamut");

    QSize size(1920 / 2.0f, 1080 / 2.0f);
    qDebug() << "widget size: " << size;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    QFrame* frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setLineWidth(1);
    frame->setMidLineWidth(0);

    GamutWidget* widget = new GamutWidget(frame);
    widget->set_resolution(size / 2.0f);  // half default size
    widget->set_background(Qt::blue);
    qDebug() << "gamutwidget resolution: " << widget->resolution();

    if (1) {
        // generate gamut - AWG1

        // generate gamut - AP1

        // generate gamut - AP0

        // add to list


        // add image

        QRect datawindow(0, 0, 64, 64);
        QRect displaywindow = datawindow;

        core::ImageFormat format(core::ImageFormat::FLOAT);
        core::ImageBuffer buffer(datawindow, displaywindow, format, 3);

        // Write RGB cube into a 2D 64×64 texture
        float* data = reinterpret_cast<float*>(buffer.data());
        size_t stride = buffer.stridesize() / sizeof(float);    // in floats
        size_t pixelsize = buffer.pixelsize() / sizeof(float);  // in floats

        int cubeSize = 16;
        int count = 0;

        for (int b = 0; b < cubeSize; ++b) {
            for (int g = 0; g < cubeSize; ++g) {
                for (int r = 0; r < cubeSize; ++r) {
                    float rf = r / float(cubeSize - 1);
                    float gf = g / float(cubeSize - 1);
                    float bf = b / float(cubeSize - 1);

                    int x = count % 64;
                    int y = count / 64;

                    float* pixel = data + y * stride + x * pixelsize;
                    pixel[0] = rf;
                    pixel[1] = gf;
                    pixel[2] = bf;

                    ++count;
                }
            }
        }

        widget->set_image(buffer);
    }

    QVBoxLayout* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addWidget(widget);

    layout->addWidget(frame);
    resize(size);  // show half of HD
}

Testgamut::~Testgamut() {}
