// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMetaEnum>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

namespace flipman {

namespace {
    QString hslString(const QColor& color)
    {
        float h = 0.0f;
        float s = 0.0f;
        float l = 0.0f;
        float a = 0.0f;
        color.getHslF(&h, &s, &l, &a);
        return QString("HSL: %1, %2%, %3%  Alpha: %4%")
            .arg(QString::number(h * 360.0, 'f', 1))
            .arg(QString::number(s * 100.0, 'f', 1))
            .arg(QString::number(l * 100.0, 'f', 1))
            .arg(QString::number(a * 100.0, 'f', 1));
    }

    QString qcolorFromHslString(const QColor& color)
    {
        return QString("QColor::fromHsl(%1, %2, %3)")
            .arg(color.hslHue())
            .arg(color.hslSaturation())
            .arg(color.lightness());
    }

}  // namespace

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    ~WindowPrivate() override;
    void init();
    QWidget* createControls(QWidget* parent);
    QWidget* createForm(QWidget* parent);
    QWidget* createPreview(QWidget* parent);
    QWidget* createStyleEditor(QWidget* parent);
    QWidget* createVideoCapture(QWidget* parent);
    QTreeWidget* createTree(QWidget* parent);
    void createMenus();

    struct Data {
        int progress = 35;
        QPointer<QProgressBar> progressBar;
        QPointer<QLabel> statusLabel;
        QPointer<QTimer> timer;
        QPointer<QComboBox> colorRoleCombo;
        QPointer<QPushButton> colorButton;
        QPointer<QPushButton> copyColorButton;
        QPointer<QLabel> hslLabel;
        QPointer<QSpinBox> smallFontSpin;
        QPointer<QSpinBox> mediumFontSpin;
        QPointer<QSpinBox> largeFontSpin;
        QPointer<QSpinBox> smallIconSpin;
        QPointer<QSpinBox> mediumIconSpin;
        QPointer<QSpinBox> largeIconSpin;
        QPointer<Window> window;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

WindowPrivate::~WindowPrivate() {}

void
WindowPrivate::init()
{
    createMenus();

    QWidget* centralWidget = new QWidget(d.window);
    QHBoxLayout* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    rootSplitter->setChildrenCollapsible(false);

    QWidget* leftPanel = createStyleEditor(rootSplitter);
    leftPanel->setMinimumWidth(260);
    leftPanel->setMaximumWidth(520);

    QTabWidget* tabs = new QTabWidget(rootSplitter);
    tabs->setDocumentMode(true);
    tabs->tabBar()->setAutoFillBackground(true);
    tabs->setTabPosition(QTabWidget::North);

    QWidget* filmScannerPage = new QWidget(tabs);
    QVBoxLayout* filmScannerLayout = new QVBoxLayout(filmScannerPage);
    filmScannerLayout->setContentsMargins(12, 12, 12, 12);
    filmScannerLayout->setSpacing(12);

    QLabel* filmScannerLabel = new QLabel("Film Scanner", filmScannerPage);
    filmScannerLabel->setAlignment(Qt::AlignCenter);
    filmScannerLayout->addWidget(filmScannerLabel, 1);

    tabs->addTab(filmScannerPage, "Film Scanner");
    tabs->addTab(createVideoCapture(tabs), "Video Capture");
    tabs->setCurrentIndex(1);

    rootSplitter->addWidget(leftPanel);
    rootSplitter->addWidget(tabs);
    rootSplitter->setStretchFactor(0, 0);
    rootSplitter->setStretchFactor(1, 1);
    rootSplitter->setSizes({ 320, 880 });

    rootLayout->addWidget(rootSplitter);

    d.window->setCentralWidget(centralWidget);
    d.window->statusBar()->showMessage("Stylesheet preview");

    d.timer = new QTimer(this);
    QObject::connect(d.timer, &QTimer::timeout, this, [this]() {
        d.progress = (d.progress + 1) % 101;

        if (d.progressBar)
            d.progressBar->setValue(d.progress);
    });
    d.timer->start(80);

    d.window->setWindowTitle("teststyle");
    d.window->resize(1200, 700);
}

QWidget*
WindowPrivate::createControls(QWidget* parent)
{
    QGroupBox* box = new QGroupBox("Controls", parent);
    QVBoxLayout* layout = new QVBoxLayout(box);
    layout->setSpacing(10);

    QComboBox* combo = new QComboBox(box);
    combo->addItems({ "Viewer", "Render", "DeckLink Output", "Export" });

    QComboBox* disabledCombo = new QComboBox(box);
    disabledCombo->addItems({ "Disabled option" });
    disabledCombo->setDisabled(true);

    QPushButton* button = new QPushButton("Push Button", box);
    QPushButton* disabledButton = new QPushButton("Disabled Button", box);
    disabledButton->setDisabled(true);

    QToolButton* toolButton = new QToolButton(box);
    toolButton->setText("Tool Button");
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);

    QMenu* toolMenu = new QMenu(toolButton);
    toolMenu->addAction("Tool Action 1");
    toolMenu->addAction("Tool Action 2");
    toolButton->setMenu(toolMenu);

    d.progressBar = new QProgressBar(box);
    d.progressBar->setRange(0, 100);
    d.progressBar->setValue(d.progress);
    d.progressBar->setTextVisible(false);

    layout->addWidget(new QLabel("Combo box", box));
    layout->addWidget(combo);
    layout->addWidget(disabledCombo);
    layout->addSpacing(6);
    layout->addWidget(button);
    layout->addWidget(disabledButton);
    layout->addWidget(toolButton);
    layout->addSpacing(6);
    layout->addWidget(new QLabel("Progress bar", box));
    layout->addWidget(d.progressBar);
    layout->addStretch();

    return box;
}

QWidget*
WindowPrivate::createForm(QWidget* parent)
{
    QGroupBox* box = new QGroupBox("Inputs", parent);
    QVBoxLayout* layout = new QVBoxLayout(box);
    layout->setSpacing(10);

    QLineEdit* lineEdit = new QLineEdit(box);
    lineEdit->setPlaceholderText("Line edit");

    QLineEdit* disabledLineEdit = new QLineEdit(box);
    disabledLineEdit->setText("Disabled line edit");
    disabledLineEdit->setDisabled(true);

    QCheckBox* checkBox = new QCheckBox("Check box", box);
    checkBox->setChecked(true);

    QCheckBox* disabledCheckBox = new QCheckBox("Disabled check box", box);
    disabledCheckBox->setDisabled(true);

    layout->addWidget(new QLabel("Text input", box));
    layout->addWidget(lineEdit);
    layout->addWidget(disabledLineEdit);
    layout->addSpacing(6);
    layout->addWidget(checkBox);
    layout->addWidget(disabledCheckBox);
    layout->addStretch();

    return box;
}

QTreeWidget*
WindowPrivate::createTree(QWidget* parent)
{
    QTreeWidget* tree = new QTreeWidget(parent);
    tree->setHeaderLabels({ "Name", "Type", "Status" });
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(true);

    auto* render = new QTreeWidgetItem(tree, { "Render", "Group", "Enabled" });
    render->setCheckState(0, Qt::Checked);

    auto* viewer = new QTreeWidgetItem(render, { "Viewer", "Widget", "Active" });
    viewer->setCheckState(0, Qt::Checked);

    auto* decklink = new QTreeWidgetItem(render, { "DeckLink", "Output", "Inactive" });
    decklink->setCheckState(0, Qt::Unchecked);

    auto* color = new QTreeWidgetItem(tree, { "Color", "Settings", "Enabled" });
    color->setCheckState(0, Qt::PartiallyChecked);

    new QTreeWidgetItem(color, { "Rec.709", "Transform", "Selected" });
    new QTreeWidgetItem(color, { "Linear", "Working Space", "Available" });

    tree->expandAll();
    return tree;
}

QWidget*
WindowPrivate::createPreview(QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QTextBrowser* browser = new QTextBrowser(widget);
    browser->setHtml("<b>QTextBrowser</b><br>"
                     "This area previews text color, background color, borders, "
                     "selection color, and scrollbars from the stylesheet.<br><br>"
                     "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                     "Integer render pipeline widgets can be tested here.");

    QScrollArea* scrollArea = new QScrollArea(widget);
    scrollArea->setWidgetResizable(true);

    QWidget* scrollContent = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);

    for (int i = 1; i <= 20; ++i)
        scrollLayout->addWidget(new QLabel(QString("Scrollable row %1").arg(i), scrollContent));

    scrollLayout->addStretch();
    scrollArea->setWidget(scrollContent);

    layout->addWidget(browser, 1);
    layout->addWidget(scrollArea, 1);

    return widget;
}

QWidget*
WindowPrivate::createVideoCapture(QWidget* parent)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget* toolbar = new QWidget(widget);
    toolbar->setFixedHeight(40);
    toolbar->setProperty("role", "toolbar");

    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 0, 12, 0);
    toolbarLayout->setSpacing(8);
    toolbarLayout->setAlignment(Qt::AlignVCenter);

    QLabel* title = new QLabel("Preview Roles", toolbar);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    toolbarLayout->addWidget(title);
    toolbarLayout->addStretch(1);

    QWidget* content = new QWidget(widget);
    QHBoxLayout* contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(12);

    QWidget* controls = createControls(content);
    QWidget* form = createForm(content);
    QTreeWidget* tree = createTree(content);
    QWidget* preview = createPreview(content);

    controls->setMinimumWidth(220);
    form->setMinimumWidth(260);
    tree->setMinimumWidth(320);
    preview->setMinimumWidth(260);

    contentLayout->addWidget(controls);
    contentLayout->addWidget(form);
    contentLayout->addWidget(tree, 1);
    contentLayout->addWidget(preview, 1);

    QFrame* statusFrame = new QFrame(widget);
    statusFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(10, 8, 10, 8);

    d.statusLabel = new QLabel("Ready. Previewing stylesheet roles.", statusFrame);
    d.statusLabel->setWordWrap(true);
    statusLayout->addWidget(d.statusLabel);

    layout->addWidget(toolbar);
    layout->addWidget(content, 1);
    layout->addWidget(statusFrame);

    return widget;
}

QWidget*
WindowPrivate::createStyleEditor(QWidget* parent)
{
    using Style = flipman::sdk::core::Style;

    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    QString* loadedStylesheetFile = new QString;

    auto loadStylesheet = [this, loadedStylesheetFile](const QString& filename) -> bool {
        if (filename.isEmpty())
            return false;

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "could not open stylesheet:" << filename << file.errorString();
            return false;
        }

        qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
        *loadedStylesheetFile = filename;

        if (d.statusLabel)
            d.statusLabel->setText(QString("Loaded stylesheet: %1").arg(QFileInfo(filename).fileName()));

        qInfo() << "Loaded stylesheet:" << filename;
        return true;
    };

    auto addSpinSliderRow = [](QFormLayout* formLayout, const QString& label, QSpinBox* spin, int minValue,
                               int maxValue, int value) {
        QWidget* row = new QWidget(formLayout->parentWidget());
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        QSlider* slider = new QSlider(Qt::Horizontal, row);
        slider->setRange(minValue, maxValue);
        slider->setValue(value);
        slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        spin->setRange(minValue, maxValue);
        spin->setSingleStep(1);
        spin->setValue(value);

        QObject::connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        QObject::connect(spin, qOverload<int>(&QSpinBox::valueChanged), slider, &QSlider::setValue);

        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(spin);

        formLayout->addRow(label, row);
    };

    QGroupBox* stylesheetBox = new QGroupBox("Stylesheet", widget);
    QHBoxLayout* stylesheetLayout = new QHBoxLayout(stylesheetBox);

    QPushButton* loadStylesheetButton = new QPushButton("Load", stylesheetBox);
    QPushButton* reloadStylesheetButton = new QPushButton("Reload", stylesheetBox);
    QPushButton* clearStylesheetButton = new QPushButton("Clear", stylesheetBox);

    stylesheetLayout->addWidget(loadStylesheetButton);
    stylesheetLayout->addWidget(reloadStylesheetButton);
    stylesheetLayout->addWidget(clearStylesheetButton);
    stylesheetLayout->addStretch();

    QObject::connect(loadStylesheetButton, &QPushButton::clicked, this, [this, loadStylesheet]() {
        const QString filename = QFileDialog::getOpenFileName(d.window, "Load Stylesheet", QString(),
                                                              "Qt Stylesheet (*.qss *.css);;All Files (*)");

        loadStylesheet(filename);
    });

    QObject::connect(reloadStylesheetButton, &QPushButton::clicked, this,
                     [this, loadedStylesheetFile, loadStylesheet]() {
                         if (loadedStylesheetFile->isEmpty()) {
                             if (d.statusLabel)
                                 d.statusLabel->setText("No external stylesheet loaded");

                             qWarning() << "no external stylesheet loaded";
                             return;
                         }

                         loadStylesheet(*loadedStylesheetFile);
                     });

    QObject::connect(clearStylesheetButton, &QPushButton::clicked, this, [this, loadedStylesheetFile]() {
        qApp->setStyleSheet(QString());
        loadedStylesheetFile->clear();

        if (d.statusLabel)
            d.statusLabel->setText("Cleared application stylesheet");

        qInfo() << "Cleared application stylesheet";
    });

    QGroupBox* colorBox = new QGroupBox("Colors", widget);
    QFormLayout* colorLayout = new QFormLayout(colorBox);

    d.colorRoleCombo = new QComboBox(colorBox);

    const QMetaEnum colorEnum = QMetaEnum::fromType<Style::ColorRole>();
    for (int i = 0; i < colorEnum.keyCount(); ++i)
        d.colorRoleCombo->addItem(QString::fromLatin1(colorEnum.key(i)), colorEnum.value(i));

    d.colorButton = new QPushButton("Pick Color", colorBox);
    d.copyColorButton = new QPushButton("Copy", colorBox);
    d.hslLabel = new QLabel(colorBox);
    d.hslLabel->setWordWrap(true);

    QWidget* hslRow = new QWidget(colorBox);
    QHBoxLayout* hslLayout = new QHBoxLayout(hslRow);
    hslLayout->setContentsMargins(0, 0, 0, 0);
    hslLayout->setSpacing(6);
    hslLayout->addWidget(d.hslLabel, 1);
    hslLayout->addWidget(d.copyColorButton);

    auto updateColorPreview = [this]() {
        using Style = flipman::sdk::core::Style;

        if (!d.colorRoleCombo || !d.colorButton || !d.copyColorButton || !d.hslLabel)
            return;

        const auto role = Style::ColorRole(d.colorRoleCombo->currentData().toInt());
        const QColor color = sdk::core::style()->color(role);

        const QString hsl = hslString(color);
        const QString qcolor = qcolorFromHslString(color);

        d.colorButton->setText(color.name(QColor::HexArgb));
        d.colorButton->setStyleSheet(QString("background-color: %1;").arg(color.name(QColor::HexArgb)));

        d.hslLabel->setText(hsl + "\n" + qcolor);
        d.copyColorButton->setProperty("qcolor", qcolor);

        qInfo() << "Style color" << d.colorRoleCombo->currentText() << color.name(QColor::HexArgb) << hsl << qcolor;
    };

    QObject::connect(d.colorRoleCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                     [updateColorPreview](int) { updateColorPreview(); });

    QObject::connect(d.colorButton, &QPushButton::clicked, this, [this, updateColorPreview]() {
        using Style = flipman::sdk::core::Style;

        const auto role = Style::ColorRole(d.colorRoleCombo->currentData().toInt());
        const QColor current = sdk::core::style()->color(role);

        const QColor color = QColorDialog::getColor(current, d.window,
                                                    QString("Select %1 Color").arg(d.colorRoleCombo->currentText()),
                                                    QColorDialog::ShowAlphaChannel);

        if (!color.isValid())
            return;

        sdk::core::style()->setColor(role, color);
        sdk::core::style()->update();

        updateColorPreview();
    });

    QObject::connect(d.copyColorButton, &QPushButton::clicked, this, [this]() {
        if (!d.copyColorButton)
            return;

        const QString text = d.copyColorButton->property("qcolor").toString();
        if (text.isEmpty())
            return;

        QApplication::clipboard()->setText(text);

        if (d.statusLabel)
            d.statusLabel->setText(QString("Copied %1").arg(text));

        qInfo().noquote() << "Copied" << text;
    });

    colorLayout->addRow("Role", d.colorRoleCombo);
    colorLayout->addRow("Color", d.colorButton);
    colorLayout->addRow("HSL", hslRow);

    QGroupBox* fontBox = new QGroupBox("Font Sizes", widget);
    QFormLayout* fontLayout = new QFormLayout(fontBox);

    d.smallFontSpin = new QSpinBox(fontBox);
    d.mediumFontSpin = new QSpinBox(fontBox);
    d.largeFontSpin = new QSpinBox(fontBox);

    addSpinSliderRow(fontLayout, "Small", d.smallFontSpin, 6, 48, sdk::core::style()->fontSize(Style::Small));
    addSpinSliderRow(fontLayout, "Medium", d.mediumFontSpin, 6, 48, sdk::core::style()->fontSize(Style::Medium));
    addSpinSliderRow(fontLayout, "Large", d.largeFontSpin, 6, 48, sdk::core::style()->fontSize(Style::Large));

    auto connectFontSpin = [this](QSpinBox* spin, Style::UIScale scale) {
        QObject::connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [scale](int value) {
            sdk::core::style()->setFontSize(scale, value);
            sdk::core::style()->update();
        });
    };

    connectFontSpin(d.smallFontSpin, Style::Small);
    connectFontSpin(d.mediumFontSpin, Style::Medium);
    connectFontSpin(d.largeFontSpin, Style::Large);

    QGroupBox* iconBox = new QGroupBox("Icon Sizes", widget);
    QFormLayout* iconLayout = new QFormLayout(iconBox);

    d.smallIconSpin = new QSpinBox(iconBox);
    d.mediumIconSpin = new QSpinBox(iconBox);
    d.largeIconSpin = new QSpinBox(iconBox);

    addSpinSliderRow(iconLayout, "Small", d.smallIconSpin, 8, 128, sdk::core::style()->iconSize(Style::Small));
    addSpinSliderRow(iconLayout, "Medium", d.mediumIconSpin, 8, 128, sdk::core::style()->iconSize(Style::Medium));
    addSpinSliderRow(iconLayout, "Large", d.largeIconSpin, 8, 128, sdk::core::style()->iconSize(Style::Large));

    auto connectIconSpin = [this](QSpinBox* spin, Style::UIScale scale) {
        QObject::connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [scale](int value) {
            sdk::core::style()->setIconSize(scale, value);
            sdk::core::style()->update();
        });
    };

    connectIconSpin(d.smallIconSpin, Style::Small);
    connectIconSpin(d.mediumIconSpin, Style::Medium);
    connectIconSpin(d.largeIconSpin, Style::Large);

    layout->addWidget(stylesheetBox);
    layout->addWidget(colorBox);
    layout->addWidget(fontBox);
    layout->addWidget(iconBox);
    layout->addStretch();

    updateColorPreview();

    return widget;
}

void
WindowPrivate::createMenus()
{
    QMenu* fileMenu = d.window->menuBar()->addMenu("File");
    fileMenu->addAction("Open");
    fileMenu->addAction("Save");
    fileMenu->addSeparator();

    QAction* checkedAction = fileMenu->addAction("Checked Action");
    checkedAction->setCheckable(true);
    checkedAction->setChecked(true);

    QAction* disabledAction = fileMenu->addAction("Disabled Action");
    disabledAction->setDisabled(true);

    QMenu* viewMenu = d.window->menuBar()->addMenu("View");
    viewMenu->addAction("Reset Layout");
    viewMenu->addAction("Toggle Viewer");

    QMenu* submenu = viewMenu->addMenu("Submenu");
    submenu->addAction("Sub Action 1");
    submenu->addAction("Sub Action 2");
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
