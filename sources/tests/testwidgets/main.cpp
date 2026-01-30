// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell

#include <core/application.h>
#include <core/environment.h>
#include <core/style.h>
#include <core/system.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>

using namespace sdk;

class TestWidgets : public QWidget {
public:
    TestWidgets(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle("Flipman SDK Widget Gallery");
        setMinimumSize(800, 600);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // --- Header Section (System Info) ---
        QGroupBox* infoGroup = new QGroupBox("SDK Environment", this);
        QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

        QLabel* envLabel = new QLabel(QString("<b>Path:</b> %1").arg(core::environment()->applicationPath()));
        QLabel* styleLabel = new QLabel(
            QString("<b>Theme:</b> %1").arg(core::style()->theme() == core::Style::Dark ? "Dark" : "Light"));

        infoLayout->addWidget(envLabel);
        infoLayout->addWidget(styleLabel);
        mainLayout->addWidget(infoGroup);

        // --- Gallery Section (The Widget Tests) ---
        QTabWidget* tabs = new QTabWidget(this);

        // Tab 1: Basic Inputs
        QWidget* inputTab = new QWidget();
        QGridLayout* grid = new QGridLayout(inputTab);

        grid->addWidget(new QLabel("QLineEdit:"), 0, 0);
        grid->addWidget(new QLineEdit("Sample text entry..."), 0, 1);

        grid->addWidget(new QLabel("QComboBox:"), 1, 0);
        QComboBox* combo = new QComboBox();
        combo->addItems({ "Option 1", "Option 2", "Option 3" });
        grid->addWidget(combo, 1, 1);

        grid->addWidget(new QLabel("QCheckBox:"), 2, 0);
        grid->addWidget(new QCheckBox("Enabled state"), 2, 1);

        grid->addWidget(new QLabel("QPushButton:"), 3, 0);
        grid->addWidget(new QPushButton("Standard Button"), 3, 1);

        grid->addWidget(new QLabel("QProgressBar:"), 4, 0);
        QProgressBar* bar = new QProgressBar();
        bar->setValue(65);
        grid->addWidget(bar, 4, 1);

        tabs->addTab(inputTab, "Basic Inputs");

        // Tab 2: Complex Views (Testing TreeView & Scrollbars)
        QWidget* viewTab = new QWidget();
        QVBoxLayout* viewLayout = new QVBoxLayout(viewTab);

        QTreeView* tree = new QTreeView();
        QStandardItemModel* model = new QStandardItemModel(5, 2);
        model->setHeaderData(0, Qt::Horizontal, "Name");
        model->setHeaderData(1, Qt::Horizontal, "Value");
        for (int i = 0; i < 5; ++i) {
            QStandardItem* item = new QStandardItem(QString("Sequence %1").arg(i));
            item->setCheckable(true);
            model->setItem(i, 0, item);
            model->setItem(i, 1, new QStandardItem("1001-1050"));
        }
        tree->setModel(model);
        tree->setAlternatingRowColors(true);
        viewLayout->addWidget(tree);

        tabs->addTab(viewTab, "Data Views");

        mainLayout->addWidget(tabs);

        // --- Bottom Action Bar ---
        QHBoxLayout* bottomLayout = new QHBoxLayout();
        QPushButton* awakeBtn = new QPushButton("Toggle Stay Awake");
        awakeBtn->setCheckable(true);
        awakeBtn->setChecked(core::system()->isStayAwake());

        bottomLayout->addStretch();
        bottomLayout->addWidget(awakeBtn);
        mainLayout->addLayout(bottomLayout);

        // --- Connections ---
        connect(awakeBtn, &QPushButton::toggled, [](bool checked) { core::system()->setStayAwake(checked); });

        connect(core::style(), &core::Style::themeChanged, this, [styleLabel](core::Style::Theme theme) {
            styleLabel->setText(QString("<b>Theme:</b> %1").arg(theme == core::Style::Dark ? "Dark" : "Light"));
        });
    }
};

int
main(int argc, char* argv[])
{
    core::Application app(argc, argv);
    TestWidgets widget;
    widget.show();
    return app.exec();
}

#include "main.moc"
