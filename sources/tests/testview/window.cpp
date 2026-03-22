// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/plugins/imageeffectreader.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/widgets/viewer.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QMap>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <algorithm>
#include <cmath>

namespace flipman {

class WindowPrivate : public QObject {
public:
    WindowPrivate();

    void init();

    QWidget* createParameterPanel(QWidget* parent = nullptr);
    QWidget* createParameterWidget(const sdk::render::ShaderDescriptor::ShaderParameter& param,
                                   QWidget* parent = nullptr);

    QWidget* createFloatRow(const QString& label, const QString& name, double value, double minValue, double maxValue,
                            QWidget* parent = nullptr);

    QWidget* createIntRow(const QString& label, const QString& name, int value, int minValue, int maxValue,
                          QWidget* parent = nullptr);

    QWidget* createVec2Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                              const QVector2D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QWidget* createVec3Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                              const QVector3D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QWidget* createVec4Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                              const QVector4D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QDoubleSpinBox* createDoubleSpinBox(double value, double minValue, double maxValue, QWidget* parent = nullptr);
    QSpinBox* createIntSpinBox(int value, int minValue, int maxValue, QWidget* parent = nullptr);
    QSlider* createFloatSlider(double value, double minValue, double maxValue, QWidget* parent = nullptr);
    QSlider* createIntSlider(int value, int minValue, int maxValue, QWidget* parent = nullptr);

    void setVec2Component(const QString& name, int component, double value);
    void setVec3Component(const QString& name, int component, double value);
    void setVec4Component(const QString& name, int component, double value);

    void setParameterValue(const QString& name, const QVariant& value);
    QVariant parameterValue(const sdk::render::ShaderDescriptor::ShaderParameter& param) const;
    void updateViewer();

    struct Data {
        QString inputFile;
        QPointer<Window> window;
        QPointer<sdk::widgets::Viewer> viewer;
        sdk::render::ImageLayer imageLayer;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

QVariant
WindowPrivate::parameterValue(const sdk::render::ShaderDescriptor::ShaderParameter& param) const
{
    return param.value.isValid() ? param.value : param.defaultValue;
}

QSlider*
WindowPrivate::createFloatSlider(double value, double minValue, double maxValue, QWidget* parent)
{
    QSlider* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, 1000);

    const double range = maxValue - minValue;
    const int sliderValue = range > 0.0 ? int(((value - minValue) / range) * 1000.0) : 0;
    slider->setValue(std::clamp(sliderValue, 0, 1000));
    return slider;
}

QSlider*
WindowPrivate::createIntSlider(int value, int minValue, int maxValue, QWidget* parent)
{
    QSlider* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(minValue, maxValue);
    slider->setValue(value);
    return slider;
}

QDoubleSpinBox*
WindowPrivate::createDoubleSpinBox(double value, double minValue, double maxValue, QWidget* parent)
{
    QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
    spin->setDecimals(3);
    spin->setSingleStep(0.01);
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
    spin->setFixedWidth(90);
    spin->setAlignment(Qt::AlignRight);
    return spin;
}

QSpinBox*
WindowPrivate::createIntSpinBox(int value, int minValue, int maxValue, QWidget* parent)
{
    QSpinBox* spin = new QSpinBox(parent);
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
    spin->setFixedWidth(90);
    spin->setAlignment(Qt::AlignRight);
    return spin;
}

QWidget*
WindowPrivate::createFloatRow(const QString& label, const QString& name, double value, double minValue, double maxValue,
                              QWidget* parent)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel* title = new QLabel(label, row);
    title->setFixedWidth(80);

    QSlider* slider = createFloatSlider(value, minValue, maxValue, row);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QDoubleSpinBox* spin = createDoubleSpinBox(value, minValue, maxValue, row);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout->addWidget(title);
    layout->addWidget(slider, 1);
    layout->addWidget(spin);

    connect(slider, &QSlider::valueChanged, this, [this, name, slider, spin, minValue, maxValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;

        QSignalBlocker blocker(spin);
        spin->setValue(v);
        setParameterValue(name, v);
    });

    connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this, name, slider, minValue, maxValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;

                QSignalBlocker blocker(slider);
                slider->setValue(std::clamp(s, 0, 1000));
                setParameterValue(name, v);
            });

    return row;
}

QWidget*
WindowPrivate::createIntRow(const QString& label, const QString& name, int value, int minValue, int maxValue,
                            QWidget* parent)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel* title = new QLabel(label, row);
    title->setMinimumWidth(80);

    QSlider* slider = createIntSlider(value, minValue, maxValue, row);
    QSpinBox* spin = createIntSpinBox(value, minValue, maxValue, row);

    layout->addWidget(title);
    layout->addWidget(slider, 1);
    layout->addWidget(spin);

    connect(slider, &QSlider::valueChanged, this, [this, name, spin](int v) {
        QSignalBlocker blocker(spin);
        spin->setValue(v);
        setParameterValue(name, v);
    });

    connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this, name, slider](int v) {
        QSignalBlocker blocker(slider);
        slider->setValue(v);
        setParameterValue(name, v);
    });

    return row;
}

QWidget*
WindowPrivate::createVec2Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                                const QVector2D& value, double minValue, double maxValue, QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QLabel* title = new QLabel(label, widget);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    layout->addWidget(title);

    QDoubleSpinBox* xSpin = nullptr;
    QDoubleSpinBox* ySpin = nullptr;
    QSlider* xSlider = nullptr;
    QSlider* ySlider = nullptr;

    QWidget* xRow = new QWidget(widget);
    QHBoxLayout* xLayout = new QHBoxLayout(xRow);
    xLayout->setContentsMargins(0, 0, 0, 0);
    xLayout->setSpacing(10);
    QLabel* xLabel = new QLabel("X", xRow);
    xLabel->setMinimumWidth(80);
    xSlider = createFloatSlider(value.x(), minValue, maxValue, xRow);
    xSpin = createDoubleSpinBox(value.x(), minValue, maxValue, xRow);
    xLayout->addWidget(xLabel);
    xLayout->addWidget(xSlider, 1);
    xLayout->addWidget(xSpin);

    QWidget* yRow = new QWidget(widget);
    QHBoxLayout* yLayout = new QHBoxLayout(yRow);
    yLayout->setContentsMargins(0, 0, 0, 0);
    yLayout->setSpacing(10);
    QLabel* yLabel = new QLabel("Y", yRow);
    yLabel->setMinimumWidth(80);
    ySlider = createFloatSlider(value.y(), minValue, maxValue, yRow);
    ySpin = createDoubleSpinBox(value.y(), minValue, maxValue, yRow);
    yLayout->addWidget(yLabel);
    yLayout->addWidget(ySlider, 1);
    yLayout->addWidget(ySpin);

    layout->addWidget(xRow);
    layout->addWidget(yRow);

    auto updateValue = [this, name = param.name, xSpin, ySpin]() {
        setParameterValue(name, QVariant::fromValue(QVector2D(float(xSpin->value()), float(ySpin->value()))));
    };

    connect(xSlider, &QSlider::valueChanged, this, [this, name = param.name, xSpin, minValue, maxValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(xSpin);
        xSpin->setValue(v);
        setVec2Component(name, 0, v);
    });

    connect(ySlider, &QSlider::valueChanged, this, [this, name = param.name, ySpin, minValue, maxValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(ySpin);
        ySpin->setValue(v);
        setVec2Component(name, 1, v);
    });

    connect(xSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this, name = param.name, xSlider, minValue, maxValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(xSlider);
                xSlider->setValue(std::clamp(s, 0, 1000));
                setVec2Component(name, 0, v);
            });

    connect(ySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this, name = param.name, ySlider, minValue, maxValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(ySlider);
                ySlider->setValue(std::clamp(s, 0, 1000));
                setVec2Component(name, 1, v);
            });

    return widget;
}

QWidget*
WindowPrivate::createVec3Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                                const QVector3D& value, double minValue, double maxValue, QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QLabel* title = new QLabel(label, widget);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    layout->addWidget(title);

    QDoubleSpinBox* xSpin = nullptr;
    QDoubleSpinBox* ySpin = nullptr;
    QDoubleSpinBox* zSpin = nullptr;
    QSlider* xSlider = nullptr;
    QSlider* ySlider = nullptr;
    QSlider* zSlider = nullptr;

    auto createComponentRow = [this, widget, minValue, maxValue](const QString& name, double v, QSlider*& slider,
                                                                 QDoubleSpinBox*& spin) {
        QWidget* row = new QWidget(widget);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        QLabel* label = new QLabel(name, row);
        label->setMinimumWidth(80);
        slider = createFloatSlider(v, minValue, maxValue, row);
        spin = createDoubleSpinBox(v, minValue, maxValue, row);

        rowLayout->addWidget(label);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);
        return row;
    };

    layout->addWidget(createComponentRow("X", value.x(), xSlider, xSpin));
    layout->addWidget(createComponentRow("Y", value.y(), ySlider, ySpin));
    layout->addWidget(createComponentRow("Z", value.z(), zSlider, zSpin));

    auto updateValue = [this, name = param.name, xSpin, ySpin, zSpin]() {
        setParameterValue(name, QVariant::fromValue(
                                    QVector3D(float(xSpin->value()), float(ySpin->value()), float(zSpin->value()))));
    };

    connect(xSlider, &QSlider::valueChanged, this, [xSpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(xSpin);
        xSpin->setValue(v);
        updateValue();
    });

    connect(ySlider, &QSlider::valueChanged, this, [ySpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(ySpin);
        ySpin->setValue(v);
        updateValue();
    });

    connect(zSlider, &QSlider::valueChanged, this, [zSpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(zSpin);
        zSpin->setValue(v);
        updateValue();
    });

    connect(xSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [xSlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(xSlider);
                xSlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    connect(ySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [ySlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(ySlider);
                ySlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    connect(zSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [zSlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(zSlider);
                zSlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    return widget;
}

QWidget*
WindowPrivate::createVec4Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                                const QVector4D& value, double minValue, double maxValue, QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QLabel* title = new QLabel(label, widget);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    layout->addWidget(title);

    QDoubleSpinBox* xSpin = nullptr;
    QDoubleSpinBox* ySpin = nullptr;
    QDoubleSpinBox* zSpin = nullptr;
    QDoubleSpinBox* wSpin = nullptr;
    QSlider* xSlider = nullptr;
    QSlider* ySlider = nullptr;
    QSlider* zSlider = nullptr;
    QSlider* wSlider = nullptr;

    auto createComponentRow = [this, widget, minValue, maxValue](const QString& name, double v, QSlider*& slider,
                                                                 QDoubleSpinBox*& spin) {
        QWidget* row = new QWidget(widget);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        QLabel* label = new QLabel(name, row);
        label->setMinimumWidth(80);
        slider = createFloatSlider(v, minValue, maxValue, row);
        spin = createDoubleSpinBox(v, minValue, maxValue, row);

        rowLayout->addWidget(label);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);
        return row;
    };

    layout->addWidget(createComponentRow("X", value.x(), xSlider, xSpin));
    layout->addWidget(createComponentRow("Y", value.y(), ySlider, ySpin));
    layout->addWidget(createComponentRow("Z", value.z(), zSlider, zSpin));
    layout->addWidget(createComponentRow("W", value.w(), wSlider, wSpin));

    auto updateValue = [this, name = param.name, xSpin, ySpin, zSpin, wSpin]() {
        setParameterValue(name, QVariant::fromValue(QVector4D(float(xSpin->value()), float(ySpin->value()),
                                                              float(zSpin->value()), float(wSpin->value()))));
    };

    connect(xSlider, &QSlider::valueChanged, this, [xSpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(xSpin);
        xSpin->setValue(v);
        updateValue();
    });

    connect(ySlider, &QSlider::valueChanged, this, [ySpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(ySpin);
        ySpin->setValue(v);
        updateValue();
    });

    connect(zSlider, &QSlider::valueChanged, this, [zSpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(zSpin);
        zSpin->setValue(v);
        updateValue();
    });

    connect(wSlider, &QSlider::valueChanged, this, [wSpin, minValue, maxValue, updateValue](int s) {
        const double t = double(s) / 1000.0;
        const double v = minValue + (maxValue - minValue) * t;
        QSignalBlocker blocker(wSpin);
        wSpin->setValue(v);
        updateValue();
    });

    connect(xSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [xSlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(xSlider);
                xSlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    connect(ySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [ySlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(ySlider);
                ySlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    connect(zSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [zSlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(zSlider);
                zSlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    connect(wSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [wSlider, minValue, maxValue, updateValue](double v) {
                const double range = maxValue - minValue;
                const int s = range > 0.0 ? int(((v - minValue) / range) * 1000.0) : 0;
                QSignalBlocker blocker(wSlider);
                wSlider->setValue(std::clamp(s, 0, 1000));
                updateValue();
            });

    return widget;
}

void
WindowPrivate::setVec2Component(const QString& name, int component, double value)
{
    sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    sdk::render::ShaderDescriptor descriptor = definition.descriptor();

    const int index = descriptor.indexOf(name);
    if (index < 0)
        return;

    QVector2D v = descriptor.parameters[index].value.isValid()
                      ? descriptor.parameters[index].value.value<QVector2D>()
                      : descriptor.parameters[index].defaultValue.value<QVector2D>();

    if (component == 0)
        v.setX(float(value));
    else if (component == 1)
        v.setY(float(value));

    descriptor.parameters[index].value = QVariant::fromValue(v);
    definition.setDescriptor(descriptor);
    imageEffect.setShaderDefinition(definition);
    d.imageLayer.setImageEffect(imageEffect);

    qDebug() << "ui: setVec2Component" << name << v;
    updateViewer();
}

void
WindowPrivate::setVec3Component(const QString& name, int component, double value)
{
    sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    sdk::render::ShaderDescriptor descriptor = definition.descriptor();

    const int index = descriptor.indexOf(name);
    if (index < 0)
        return;

    QVector3D v = descriptor.parameters[index].value.isValid()
                      ? descriptor.parameters[index].value.value<QVector3D>()
                      : descriptor.parameters[index].defaultValue.value<QVector3D>();

    if (component == 0)
        v.setX(float(value));
    else if (component == 1)
        v.setY(float(value));
    else if (component == 2)
        v.setZ(float(value));

    descriptor.parameters[index].value = QVariant::fromValue(v);
    definition.setDescriptor(descriptor);
    imageEffect.setShaderDefinition(definition);
    d.imageLayer.setImageEffect(imageEffect);

    qDebug() << "ui: setVec3Component" << name << v;
    updateViewer();
}

void
WindowPrivate::setVec4Component(const QString& name, int component, double value)
{
    sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    sdk::render::ShaderDescriptor descriptor = definition.descriptor();

    const int index = descriptor.indexOf(name);
    if (index < 0)
        return;

    QVector4D v = descriptor.parameters[index].value.isValid()
                      ? descriptor.parameters[index].value.value<QVector4D>()
                      : descriptor.parameters[index].defaultValue.value<QVector4D>();

    if (component == 0)
        v.setX(float(value));
    else if (component == 1)
        v.setY(float(value));
    else if (component == 2)
        v.setZ(float(value));
    else if (component == 3)
        v.setW(float(value));

    descriptor.parameters[index].value = QVariant::fromValue(v);
    definition.setDescriptor(descriptor);
    imageEffect.setShaderDefinition(definition);
    d.imageLayer.setImageEffect(imageEffect);

    qDebug() << "ui: setVec4Component" << name << v;
    updateViewer();
}

void
WindowPrivate::setParameterValue(const QString& name, const QVariant& value)
{
    qDebug() << "ui: setParameterValue" << name << value;
    sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    sdk::render::ShaderDescriptor descriptor = definition.descriptor();

    const int index = descriptor.indexOf(name);
    if (index < 0) {
        qWarning() << "ui: parameter not found:" << name;
        return;
    }

    descriptor.parameters[index].value = value;
    definition.setDescriptor(descriptor);
    imageEffect.setShaderDefinition(definition);
    d.imageLayer.setImageEffect(imageEffect);

    updateViewer();
}

void
WindowPrivate::updateViewer()
{
    if (!d.viewer)
        return;

    d.viewer->setImageLayers({ d.imageLayer });
    d.viewer->update();
}

QWidget*
WindowPrivate::createParameterWidget(const sdk::render::ShaderDescriptor::ShaderParameter& param, QWidget* parent)
{
    using ShaderParameter = sdk::render::ShaderDescriptor::ShaderParameter;

    const QVariant value = parameterValue(param);
    const QString label = !param.label.isEmpty() ? param.label : param.name;

    switch (param.type) {
    case ShaderParameter::Type::Float: {
        const double current = value.toDouble();
        const double minValue = param.minValue.isValid() ? param.minValue.toDouble() : 0.0;
        const double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        return createFloatRow(label, param.name, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Int: {
        const int current = value.toInt();
        const int minValue = param.minValue.isValid() ? param.minValue.toInt() : 0;
        const int maxValue = param.maxValue.isValid() ? param.maxValue.toInt() : 100;
        return createIntRow(label, param.name, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Bool: {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);

        QLabel* title = new QLabel(label, row);
        title->setMinimumWidth(160);

        QCheckBox* box = new QCheckBox(row);
        box->setChecked(value.toBool());

        layout->addWidget(title);
        layout->addStretch();
        layout->addWidget(box);

        connect(box, &QCheckBox::toggled, this,
                [this, name = param.name](bool checked) { setParameterValue(name, checked); });

        return row;
    }

    case ShaderParameter::Type::Vec2: {
        const QVector2D current = value.value<QVector2D>();
        const double minValue = param.minValue.isValid() ? param.minValue.toDouble() : 0.0;
        const double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        return createVec2Widget(param, label, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Vec3: {
        const QVector3D current = value.value<QVector3D>();
        const double minValue = param.minValue.isValid() ? param.minValue.toDouble() : 0.0;
        const double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        return createVec3Widget(param, label, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Vec4: {
        const QVector4D current = value.value<QVector4D>();
        const double minValue = param.minValue.isValid() ? param.minValue.toDouble() : 0.0;
        const double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        return createVec4Widget(param, label, current, minValue, maxValue, parent);
    }
    }

    return new QWidget(parent);
}

QWidget*
WindowPrivate::createParameterPanel(QWidget* parent)
{
    QWidget* panel = new QWidget(parent);
    panel->setMinimumWidth(320);

    QVBoxLayout* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(panel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* content = new QWidget(scrollArea);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(12);

    const sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    const sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    const auto& parameters = definition.descriptor().parameters;

    QLabel* titleLabel = new QLabel(imageEffect.name().isEmpty() ? "Parameters" : imageEffect.name(), content);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);
    contentLayout->addWidget(titleLabel);

    QMap<QString, QGroupBox*> groupBoxes;
    QMap<QString, QVBoxLayout*> groupLayouts;

    for (const auto& param : parameters) {
        QWidget* editor = createParameterWidget(param, content);
        const QString groupName = param.group.trimmed();

        if (groupName.isEmpty()) {
            contentLayout->addWidget(editor);
            continue;
        }

        if (!groupBoxes.contains(groupName)) {
            QGroupBox* box = new QGroupBox(groupName, content);
            QVBoxLayout* boxLayout = new QVBoxLayout(box);
            boxLayout->setContentsMargins(8, 8, 8, 8);
            boxLayout->setSpacing(10);

            groupBoxes.insert(groupName, box);
            groupLayouts.insert(groupName, boxLayout);
            contentLayout->addWidget(box);
        }

        groupLayouts[groupName]->addWidget(editor);
    }

    contentLayout->addStretch();
    scrollArea->setWidget(content);
    panelLayout->addWidget(scrollArea);

    return panel;
}

void
WindowPrivate::init()
{
    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2)
        qFatal("No input file provided in arguments.");

    d.inputFile = args.at(1);

    const QString dataPath = sdk::core::Environment::resourcePath("../../../data");
#if (0)
    // rgb
    const QString filename = "23.967.00086400.exr";
    sdk::core::File file(QString("%1/exr/%2").arg(dataPath).arg(filename));
#else
    // nv12
    const QString filename = "square export 23.976 512x512.mov";
    sdk::core::File file(QString("%1/quicktime/%2").arg(sdk::core::Environment::resourcePath(dataPath)).arg(filename));
#endif

    Q_ASSERT_X(file.exists(), "RenderEngine::loadFile", "file does not exist");

    sdk::av::Media media;
    Q_ASSERT(media.open(file) && media.waitForOpened() && "could not open media");
    Q_ASSERT(media.isValid() && "error open media");

    media.read();
    sdk::core::ImageBuffer image = media.image();
    Q_ASSERT(image.isValid() && "image not valid");

    sdk::render::ImageLayer imageLayer;
    imageLayer.setImage(image);

    QString shader = "fx/debug.fx";
    sdk::core::File shaderFile(QString("%1/%2").arg(dataPath).arg(shader));
    Q_ASSERT(shaderFile.exists() && "fx file does not exist");

    QScopedPointer<sdk::plugins::ImageEffectReader> reader(
        sdk::core::pluginRegistry()->getPlugin<sdk::plugins::ImageEffectReader>(shaderFile.extension()));

    Q_ASSERT(reader && "no reader for fx extension");
    Q_ASSERT(reader->open(shaderFile) && "could not open reader for fx");

    sdk::render::ImageEffect imageEffect = reader->imageEffect();
    Q_ASSERT(imageEffect.isValid() && "image effect is not valid");

    imageLayer.setImageEffect(imageEffect);
    d.imageLayer = imageLayer;

    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, centralWidget);

    QWidget* leftWidget = new QWidget(splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* controlsWidget = new QWidget(leftWidget);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(8);

    QPushButton* fitButton = new QPushButton("Fit");
    fitButton->setShortcut(QKeySequence(Qt::Key_F));

    QPushButton* z25Button = new QPushButton("25%");
    z25Button->setShortcut(QKeySequence(Qt::Key_1));

    QPushButton* z50Button = new QPushButton("50%");
    z50Button->setShortcut(QKeySequence(Qt::Key_2));

    QPushButton* z75Button = new QPushButton("75%");
    z75Button->setShortcut(QKeySequence(Qt::Key_3));

    QPushButton* z100Button = new QPushButton("100%");
    z100Button->setShortcut(QKeySequence(Qt::Key_4));

    QLabel* zoomLabel = new QLabel("100%");
    zoomLabel->setMinimumWidth(60);
    zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    controlsLayout->addWidget(fitButton);
    controlsLayout->addWidget(z25Button);
    controlsLayout->addWidget(z50Button);
    controlsLayout->addWidget(z75Button);
    controlsLayout->addWidget(z100Button);
    controlsLayout->addStretch();
    controlsLayout->addWidget(zoomLabel);
    leftLayout->addWidget(controlsWidget);

    QFrame* viewerFrame = new QFrame(leftWidget);
    viewerFrame->setFrameShape(QFrame::NoFrame);
    viewerFrame->setObjectName("viewerFrame");
    viewerFrame->setStyleSheet("#viewerFrame {"
                               "  border: 1px solid #3a3a3a;"
                               "  background: #1e1e1e;"
                               "}");
    QVBoxLayout* frameLayout = new QVBoxLayout(viewerFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);

    d.viewer = new sdk::widgets::Viewer();
    d.viewer->setResolution(QSize(1920, 1080));
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d.viewer->setImageLayers({ d.imageLayer });

    frameLayout->addWidget(d.viewer);
    leftLayout->addWidget(viewerFrame);

    QSlider* timelineSlider = new QSlider(Qt::Horizontal);
    timelineSlider->setRange(0, 100);
    timelineSlider->setValue(0);
    timelineSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    timelineSlider->setFixedHeight(28);

    leftLayout->addWidget(timelineSlider);

    QWidget* parameterPanel = createParameterPanel(splitter);

    splitter->addWidget(leftWidget);
    splitter->addWidget(parameterPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({ 900, 320 });

    mainLayout->addWidget(splitter);

    connect(fitButton, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView);
    });

    connect(z25Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.25f);
    });

    connect(z50Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.5f);
    });

    connect(z75Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.75f);
    });

    connect(z100Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(1.0f);
    });

    connect(d.viewer, &sdk::widgets::Viewer::zoomChanged, zoomLabel, [zoomLabel](float zoom) {
        int percent = int(std::round(zoom * 100.0f));
        zoomLabel->setText(QString("%1%").arg(percent));
    });

    d.window->setWindowTitle("testview");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(1200, 700);
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
