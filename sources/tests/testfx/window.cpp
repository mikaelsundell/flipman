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
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
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
    void update();

public:
    QWidget* addParameterPanel(QWidget* parent = nullptr);
    QWidget* addParameterWidget(const sdk::render::ShaderDescriptor::ShaderParameter& param, QWidget* parent = nullptr);

    QWidget* addComboRow(const QString& label, const QString& name, const QVariant& value,
                         const QVector<sdk::render::ShaderDescriptor::ShaderOption>& options,
                         QWidget* parent = nullptr);

    QWidget* addFloatRow(const QString& label, const QString& name, double value, double minValue, double maxValue,
                         QWidget* parent = nullptr);

    QWidget* addIntRow(const QString& label, const QString& name, int value, int minValue, int maxValue,
                       QWidget* parent = nullptr);

    QWidget* addVec2Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                           const QVector2D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QWidget* addVec3Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                           const QVector3D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QWidget* addVec4Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
                           const QVector4D& value, double minValue, double maxValue, QWidget* parent = nullptr);

    QDoubleSpinBox* addDoubleSpinBox(double value, double minValue, double maxValue, QWidget* parent = nullptr);
    QSpinBox* addIntSpinBox(int value, int minValue, int maxValue, QWidget* parent = nullptr);
    QSlider* addFloatSlider(double value, double minValue, double maxValue, QWidget* parent = nullptr);
    QSlider* addIntSlider(int value, int minValue, int maxValue, QWidget* parent = nullptr);

    QStringList fxShaders() const;
    bool loadShader(const QString& shader);
    void rebuildParameterPanel();

    void setParameterValue(const QString& name, const QVariant& value);
    QVariant parameterValue(const sdk::render::ShaderDescriptor::ShaderParameter& param) const;

    void seekFrame(int frame);
    void updateTimelineLabel();

    struct Data {
        QString inputFile;
        QString dataPath;
        QString currentShader;
        QPointer<Window> window;
        QPointer<sdk::widgets::Viewer> viewer;
        QPointer<QSlider> timelineSlider;
        QPointer<QLabel> timelineLabel;
        QPointer<QSplitter> splitter;
        QPointer<QWidget> parameterPanel;
        QPointer<QWidget> parameterContainer;
        QPointer<QVBoxLayout> parameterLayout;
        sdk::av::Media media;
        sdk::av::TimeRange timeRange;
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

QStringList
WindowPrivate::fxShaders() const
{
    QDir dir(QString("%1/fx").arg(d.dataPath));

    QStringList files = dir.entryList({ "*.fx" }, QDir::Files, QDir::Name);
    for (QString& file : files)
        file = QString("fx/%1").arg(file);

    return files;
}

bool
WindowPrivate::loadShader(const QString& shader)
{
    sdk::core::File shaderFile(QString("%1/%2").arg(d.dataPath).arg(shader));

    if (!shaderFile.exists()) {
        qWarning() << "fx file does not exist:" << QString("%1/%2").arg(d.dataPath).arg(shader);
        return false;
    }

    QScopedPointer<sdk::plugins::ImageEffectReader> reader(
        sdk::core::pluginRegistry()->getPlugin<sdk::plugins::ImageEffectReader>(shaderFile.extension()));

    if (!reader) {
        qWarning() << "no reader for fx extension:" << shaderFile.extension();
        return false;
    }

    if (!reader->open(shaderFile)) {
        qWarning() << "could not open reader for fx:" << QString("%1/%2").arg(d.dataPath).arg(shader);
        return false;
    }

    sdk::render::ImageEffect imageEffect = reader->imageEffect();
    if (!imageEffect.isValid()) {
        qWarning() << "image effect is not valid:" << QString("%1/%2").arg(d.dataPath).arg(shader);
        return false;
    }

    d.imageLayer.setImageEffect(imageEffect);
    d.currentShader = shader;

    update();
    return true;
}

void
WindowPrivate::rebuildParameterPanel()
{
    if (!d.parameterContainer || !d.parameterLayout)
        return;

    if (d.parameterPanel) {
        d.parameterLayout->removeWidget(d.parameterPanel);
        delete d.parameterPanel;
        d.parameterPanel = nullptr;
    }

    d.parameterPanel = addParameterPanel(d.parameterContainer);
    d.parameterLayout->addWidget(d.parameterPanel);
}

QSlider*
WindowPrivate::addFloatSlider(double value, double minValue, double maxValue, QWidget* parent)
{
    QSlider* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, 1000);

    const double range = maxValue - minValue;
    const int sliderValue = range > 0.0 ? int(((value - minValue) / range) * 1000.0) : 0;
    slider->setValue(std::clamp(sliderValue, 0, 1000));

    return slider;
}

QSlider*
WindowPrivate::addIntSlider(int value, int minValue, int maxValue, QWidget* parent)
{
    QSlider* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(minValue, maxValue);
    slider->setValue(value);
    return slider;
}

QDoubleSpinBox*
WindowPrivate::addDoubleSpinBox(double value, double minValue, double maxValue, QWidget* parent)
{
    QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
    spin->setDecimals(3);
    spin->setSingleStep(0.01);
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
    spin->setFixedWidth(80);
    spin->setAlignment(Qt::AlignCenter);
    return spin;
}

QSpinBox*
WindowPrivate::addIntSpinBox(int value, int minValue, int maxValue, QWidget* parent)
{
    QSpinBox* spin = new QSpinBox(parent);
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
    spin->setFixedWidth(80);
    spin->setAlignment(Qt::AlignCenter);
    return spin;
}

QWidget*
WindowPrivate::addComboRow(const QString& label, const QString& name, const QVariant& value,
                           const QVector<sdk::render::ShaderDescriptor::ShaderOption>& options, QWidget* parent)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QLabel* title = new QLabel(label, row);
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QComboBox* combo = new QComboBox(row);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    int currentIndex = 0;

    for (int i = 0; i < options.size(); ++i) {
        const auto& option = options[i];
        const QString optionLabel = option.label.isEmpty() ? option.value.toString() : option.label;

        combo->addItem(optionLabel, option.value);

        if (option.value == value || option.value.toString() == value.toString())
            currentIndex = i;
    }

    combo->setCurrentIndex(currentIndex);

    layout->addWidget(title);
    layout->addWidget(combo, 1);

    connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, name, combo](int index) {
        if (index < 0)
            return;

        setParameterValue(name, combo->itemData(index));
    });

    return row;
}

QWidget*
WindowPrivate::addFloatRow(const QString& label, const QString& name, double value, double minValue, double maxValue,
                           QWidget* parent)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QLabel* title = new QLabel(label, row);
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QSlider* slider = addFloatSlider(value, minValue, maxValue, row);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QDoubleSpinBox* spin = addDoubleSpinBox(value, minValue, maxValue, row);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout->addWidget(title);
    layout->addWidget(slider, 1);
    layout->addWidget(spin);

    connect(slider, &QSlider::valueChanged, this, [this, name, spin, minValue, maxValue](int s) {
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
WindowPrivate::addIntRow(const QString& label, const QString& name, int value, int minValue, int maxValue,
                         QWidget* parent)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QLabel* title = new QLabel(label, row);
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QSlider* slider = addIntSlider(value, minValue, maxValue, row);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QSpinBox* spin = addIntSpinBox(value, minValue, maxValue, row);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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
WindowPrivate::addVec2Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
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
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(title);

    QDoubleSpinBox* xSpin = nullptr;
    QDoubleSpinBox* ySpin = nullptr;
    QSlider* xSlider = nullptr;
    QSlider* ySlider = nullptr;

    auto createComponentRow = [this, widget, minValue, maxValue](const QString& name, double v, QSlider*& slider,
                                                                 QDoubleSpinBox*& spin) {
        QWidget* row = new QWidget(widget);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(12);

        QLabel* componentLabel = new QLabel(name, row);
        componentLabel->setFixedWidth(80);
        componentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        slider = addFloatSlider(v, minValue, maxValue, row);
        slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        spin = addDoubleSpinBox(v, minValue, maxValue, row);
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        rowLayout->addWidget(componentLabel);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);

        return row;
    };

    layout->addWidget(createComponentRow("X", value.x(), xSlider, xSpin));
    layout->addWidget(createComponentRow("Y", value.y(), ySlider, ySpin));

    auto updateValue = [this, name = param.name, xSpin, ySpin]() {
        const QVector2D value(float(xSpin->value()), float(ySpin->value()));
        setParameterValue(name, QVariant::fromValue(value));
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

    return widget;
}

QWidget*
WindowPrivate::addVec3Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
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
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
        rowLayout->setSpacing(12);

        QLabel* componentLabel = new QLabel(name, row);
        componentLabel->setFixedWidth(80);
        componentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        slider = addFloatSlider(v, minValue, maxValue, row);
        slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        spin = addDoubleSpinBox(v, minValue, maxValue, row);
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        rowLayout->addWidget(componentLabel);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);

        return row;
    };

    layout->addWidget(createComponentRow("X", value.x(), xSlider, xSpin));
    layout->addWidget(createComponentRow("Y", value.y(), ySlider, ySpin));
    layout->addWidget(createComponentRow("Z", value.z(), zSlider, zSpin));

    auto updateValue = [this, name = param.name, xSpin, ySpin, zSpin]() {
        const QVector3D value(float(xSpin->value()), float(ySpin->value()), float(zSpin->value()));
        setParameterValue(name, QVariant::fromValue(value));
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
WindowPrivate::addVec4Widget(const sdk::render::ShaderDescriptor::ShaderParameter& param, const QString& label,
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
    title->setFixedWidth(80);
    title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
        rowLayout->setSpacing(12);

        QLabel* componentLabel = new QLabel(name, row);
        componentLabel->setFixedWidth(80);
        componentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        slider = addFloatSlider(v, minValue, maxValue, row);
        slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        spin = addDoubleSpinBox(v, minValue, maxValue, row);
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        rowLayout->addWidget(componentLabel);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);

        return row;
    };

    layout->addWidget(createComponentRow("X", value.x(), xSlider, xSpin));
    layout->addWidget(createComponentRow("Y", value.y(), ySlider, ySpin));
    layout->addWidget(createComponentRow("Z", value.z(), zSlider, zSpin));
    layout->addWidget(createComponentRow("W", value.w(), wSlider, wSpin));

    auto updateValue = [this, name = param.name, xSpin, ySpin, zSpin, wSpin]() {
        const QVector4D value(float(xSpin->value()), float(ySpin->value()), float(zSpin->value()),
                              float(wSpin->value()));
        setParameterValue(name, QVariant::fromValue(value));
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
WindowPrivate::setParameterValue(const QString& name, const QVariant& value)
{
    sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    sdk::render::ShaderDescriptor descriptor = definition.descriptor();

    const int index = descriptor.indexOf(name);
    if (index < 0) {
        qWarning() << "parameter not found:" << name;
        return;
    }

    descriptor.parameters[index].value = value;
    definition.setDescriptor(descriptor);
    imageEffect.setShaderDefinition(definition);
    d.imageLayer.setImageEffect(imageEffect);

    update();
}

QWidget*
WindowPrivate::addParameterWidget(const sdk::render::ShaderDescriptor::ShaderParameter& param, QWidget* parent)
{
    using ShaderParameter = sdk::render::ShaderDescriptor::ShaderParameter;

    const QVariant value = parameterValue(param);
    const QString label = !param.label.isEmpty() ? param.label : param.name;

    auto fixRange = [](double& minValue, double& maxValue) {
        if (minValue > maxValue)
            std::swap(minValue, maxValue);

        if (qAbs(maxValue - minValue) < 1e-12) {
            minValue = -1.0;
            maxValue = 1.0;
        }
    };

    if (param.hasOptions())
        return addComboRow(label, param.name, value, param.options, parent);

    switch (param.type) {
    case ShaderParameter::Type::Float: {
        const double current = value.toDouble();
        double minValue = param.minValue.isValid() ? param.minValue.toDouble() : 0.0;
        double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        fixRange(minValue, maxValue);

        return addFloatRow(label, param.name, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Int: {
        const int current = value.toInt();
        int minValue = param.minValue.isValid() ? param.minValue.toInt() : 0;
        int maxValue = param.maxValue.isValid() ? param.maxValue.toInt() : 100;

        if (minValue > maxValue)
            std::swap(minValue, maxValue);

        if (minValue == maxValue) {
            minValue = 0;
            maxValue = 100;
        }

        return addIntRow(label, param.name, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Bool: {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        QLabel* title = new QLabel(label, row);
        title->setFixedWidth(80);
        title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QCheckBox* box = new QCheckBox(row);
        box->setChecked(value.toBool());
        box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        layout->addWidget(title);
        layout->addWidget(box, 0, Qt::AlignVCenter);
        layout->addStretch(1);

        connect(box, &QCheckBox::toggled, this,
                [this, name = param.name](bool checked) { setParameterValue(name, checked); });

        return row;
    }

    case ShaderParameter::Type::Vec2: {
        const QVector2D current = value.value<QVector2D>();
        double minValue = param.minValue.isValid() ? param.minValue.toDouble() : -1.0;
        double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        fixRange(minValue, maxValue);

        return addVec2Widget(param, label, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Vec3: {
        const QVector3D current = value.value<QVector3D>();
        double minValue = param.minValue.isValid() ? param.minValue.toDouble() : -1.0;
        double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        fixRange(minValue, maxValue);

        return addVec3Widget(param, label, current, minValue, maxValue, parent);
    }

    case ShaderParameter::Type::Vec4: {
        const QVector4D current = value.value<QVector4D>();
        double minValue = param.minValue.isValid() ? param.minValue.toDouble() : -1.0;
        double maxValue = param.maxValue.isValid() ? param.maxValue.toDouble() : 1.0;
        fixRange(minValue, maxValue);

        return addVec4Widget(param, label, current, minValue, maxValue, parent);
    }
    }

    return new QWidget(parent);
}

QWidget*
WindowPrivate::addParameterPanel(QWidget* parent)
{
    QGroupBox* panel = new QGroupBox("Parameters", parent);
    panel->setMinimumWidth(220);

    QVBoxLayout* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(8, 8, 8, 8);
    panelLayout->setSpacing(8);

    QScrollArea* scrollArea = new QScrollArea(panel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* content = new QWidget(scrollArea);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);

    const sdk::render::ImageEffect imageEffect = d.imageLayer.imageEffect();
    const sdk::render::ShaderDefinition definition = imageEffect.shaderDefinition();
    const auto& parameters = definition.descriptor().parameters;

    QMap<QString, QGroupBox*> groupBoxes;
    QMap<QString, QVBoxLayout*> groupLayouts;

    for (const auto& param : parameters) {
        QWidget* editor = addParameterWidget(param, content);
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
    if (args.size() > 1)
        d.inputFile = args.at(1);

    d.dataPath = sdk::core::Environment::resourcePath("../../../data");

    sdk::core::File file(d.inputFile);

    if (!file.exists()) {
#if (1)
        const QString filename = "23.967.00086400.exr";
        file = sdk::core::File(QString("%1/exr/%2").arg(d.dataPath).arg(filename));
#else
        const QString filename = "square export 23.976 512x512.mov";
        file = sdk::core::File(QString("%1/quicktime/%2").arg(d.dataPath).arg(filename));
#endif
    }

    Q_ASSERT_X(file.exists(), "RenderEngine::loadFile", "file does not exist");

    Q_ASSERT(d.media.open(file) && d.media.waitForOpened() && "could not open media");
    Q_ASSERT(d.media.isValid() && "error open media");

    d.timeRange = d.media.timeRange();
    d.media.read();

    sdk::core::ImageBuffer image = d.media.image();
    Q_ASSERT(image.isValid() && "image not valid");

    sdk::render::ImageLayer imageLayer;
    imageLayer.setImage(image);
    d.imageLayer = imageLayer;

    const QStringList shaders = fxShaders();
    Q_ASSERT(!shaders.isEmpty() && "no fx shaders found");

    const QString defaultShader = shaders.contains("fx/logc.fx") ? QString("fx/logc.fx") : shaders.first();
    Q_ASSERT(loadShader(defaultShader) && "could not load fx shader");

    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget* controlsWidget = new QWidget(centralWidget);
    controlsWidget->setFixedHeight(40);
    controlsWidget->setProperty("role", "toolbar");

    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(8, 0, 8, 0);
    controlsLayout->setSpacing(8);
    controlsLayout->setAlignment(Qt::AlignVCenter);

    QPushButton* fitButton = new QPushButton("Fit", controlsWidget);
    fitButton->setShortcut(QKeySequence(Qt::Key_F));

    QPushButton* z25Button = new QPushButton("25%", controlsWidget);
    z25Button->setShortcut(QKeySequence(Qt::Key_1));

    QPushButton* z50Button = new QPushButton("50%", controlsWidget);
    z50Button->setShortcut(QKeySequence(Qt::Key_2));

    QPushButton* z75Button = new QPushButton("75%", controlsWidget);
    z75Button->setShortcut(QKeySequence(Qt::Key_3));

    QPushButton* z100Button = new QPushButton("100%", controlsWidget);
    z100Button->setShortcut(QKeySequence(Qt::Key_4));

    QLabel* shaderLabel = new QLabel("Shader", controlsWidget);
    shaderLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QComboBox* shaderCombo = new QComboBox(controlsWidget);
    shaderCombo->setMinimumWidth(180);

    for (const QString& shader : shaders) {
        const QString label = QFileInfo(shader).baseName();
        shaderCombo->addItem(label, shader);
    }

    const int shaderIndex = shaderCombo->findData(d.currentShader);
    if (shaderIndex >= 0)
        shaderCombo->setCurrentIndex(shaderIndex);

    QLabel* zoomLabel = new QLabel("100%", controlsWidget);
    zoomLabel->setMinimumWidth(60);
    zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    controlsLayout->addWidget(fitButton, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z25Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z50Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z75Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z100Button, 0, Qt::AlignVCenter);
    controlsLayout->addSpacing(16);
    controlsLayout->addWidget(shaderLabel, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(shaderCombo, 0, Qt::AlignVCenter);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(zoomLabel, 0, Qt::AlignVCenter);

    mainLayout->addWidget(controlsWidget);

    d.splitter = new QSplitter(Qt::Horizontal, centralWidget);
    d.splitter->setHandleWidth(1);

    QWidget* leftWidget = new QWidget(d.splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(8, 8, 8, 8);
    leftLayout->setSpacing(0);

    QFrame* viewerFrame = new QFrame(leftWidget);
    QVBoxLayout* frameLayout = new QVBoxLayout(viewerFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);

    d.viewer = new sdk::widgets::Viewer(viewerFrame);
    d.viewer->setResolution(QSize(1920, 1080));
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d.viewer->setImageLayers({ d.imageLayer });

    frameLayout->addWidget(d.viewer);
    leftLayout->addWidget(viewerFrame, 1);

    QWidget* timelineWidget = new QWidget(leftWidget);
    QHBoxLayout* timelineLayout = new QHBoxLayout(timelineWidget);
    timelineLayout->setContentsMargins(8, 0, 8, 0);
    timelineLayout->setSpacing(8);

    d.timelineSlider = new QSlider(Qt::Horizontal, timelineWidget);

    const qint64 frameCount = d.timeRange.isValid() ? d.timeRange.duration().frames() : 1;
    const int lastFrame = int(std::max<qint64>(0, frameCount - 1));

    d.timelineSlider->setRange(0, lastFrame);
    d.timelineSlider->setValue(0);
    d.timelineSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d.timelineSlider->setFixedHeight(28);

    d.timelineLabel = new QLabel(timelineWidget);
    d.timelineLabel->setMinimumWidth(140);
    d.timelineLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    timelineLayout->addWidget(d.timelineSlider, 1);
    timelineLayout->addWidget(d.timelineLabel);

    leftLayout->addWidget(timelineWidget);
    updateTimelineLabel();

    d.parameterContainer = new QWidget(d.splitter);
    d.parameterLayout = new QVBoxLayout(d.parameterContainer);
    d.parameterLayout->setContentsMargins(8, 8, 8, 8);
    d.parameterLayout->setSpacing(0);

    d.parameterPanel = addParameterPanel(d.parameterContainer);
    d.parameterLayout->addWidget(d.parameterPanel);

    d.splitter->addWidget(leftWidget);
    d.splitter->addWidget(d.parameterContainer);
    d.splitter->setStretchFactor(0, 1);
    d.splitter->setStretchFactor(1, 0);
    d.splitter->setSizes({ 900, 320 });

    mainLayout->addWidget(d.splitter, 1);

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

    connect(shaderCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, shaderCombo](int index) {
        if (index < 0)
            return;

        const QString shader = shaderCombo->itemData(index).toString();
        if (shader.isEmpty() || shader == d.currentShader)
            return;

        if (!loadShader(shader))
            return;

        rebuildParameterPanel();
    });

    connect(d.viewer, &sdk::widgets::Viewer::zoomChanged, zoomLabel, [zoomLabel](float zoom) {
        const int percent = int(std::round(zoom * 100.0f));
        zoomLabel->setText(QString("%1%").arg(percent));
    });

    connect(d.timelineSlider, &QSlider::valueChanged, this, [this](int frame) { seekFrame(frame); });

    d.window->setWindowTitle("testviewer");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(1200, 800);
}

void
WindowPrivate::seekFrame(int frame)
{
    if (!d.media.isValid() || !d.timeRange.isValid())
        return;

    const sdk::av::Fps fps = d.media.fps();
    const sdk::av::Time frameOffset = sdk::av::Time::fromFrames(frame, fps);
    const sdk::av::Time target = d.timeRange.start() + frameOffset;
    const sdk::av::Time duration = sdk::av::Time::fromFrames(1, fps);
    const sdk::av::TimeRange range(target, duration);

    const sdk::av::Time seekTime = d.media.seek(range);
    if (!seekTime.isValid()) {
        qWarning() << "seek failed:" << d.media.error();
        return;
    }

    const sdk::av::Time readTime = d.media.read();
    if (!readTime.isValid()) {
        qWarning() << "read after seek failed:" << d.media.error();
        return;
    }

    const sdk::core::ImageBuffer image = d.media.image();
    if (!image.isValid()) {
        qWarning() << "invalid image after seek:" << d.media.error();
        return;
    }

    d.imageLayer.setImage(image);

    updateTimelineLabel();
    update();
}

void
WindowPrivate::updateTimelineLabel()
{
    if (!d.timelineLabel)
        return;

    const int frame = d.timelineSlider ? d.timelineSlider->value() : 0;
    const int total = d.timelineSlider ? d.timelineSlider->maximum() + 1 : 0;

    if (d.media.isValid() && d.media.time().isValid()) {
        d.timelineLabel->setText(QString("%1 / %2  %3").arg(frame + 1).arg(total).arg(d.media.time().toString()));
    }
    else {
        d.timelineLabel->setText(QString("%1 / %2").arg(frame + 1).arg(total));
    }
}

void
WindowPrivate::update()
{
    if (!d.viewer)
        return;

    d.viewer->setImageLayers({ d.imageLayer });
    d.viewer->update();
}

Window::Window(QWidget* parent)
    : sdk::widgets::Window(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

}  // namespace flipman
