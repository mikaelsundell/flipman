// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

/**
 * @brief
 * Legacy, unbuilt feature-request form with no active submission backend.
 */
class FeatureRequestDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief
     * Builds the legacy feature form; this unused dialog is not part of the main application target.
     */
    explicit FeatureRequestDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Request a Feature");

        QVBoxLayout* layout = new QVBoxLayout(this);

        d.titleEdit = new QLineEdit(this);
        d.titleEdit->setPlaceholderText("Enter feature title...");
        layout->addWidget(d.titleEdit);

        d.descriptionEdit = new QTextEdit(this);
        d.descriptionEdit->setPlaceholderText("Describe the feature...");
        layout->addWidget(d.descriptionEdit);

        QPushButton* submitButton = new QPushButton("Submit", this);
        layout->addWidget(submitButton);

        connect(submitButton, &QPushButton::clicked, this, &FeatureRequestDialog::submitFeatureRequest);
    }

    /**
     * @brief
     * Placeholder action; does not submit a network request or modify a project.
     */
private Q_SLOTS:

    void submitFeatureRequest()
    {
        /*
        QString title = d.titleEdit->text();
        QString description = d.descriptionEdit->toPlainText();

        if (title.isEmpty() || description.isEmpty()) {
            return;
        }
        QString repoOwner = "YourGitHubUsername";
        QString repoName = "YourRepository";
        QString githubToken = "your_personal_access_token";

        QUrl url(QString("https://api.github.com/repos/%1/%2/issues").arg(repoOwner).arg(repoName));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("token %1").arg(githubToken).toUtf8());

        QJsonObject json;
        json["title"] = title;
        json["body"] = description;
        QJsonDocument doc(json);

        QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
        QNetworkReply *reply = networkManager->post(request, doc.toJson());
        connect(reply, &QNetworkReply::finished, this, [reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "Feature request submitted successfully!";
            } else {
                qDebug() << "Error submitting request: " << reply->errorString();
            }
            reply->deleteLater();
        });

        this->close();*/
    }

private:
    struct Data {
        QLineEdit* titleEdit = nullptr;
        QTextEdit* descriptionEdit = nullptr;
    };
    Data d;
};
