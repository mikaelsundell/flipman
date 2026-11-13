// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "colorselector.h"
#include "ui_viewer.h"
#include "window.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/render/render.h>
#include <flipmansdk/widgets/viewer.h>
#include <QActionGroup>
#include <QBoxLayout>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <limits>

namespace flipman {

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    void init();
    bool loadFile(const QString& path);
    QString dropFile(const QMimeData* mime) const;
    void applyColor();
    void updateResolution();
    void initBackgroundMenu();
    void update();
    void updateTimeline();
    bool eventFilter(QObject* object, QEvent* event);
    void seekFrame(int frame);

public:
    struct DisplayTransform {
        QString label;
        sdk::render::DisplayTransform transform;
    };

    QVector<DisplayTransform> displayTransforms();

public:
    struct Data {
        Ui::ViewerWindow windowUi;
        sdk::av::Media media;
        sdk::av::TimeRange timeRange;
        sdk::render::ImageLayer imageLayer;
        QString inputFile;
        QString autoInput;
        QSize sourceResolution;
        QScopedPointer<ColorSelector> inputSelector;
        QScopedPointer<ColorSelector> workingSelector;
        QScopedPointer<ColorSelector> outputSelector;
        QPointer<sdk::render::RenderEngine> renderEngine;
        QPointer<sdk::widgets::Viewer> viewer;
        QPointer<QSlider> timelineSlider;
        QPointer<Window> window;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1)
        d.inputFile = args.at(1);

    d.windowUi.setupUi(d.window);
    auto* displayCombo = d.windowUi.metalTag;
    auto* fitButton = d.windowUi.fitButton;
    auto* z25Button = d.windowUi.z25Button;
    auto* z50Button = d.windowUi.z50Button;
    auto* z75Button = d.windowUi.z75Button;
    auto* z100Button = d.windowUi.z100Button;
    auto* zoomLabel = d.windowUi.zoomLabel;
    d.timelineSlider = d.windowUi.timelineSlider;
    d.windowUi.timelineSlider->setTimelineMode(true);

    const QVector<DisplayTransform> transforms = displayTransforms();
    for (int i = 0; i < transforms.size(); ++i)
        displayCombo->addItem(transforms[i].label, i);

    d.renderEngine = new sdk::render::RenderEngine(d.window);
    d.renderEngine->setBackground(Qt::black);
    auto* resolution = d.windowUi.projectResolution;
    resolution->addItem("Auto (source)", QSize());
    resolution->insertSeparator(1);
    for (const auto& size : { QSize(1280, 720), QSize(1920, 1080), QSize(2048, 1080), QSize(2048, 858),
                              QSize(3840, 2160), QSize(4096, 2160), QSize(4096, 1716) })
        resolution->addItem(QString("%1 x %2").arg(size.width()).arg(size.height()), size);
    connect(resolution, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { updateResolution(); });

    d.viewer = new sdk::widgets::Viewer(d.windowUi.viewportContainer);
    d.viewer->setAcceptDrops(true);
    d.viewer->installEventFilter(this);
    d.viewer->setRenderEngine(d.renderEngine.data());
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    initBackgroundMenu();

    const sdk::render::DisplayTransform displayTransform { sdk::render::ColorSpace::Rec709,
                                                           sdk::render::TransferFunction::Gamma24 };

    d.viewer->setDisplayTransform(displayTransform);

    const auto profiles = sdk::render::ColorPipeline::colorProfiles();
    QList<sdk::render::ColorProfile> workingProfiles, outputProfiles;
    const auto workingNames = sdk::render::ColorPipeline::workingSpaces();
    const auto displayNames = sdk::render::ColorPipeline::displays();
    for (const auto& profile : profiles) {
        if (workingNames.contains(profile.name))
            workingProfiles.append(profile);
        if (displayNames.contains(profile.name) || workingNames.contains(profile.name))
            outputProfiles.append(profile);
    }
    d.inputSelector.reset(new ColorSelector(d.windowUi.input, d.windowUi.inputTransfer, profiles, {}));
    d.workingSelector.reset(
        new ColorSelector(d.windowUi.working, d.windowUi.workingTransfer, workingProfiles, "ACEScg"));
    d.outputSelector.reset(
        new ColorSelector(d.windowUi.output, d.windowUi.outputTransfer, outputProfiles, "Rec.1886 Rec.709 - Display"));
    d.workingSelector->setAutoName("ACEScg");
    d.outputSelector->setAutoName("Rec.1886 Rec.709 - Display");

    // Stable widths keep the stacked gamut and curve controls aligned.
    d.windowUi.view->setFixedWidth(180);
    displayCombo->setFixedWidth(200);
    resolution->setFixedWidth(155);
    for (auto* combo : { resolution, displayCombo })
        for (int i = 0; i < combo->count(); ++i)
            combo->setItemData(i, int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
    const auto refreshViews = [this]() {
        const QString previous = d.windowUi.view->currentData().toString();
        const QSignalBlocker blocker(d.windowUi.view);
        d.windowUi.view->clear();
        d.windowUi.view->addItem("Auto", "Auto");
        d.windowUi.view->insertSeparator(d.windowUi.view->count());
        const auto output = d.outputSelector->name();
        for (const auto& view : sdk::render::ColorPipeline::views(output))
            if (view != "Raw")
                d.windowUi.view->addItem(view == "Un-tone-mapped" ? QStringLiteral("No tone mapping") : view, view);
        d.windowUi.view->insertSeparator(d.windowUi.view->count());
        d.windowUi.view->addItem("Raw", "Raw");
        for (int i = 0; i < d.windowUi.view->count(); ++i)
            d.windowUi.view->setItemData(i, int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
        const int index = d.windowUi.view->findData(previous);
        d.windowUi.view->setCurrentIndex(index >= 0 ? index : d.windowUi.view->findData("Un-tone-mapped"));
    };
    refreshViews();
    d.inputSelector->changed = [this]() { applyColor(); };
    d.workingSelector->changed = [this]() { applyColor(); };
    connect(d.windowUi.view, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { applyColor(); });
    d.outputSelector->changed = [this, refreshViews, displayCombo, transforms]() {
        refreshViews();
        applyColor();
        sdk::render::DisplayTransform tag;
        if (sdk::render::ColorPipeline::displayTag(d.outputSelector->name(), tag)) {
            for (int i = 0; i < transforms.size(); ++i) {
                if (transforms[i].transform.colorSpace == tag.colorSpace
                    && transforms[i].transform.transferFunction == tag.transferFunction) {
                    displayCombo->setCurrentIndex(i);
                    break;
                }
            }
        }
    };

    int index = 0;
    for (int i = 0; i < transforms.size(); ++i) {
        if (transforms[i].transform.colorSpace == displayTransform.colorSpace
            && transforms[i].transform.transferFunction == displayTransform.transferFunction) {
            index = i;
            break;
        }
    }
    displayCombo->setCurrentIndex(index);

    d.windowUi.viewportLayout->addWidget(d.viewer);

    d.window->setAcceptDrops(true);
    d.window->installEventFilter(this);

    connect(fitButton, &QPushButton::clicked, this,
            [this]() { d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView); });

    connect(z25Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.25f); });
    connect(z50Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.5f); });
    connect(z75Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.75f); });
    connect(z100Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(1.0f); });

    connect(displayCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, displayCombo, transforms](int index) {
                if (index < 0)
                    return;

                const int transformIndex = displayCombo->itemData(index).toInt();
                if (transformIndex < 0 || transformIndex >= transforms.size())
                    return;

                const DisplayTransform& displayTransform = transforms[transformIndex];

                d.viewer->setDisplayTransform(
                    { displayTransform.transform.colorSpace, displayTransform.transform.transferFunction });

                d.viewer->update();
            });

    connect(d.viewer, &sdk::widgets::Viewer::zoomChanged, zoomLabel, [zoomLabel](float zoom) {
        const int percent = int(std::round(zoom * 100.0f));
        zoomLabel->setText(QString("%1%").arg(percent));
    });

    connect(d.timelineSlider, &QSlider::valueChanged, this, [this](int frame) { seekFrame(frame - 1); });
    connect(d.windowUi.frameField, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int frame) { d.timelineSlider->setValue(frame); });

    d.window->setWindowTitle("testviewer");
    d.window->resize(1200, 700);

    if (!d.inputFile.isEmpty())
        loadFile(d.inputFile);
    else {
        d.window->statusBar()->showMessage("Drop a movie or image here to open it");
        updateTimeline();
    }
}

bool
WindowPrivate::loadFile(const QString& path)
{
    // Decode before replacing the current media, so failed drops leave it usable.
    sdk::av::Media media;
    if (!media.open(sdk::core::File(path)) || !media.waitForOpened(5000) || !media.isValid()) {
        d.window->statusBar()->showMessage(QString("Could not open %1: %2").arg(path, media.error().message()));
        return false;
    }
    const auto time = media.read();
    const auto image = media.image();
    if (!time.isValid() || !image.isValid() || !image.isAllocated()) {
        d.window->statusBar()->showMessage(QString("Could not decode %1").arg(path));
        return false;
    }
    d.media = media;
    d.inputFile = path;
    d.timeRange = media.timeRange();
    d.imageLayer.setImage(image);
    d.autoInput = sdk::render::ColorPipeline::inputColorSpace(image);
    QString assumption;
    if (d.autoInput.isEmpty()) {
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == "braw") {
            const auto metadata = media.metaData();
            const auto gamma = metadata.value(sdk::core::MetaData::Group::Video, "decodeGamma").toString();
            const auto gamut = metadata.value(sdk::core::MetaData::Group::Video, "decodeGamut").toString();
            const auto generation = metadata.value(sdk::core::MetaData::Group::Video, "decodeColorScienceGen").toInt();
            if (generation == 5 && gamma == "Blackmagic Design Film" && gamut == "Blackmagic Design")
                d.autoInput = "BMDFilm WideGamut Gen5";
        }
        else if (suffix == "exr" || suffix == "hdr") {
            d.autoInput = "Linear Rec.709 (sRGB)";
            assumption = "Missing source tags: assuming linear Rec.709; select Input to override";
        }
        else {
            d.autoInput = d.timeRange.isValid() && d.timeRange.duration().frames() > 1 ? "Rec.1886 Rec.709 - Display"
                                                                                       : "sRGB - Display";
            assumption = "Missing source tags: assuming " + d.autoInput + "; select Input to override";
        }
    }
    d.inputSelector->setAutoName(d.autoInput);
    if (d.autoInput.isEmpty())
        assumption = "Unknown source encoding: select Input";
    applyColor();
    d.sourceResolution = image.displayWindow().size();
    updateResolution();
    {
        const QSignalBlocker blocker(d.timelineSlider);
        const qint64 frameCount = d.timeRange.isValid() ? d.timeRange.duration().frames() : 1;
        const int total = int(std::clamp<qint64>(frameCount, 1, std::numeric_limits<int>::max() - 1));
        d.timelineSlider->setRange(1, total);
        d.timelineSlider->setValue(1);
        d.timelineSlider->setEnabled(total > 1);
        const QSignalBlocker fieldBlocker(d.windowUi.frameField);
        d.windowUi.frameField->setRange(1, total);
        d.windowUi.frameField->setValue(1);
        d.windowUi.frameField->setEnabled(total > 1);
    }
    d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView);
    const auto metadata = media.metaData();
    QString title = metadata.value("title").toString().trimmed();
    if (title.isEmpty())
        title = metadata.value(sdk::core::MetaData::convert(sdk::core::MetaData::Title)).toString().trimmed();
    if (title.isEmpty())
        title = metadata.value("name").toString().trimmed();
    if (title.isEmpty())
        title = metadata.value(sdk::core::MetaData::Group::Container, "title").toString().trimmed();
    if (title.isEmpty())
        title = QFileInfo(path).fileName();
    d.window->setWindowTitle(QString("testviewer — %1").arg(title));
    d.window->statusBar()->showMessage(assumption);
    updateTimeline();
    update();
    return true;
}

QString
WindowPrivate::dropFile(const QMimeData* mime) const
{
    if (!mime || !mime->hasUrls() || mime->urls().size() != 1)
        return {};
    const auto url = mime->urls().first();
    const QFileInfo file(url.toLocalFile());
    if (!url.isLocalFile() || !file.isFile() || !d.media.isSupported(file.suffix()))
        return {};
    return file.absoluteFilePath();
}

void
WindowPrivate::applyColor()
{
    sdk::render::ColorSettings settings;
    settings.enabled = true;
    const QString source = d.inputSelector->name().isEmpty() ? d.autoInput : d.inputSelector->name();
    settings.workingSpace = d.workingSelector->name();
    if (settings.workingSpace.isEmpty())
        settings.workingSpace = "ACEScg";
    if (settings.workingSpace == "Raw") {
        // Bypass the working conversion: effects receive the source encoding and
        // the output transform is informed of that same encoding.
        settings.workingSpace = source.isEmpty() || source == "Raw" ? QStringLiteral("ACEScg") : source;

        // Display-referred sources need a scene counterpart for effect processing.
        for (const auto& profile : sdk::render::ColorPipeline::colorProfiles())
            if (profile.name == settings.workingSpace) {
                settings.workingSpace = sdk::render::ColorPipeline::combinedColorSpace(profile.gamut, profile.encoding);
                break;
            }
    }
    settings.display = d.outputSelector->name().isEmpty() ? QStringLiteral("Rec.1886 Rec.709 - Display")
                                                          : d.outputSelector->name();
    settings.view = d.windowUi.view->currentData().toString() == "Auto" ? QStringLiteral("Un-tone-mapped")
                                                                        : d.windowUi.view->currentData().toString();
    if (!d.renderEngine->setColorSettings(settings)) {
        d.window->statusBar()->showMessage(d.renderEngine->error().message());
        return;
    }
    d.imageLayer.setInputColorSpace(source);
    if (d.media.isValid())
        update();
}

void
WindowPrivate::initBackgroundMenu()
{
    auto* menu = new QMenu(d.viewer);
    menu->setObjectName("renderContextMenu");
    auto* background = menu->addMenu("Timeline background");
    background->setObjectName("timelineBackgroundMenu");
    auto* choices = new QActionGroup(background);
    choices->setExclusive(true);
    for (const auto& preset : QList<QPair<QString, QColor>> { { "Black", Qt::black },
                                                              { "Dark gray", QColor(46, 46, 46) },
                                                              { "Gray", QColor(128, 128, 128) },
                                                              { "White", Qt::white } }) {
        auto* action = background->addAction(preset.first);
        action->setCheckable(true);
        action->setData(preset.second);
        choices->addAction(action);
        connect(action, &QAction::triggered, this, [this, color = preset.second]() {
            d.renderEngine->setBackground(color);
            d.viewer->update();
        });
    }
    background->addSeparator();
    auto* custom = background->addAction("Custom color…");
    custom->setCheckable(true);
    choices->addAction(custom);
    connect(custom, &QAction::triggered, this, [this]() {
        const auto color = QColorDialog::getColor(d.renderEngine->background(), d.window, "Timeline background");
        if (color.isValid()) {
            d.renderEngine->setBackground(color);
            d.viewer->update();
        }
    });
    connect(background, &QMenu::aboutToShow, this, [this, choices, custom]() {
        bool matched = false;
        for (auto* action : choices->actions()) {
            if (action == custom)
                continue;
            const bool selected = action->data().value<QColor>() == d.renderEngine->background();
            action->setChecked(selected);
            matched |= selected;
        }
        custom->setChecked(!matched);
    });
    for (auto* widget : { static_cast<QWidget*>(d.viewer.data()), d.windowUi.RenderView }) {
        widget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(widget, &QWidget::customContextMenuRequested, this,
                [menu, widget](const QPoint& position) { menu->exec(widget->mapToGlobal(position)); });
    }
}

void
WindowPrivate::updateResolution()
{
    QSize resolution = d.windowUi.projectResolution->currentData().toSize();
    if (resolution.isEmpty())
        resolution = d.sourceResolution;
    if (resolution.isEmpty())
        return;

    d.renderEngine->setResolution(resolution);
    d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView);
    d.viewer->update();
}

void
WindowPrivate::update()
{
    Q_ASSERT(d.viewer);
    d.renderEngine->setImageLayers({ d.imageLayer });
    d.viewer->update();
}

void
WindowPrivate::updateTimeline()
{
    const int total = d.media.isValid() ? d.timelineSlider->maximum() : 0;
    const QSignalBlocker blocker(d.windowUi.frameField);
    d.windowUi.frameField->setValue(total > 0 ? d.timelineSlider->value() : 0);
    d.windowUi.totalFramesField->setText(QString::number(total));
    QString timecode = "00:00:00:00";
    if (d.media.isValid() && d.timeRange.isValid()) {
        // Readers differ in whether time() reports the decoded frame or next cursor.
        // Source timecode is independent of the zero-based seek range.
        const auto start = d.media.start().isValid() ? d.media.start() : d.timeRange.start();
        const sdk::av::Time selected(start, start.ticks() + start.ticks(d.timelineSlider->value() - 1));
        timecode = sdk::av::SmpteTime(selected).toString();
        if (d.media.fps().dropFrame())
            timecode.replace('.', ';');
    }
    d.windowUi.smpteField->setText(timecode);
}

bool
WindowPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (object == d.window || object == d.viewer) {
        if (event->type() == QEvent::DragEnter) {
            auto* drag = static_cast<QDragEnterEvent*>(event);
            if (!dropFile(drag->mimeData()).isEmpty() && drag->possibleActions().testFlag(Qt::CopyAction)) {
                drag->setDropAction(Qt::CopyAction);
                drag->accept();
            }
            else
                drag->ignore();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            auto* drop = static_cast<QDropEvent*>(event);
            const auto path = dropFile(drop->mimeData());
            if (!path.isEmpty() && drop->possibleActions().testFlag(Qt::CopyAction) && loadFile(path)) {
                drop->setDropAction(Qt::CopyAction);
                drop->accept();
            }
            else
                drop->ignore();
            return true;
        }
    }
    if (object != d.window)
        return QObject::eventFilter(object, event);

    switch (event->type()) {
    case QEvent::ScreenChangeInternal:
    case QEvent::DevicePixelRatioChange:
    case QEvent::PlatformSurface:
        d.window->update();
        QTimer::singleShot(0, d.window, [this]() {
            if (d.viewer)
                d.viewer->update();
            if (d.window)
                d.window->update();
        });
        break;
    default: break;
    }
    return QObject::eventFilter(object, event);
}

void
WindowPrivate::seekFrame(int frame)
{
    if (!d.media.isValid() || !d.timeRange.isValid())
        return;

    const sdk::av::Time start = d.timeRange.start();
    const sdk::av::Time target(start, start.ticks() + start.ticks(frame));
    const sdk::av::Time duration(start, start.ticks(1));
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

    updateTimeline();
    update();
}

QVector<WindowPrivate::DisplayTransform>
WindowPrivate::displayTransforms()
{
    using ColorSpace = sdk::render::ColorSpace;
    using TransferFunction = sdk::render::TransferFunction;
    return {
        { "sRGB", { ColorSpace::Rec709, TransferFunction::SRGB } },
        { "Rec.709 Gamma 2.2", { ColorSpace::Rec709, TransferFunction::Gamma22 } },
        { "Rec.709 Gamma 2.4", { ColorSpace::Rec709, TransferFunction::Gamma24 } },
        { "Rec.709 Gamma 2.6", { ColorSpace::Rec709, TransferFunction::Gamma26 } },
        { "Display P3 sRGB", { ColorSpace::DisplayP3, TransferFunction::SRGB } },
        { "Display P3 Gamma 2.2", { ColorSpace::DisplayP3, TransferFunction::Gamma22 } },
        { "Display P3 Gamma 2.4", { ColorSpace::DisplayP3, TransferFunction::Gamma24 } },
        { "DCI-P3 Gamma 2.6", { ColorSpace::DCIP3, TransferFunction::Gamma26 } },
        { "Rec.2020 Gamma 2.4", { ColorSpace::Rec2020, TransferFunction::Gamma24 } },
    };
}

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

bool
Window::isValid() const
{
    return p->d.media.isValid() && p->d.imageLayer.image().isAllocated();
}

}  // namespace flipman
