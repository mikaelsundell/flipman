// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "app/application.h"
#include "commands/commanddisplay.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/displayprofiles.h"
#include "core/session.h"
#include "mcpserver.h"
#include "python/pythoninterpreter.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <cmath>

namespace flipman {

namespace {

    constexpr qsizetype maxHeaderSize = 32 * 1024;
    constexpr qsizetype maxBodySize = 1024 * 1024;

    QJsonObject jsonRpcError(const QJsonValue& id, int code, const QString& message)
    {
        return { { "jsonrpc", "2.0" },
                 { "id", id },
                 { "error", QJsonObject { { "code", code }, { "message", message } } } };
    }

    QJsonObject toolResult(const QString& text, bool isError = false)
    {
        QJsonObject result { { "content", QJsonArray { QJsonObject { { "type", "text" }, { "text", text } } } } };
        if (isError)
            result.insert("isError", true);
        return result;
    }

    QString jsonText(const QJsonValue& value)
    {
        if (value.isObject())
            return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Indented));

        if (value.isArray())
            return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Indented));

        return value.toString();
    }

    QJsonObject toolDefinition(const QString& name, const QString& description, const QJsonObject& properties,
                               const QJsonArray& required, bool readOnly, bool destructive = false)
    {
        QJsonObject annotations { { "readOnlyHint", readOnly },
                                  { "destructiveHint", destructive },
                                  { "openWorldHint", false } };
        return { { "name", name },
                 { "description", description },
                 { "inputSchema", QJsonObject { { "type", "object" },
                                                { "properties", properties },
                                                { "required", required },
                                                { "additionalProperties", false } } },
                 { "annotations", annotations } };
    }

    QJsonArray availableTools()
    {
        const auto string = QJsonObject { { "type", "string" } };
        const auto integer = QJsonObject { { "type", "integer" } };
        QJsonArray profiles;
        for (const auto& profile : displayProfiles())
            profiles.append(profile.name);
        return {
            toolDefinition("flipman_get_session_info", "Read current file, timeline, playback and display state.", {},
                           {}, true),
            toolDefinition("flipman_get_metadata", "Read source metadata without decoding another frame.", {}, {}, true),
            toolDefinition("flipman_open_file",
                           "Open a movie or image asynchronously; poll session info for completion.",
                           { { "filename", string } }, { "filename" }, false),
            toolDefinition("flipman_close_file", "Close the current file and clear its command history.", {}, {}, false),
            toolDefinition("flipman_set_time", "Seek to a zero-based timeline frame through command history.",
                           { { "frame", integer } }, { "frame" }, false),
            toolDefinition("flipman_set_fps", "Set rational playback FPS while preserving frame indices; undoable.",
                           { { "numerator", integer },
                             { "denominator", integer },
                             { "dropFrame", QJsonObject { { "type", "boolean" } } } },
                           { "numerator" }, false),
            toolDefinition("flipman_set_display_profile",
                           "Change the native surface tag through command history; does not transform pixels.",
                           { { "name", QJsonObject { { "type", "string" }, { "enum", profiles } } } }, { "name" },
                           false),
            toolDefinition("flipman_play", "Start playback of the loaded timeline.", {}, {}, false),
            toolDefinition("flipman_stop", "Stop playback without changing the current frame.", {}, {}, false),
            toolDefinition("flipman_undo", "Undo the most recent session command.", {}, {}, false),
            toolDefinition("flipman_redo", "Redo the most recently undone session command.", {}, {}, false),
            toolDefinition(
                "flipman_run_python",
                "Execute Python in the shared GUI-thread interpreter. Scripts may block interaction or perform arbitrary application actions.",
                { { "code", string } }, { "code" }, false, true),
            toolDefinition("flipman_quit", "Request application exit after the response is sent.", {}, {}, false, true)
        };
    }

}  // namespace

class McpServerPrivate {
public:
    explicit McpServerPrivate(McpServer* owner)
    {
        d.object = owner;
        d.server = new QTcpServer(owner);
        QObject::connect(d.server, &QTcpServer::newConnection, d.object, [this]() { acceptConnections(); });
    }

    void acceptConnections()
    {
        while (d.server->hasPendingConnections()) {
            QTcpSocket* socket = d.server->nextPendingConnection();
            if (!socket)
                continue;
            socket->setReadBufferSize(maxHeaderSize + maxBodySize);
            QTimer::singleShot(10000, socket, [socket]() { socket->abort(); });
            QObject::connect(socket, &QTcpSocket::readyRead, d.object, [this, socket]() { readRequest(socket); });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    void writeResponse(QTcpSocket* socket, int status, const QByteArray& reason, const QByteArray& body = {},
                       const QByteArray& contentType = "application/json", const QByteArray& corsOrigin = {})
    {
        if (!socket)
            return;

        QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
        response += "Content-Type: " + contentType + "\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Cache-Control: no-store\r\nConnection: close\r\n";
        if (!corsOrigin.isEmpty()) {
            response += "Access-Control-Allow-Origin: " + corsOrigin + "\r\n";
            response += "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
            response += "Access-Control-Allow-Headers: Content-Type, Accept, Mcp-Protocol-Version\r\n";
            response += "Vary: Origin\r\n";
        }
        response += "\r\n";
        response += body;
        socket->write(response);
        socket->disconnectFromHost();
    }

    QByteArray errorBody(int code, const QString& message)
    {
        return QJsonDocument(jsonRpcError(QJsonValue(QJsonValue::Null), code, message)).toJson(QJsonDocument::Compact);
    }

    bool allowedHost(const QByteArray& host) const
    {
        const QByteArray port = QByteArray::number(d.server->serverPort());
        return host.compare("127.0.0.1:" + port, Qt::CaseInsensitive) == 0
               || host.compare("localhost:" + port, Qt::CaseInsensitive) == 0;
    }

    QByteArray allowedOrigin(const QByteArray& origin) const
    {
        if (origin.isEmpty())
            return {};
        const QUrl url(QString::fromUtf8(origin));
        if (url.scheme() != QLatin1String("http") || url.port(-1) != d.server->serverPort()
            || (url.host() != QLatin1String("127.0.0.1") && url.host() != QLatin1String("localhost")))
            return {};
        return origin;
    }

    void readRequest(QTcpSocket* socket)
    {
        if (!socket || socket->property("mcpResponded").toBool())
            return;

        QByteArray buffer = socket->property("mcpRequestBuffer").toByteArray();
        buffer += socket->readAll();
        if (buffer.size() > maxHeaderSize + maxBodySize) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 413, "Payload Too Large", "Request body is too large.", "text/plain");
            return;
        }

        const qsizetype headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            if (buffer.size() > maxHeaderSize) {
                socket->setProperty("mcpResponded", true);
                writeResponse(socket, 431, "Request Header Fields Too Large", "Request headers are too large.",
                              "text/plain");
                return;
            }
            socket->setProperty("mcpRequestBuffer", buffer);
            return;
        }

        if (headerEnd > maxHeaderSize) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 431, "Request Header Fields Too Large", "Request headers are too large.",
                          "text/plain");
            return;
        }
        const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
        if (lines.isEmpty()) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 400, "Bad Request", "Malformed request.", "text/plain");
            return;
        }
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        if (requestLine.size() != 3) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 400, "Bad Request", "Malformed request line.", "text/plain");
            return;
        }

        QHash<QByteArray, QByteArray> headers;
        for (qsizetype i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines.at(i).trimmed();
            const qsizetype separator = line.indexOf(':');
            if (separator <= 0)
                continue;
            headers.insert(line.left(separator).trimmed().toLower(), line.mid(separator + 1).trimmed());
        }

        const QByteArray origin = allowedOrigin(headers.value("origin"));
        const QByteArray host = headers.value("host");
        if (!allowedHost(host) || (!headers.value("origin").isEmpty() && origin.isEmpty())) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 403, "Forbidden", "Only local MCP clients are allowed.", "text/plain");
            return;
        }

        const QByteArray method = requestLine.at(0);
        const QByteArray path = requestLine.at(1);
        if (method == "OPTIONS" && path == "/mcp") {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 204, "No Content", {}, "text/plain", origin);
            return;
        }
        if (method == "GET" && path == "/mcp") {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 405, "Method Not Allowed",
                          "This endpoint uses POST responses, without an SSE stream.", "text/plain", origin);
            return;
        }
        if (method != "POST" || path != "/mcp") {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 404, "Not Found", "MCP endpoint not found.", "text/plain", origin);
            return;
        }
        if (!headers.value("transfer-encoding").isEmpty()) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 400, "Bad Request", "Transfer-Encoding is not supported.", "text/plain", origin);
            return;
        }
        if (!headers.value("content-type").toLower().startsWith("application/json")) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, 415, "Unsupported Media Type", "Expected application/json.", "text/plain", origin);
            return;
        }

        bool lengthOk = false;
        const qlonglong contentLength = headers.value("content-length").toLongLong(&lengthOk);
        if (!lengthOk || contentLength < 1 || contentLength > maxBodySize) {
            socket->setProperty("mcpResponded", true);
            writeResponse(socket, contentLength > maxBodySize ? 413 : 400,
                          contentLength > maxBodySize ? "Payload Too Large" : "Bad Request", "Invalid Content-Length.",
                          "text/plain", origin);
            return;
        }

        const qsizetype bodyOffset = headerEnd + 4;
        if (buffer.size() - bodyOffset < contentLength) {
            socket->setProperty("mcpRequestBuffer", buffer);
            return;
        }

        socket->setProperty("mcpResponded", true);
        const QByteArray body = buffer.mid(bodyOffset, static_cast<qsizetype>(contentLength));
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            writeResponse(socket, 400, "Bad Request", errorBody(-32700, "Invalid JSON request."), "application/json",
                          origin);
            return;
        }

        bool hasResponse = false;
        const QJsonObject response = handleMessage(document.object(), &hasResponse);
        if (!hasResponse) {
            writeResponse(socket, 202, "Accepted", {}, "application/json", origin);
            return;
        }
        writeResponse(socket, 200, "OK", QJsonDocument(response).toJson(QJsonDocument::Compact), "application/json",
                      origin);
    }

    QJsonObject handleMessage(const QJsonObject& request, bool* hasResponse)
    {
        const QJsonValue id = request.value("id");
        const bool hasId = request.contains("id");
        if (request.value("jsonrpc").toString() != QLatin1String("2.0") || !request.value("method").isString()) {
            *hasResponse = true;
            return jsonRpcError(id, -32600, "Invalid Request");
        }

        const QString method = request.value("method").toString();
        const QJsonObject params = request.value("params").toObject();
        if (method.startsWith(QLatin1String("notifications/"))) {
            *hasResponse = false;
            return {};
        }
        if (!hasId) {
            *hasResponse = true;
            return jsonRpcError(QJsonValue(QJsonValue::Null), -32600, "Requests must include an id.");
        }

        *hasResponse = true;
        QJsonValue result;
        if (method == QLatin1String("initialize")) {
            result = QJsonObject {
                { "protocolVersion", "2025-11-25" },
                { "capabilities", QJsonObject { { "tools", QJsonObject { { "listChanged", false } } } } },
                { "serverInfo", QJsonObject { { "name", "Flipman" }, { "version", QStringLiteral("1.0") } } },
                { "instructions",
                  "Flipman tools operate on the current movie/image session. File opening and seeking are asynchronous. "
                  "Use native tools for command history; Python executes with the permissions of this desktop process." }
            };
        }
        else if (method == QLatin1String("ping")) {
            result = QJsonObject {};
        }
        else if (method == QLatin1String("tools/list")) {
            result = QJsonObject { { "tools", availableTools() } };
        }
        else if (method == QLatin1String("tools/call")) {
            const QString name = params.value("name").toString();
            const QJsonObject arguments = params.value("arguments").toObject();
            result = callTool(name, arguments);
        }
        else {
            *hasResponse = true;
            return jsonRpcError(id, -32601, QStringLiteral("Method not found: %1").arg(method));
        }

        return { { "jsonrpc", "2.0" }, { "id", id }, { "result", result } };
    }

    QJsonObject callTool(const QString& name, const QJsonObject& args)
    {
        auto* current = flipman::session();
        QJsonObject schema;
        for (const auto& value : availableTools())
            if (value.toObject().value("name").toString() == name)
                schema = value.toObject().value("inputSchema").toObject();
        if (schema.isEmpty())
            return toolResult("Unknown Flipman tool: " + name, true);

        const auto properties = schema.value("properties").toObject();
        for (const auto& required : schema.value("required").toArray())
            if (!args.contains(required.toString()))
                return toolResult("Missing argument: " + required.toString(), true);

        for (auto it = args.begin(); it != args.end(); ++it) {
            if (!properties.contains(it.key()))
                return toolResult("Unknown argument: " + it.key(), true);

            const auto type = properties.value(it.key()).toObject().value("type").toString();
            const auto value = it.value();
            if ((type == "string" && !value.isString()) || (type == "boolean" && !value.isBool())
                || (type == "integer"
                    && (!value.isDouble() || !std::isfinite(value.toDouble())
                        || std::floor(value.toDouble()) != value.toDouble())))
                return toolResult("Invalid argument type: " + it.key(), true);
        }
        if (name == "flipman_get_session_info") {
            const auto color = current->colorSettings();
            const auto tag = current->displayTransform();
            QString profileName;
            for (const auto& profile : displayProfiles())
                if (profile.transform.colorSpace == tag.colorSpace
                    && profile.transform.transferFunction == tag.transferFunction)
                    profileName = profile.name;
            return toolResult(jsonText(QJsonObject { { "filename", current->filename() },
                                                     { "title", current->title() },
                                                     { "loaded", current->isLoaded() },
                                                     { "loading", current->isLoading() },
                                                     { "status", current->status() },
                                                     { "frame", current->timeline()->time().frames() },
                                                     { "presentedFrame", current->presentedFrame() },
                                                     { "frameCount", current->frameCount() },
                                                     { "fpsNumerator", current->timeline()->fps().numerator() },
                                                     { "fpsDenominator", current->timeline()->fps().denominator() },
                                                     { "playing", current->timeline()->isPlaying() },
                                                     { "workingSpace", color.workingSpace },
                                                     { "output", color.display },
                                                     { "view", color.view },
                                                     { "displayProfile", profileName },
                                                     { "canUndo", current->commands()->canUndo() },
                                                     { "canRedo", current->commands()->canRedo() },
                                                     { "settingsDirectory", app()->settingsDirectory() } }));
        }
        if (name == "flipman_get_metadata") {
            QJsonObject result, common;
            const auto metadata = current->metadata();
            for (const auto& key : metadata.keys())
                common.insert(key, QJsonValue::fromVariant(metadata.value(key)));
            result.insert("common", common);
            for (const auto group : metadata.groups()) {
                QJsonObject values;
                for (const auto& key : metadata.keys(group))
                    values.insert(key, QJsonValue::fromVariant(metadata.value(group, key)));
                result.insert(sdk::core::MetaData::convert(group), values);
            }
            return toolResult(jsonText(result));
        }
        if (name == "flipman_open_file") {
            const auto file = args.value("filename").toString();
            if (!QFileInfo(file).isFile())
                return toolResult("File does not exist", true);

            current->openFile(file);
            return toolResult("Open requested. Poll flipman_get_session_info until loading is false.");
        }
        if (name == "flipman_close_file")
            current->closeFile();
        else if (name == "flipman_set_time") {
            const auto frame = args.value("frame").toDouble(-1);
            if (!current->isLoaded() || frame < 0 || frame >= double(current->frameCount())
                || frame > 9007199254740991.0)
                return toolResult("Frame is outside the loaded timeline", true);

            current->commands()->execute(
                timeCommand(current->timeline()->time(),
                            sdk::av::Time::fromFrames(qint64(frame), current->timeline()->fps())));
        }
        else if (name == "flipman_set_fps") {
            const auto n = args.value("numerator").toDouble(), denominator = args.value("denominator").toDouble(1);
            if (n < 1 || n > 2147483647 || denominator < 1 || denominator > 2147483647 || n / denominator > 1000)
                return toolResult("FPS must be a positive rational rate at most 1000", true);

            current->commands()->execute(
                fpsCommand(current->timeline()->fps(),
                           sdk::av::Fps(qint32(n), qint32(denominator), args.value("dropFrame").toBool())));
        }
        else if (name == "flipman_set_display_profile") {
            sdk::render::DisplayTransform tag;
            if (!findDisplayProfile(args.value("name").toString(), tag))
                return toolResult("Unknown display profile", true);

            current->commands()->execute(displayCommand(current->displayTransform(), tag));
        }
        else if (name == "flipman_play") {
            if (!current->isLoaded() || current->isLoading())
                return toolResult("No ready timeline", true);

            if (!current->timeline()->isPlaying())
                current->togglePlaying();
        }
        else if (name == "flipman_stop")
            current->timeline()->stop();
        else if (name == "flipman_undo")
            current->commands()->undo();
        else if (name == "flipman_redo")
            current->commands()->redo();
        else if (name == "flipman_run_python") {
            const auto code = args.value("code").toString();
            if (code.isEmpty() || code.toUtf8().size() > 64 * 1024)
                return toolResult("Python source must contain 1..65536 bytes", true);

            const auto output = pythonInterpreter()->executeScript(code);
            return toolResult(output, output.contains("Traceback (most recent call last):"));
        }
        else if (name == "flipman_quit")
            QTimer::singleShot(100, app(), &QCoreApplication::quit);
        return toolResult("Done");
    }

    struct Data {
        QTcpServer* server = nullptr;
        QString error;
        QPointer<McpServer> object;
    };
    Data d;
};

McpServer::McpServer(QObject* parent)
    : QObject(parent)
    , p(new McpServerPrivate(this))
{}

McpServer::~McpServer() { stop(); }

bool
McpServer::start()
{
    if (p->d.server->isListening())
        return true;

    quint16 port = defaultPort;
    const QByteArray testPort = qgetenv("FLIPMAN_TEST_MCP_PORT");
    if (!testPort.isEmpty()) {
        bool valid = false;
        const int value = testPort.toInt(&valid);
        if (!valid || value < 0 || value > 65535) {
            p->d.error = QStringLiteral("Invalid FLIPMAN_TEST_MCP_PORT: expected 0..65535");
            Q_EMIT errorOccurred(p->d.error);
            return false;
        }
        port = static_cast<quint16>(value);
    }
    if (!p->d.server->listen(QHostAddress::LocalHost, port)) {
        p->d.error = p->d.server->errorString();
        Q_EMIT errorOccurred(p->d.error);
        return false;
    }
    p->d.error.clear();
    Q_EMIT runningChanged(true);
    return true;
}

void
McpServer::stop()
{
    if (!p->d.server->isListening())
        return;

    p->d.server->close();
    const auto sockets = p->d.server->findChildren<QTcpSocket*>();
    for (QTcpSocket* socket : sockets)
        socket->disconnectFromHost();
    Q_EMIT runningChanged(false);
}

bool
McpServer::isRunning() const
{
    return p->d.server->isListening();
}

QUrl
McpServer::url() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/mcp").arg(isRunning() ? p->d.server->serverPort() : defaultPort));
}

QString
McpServer::errorString() const
{
    return p->d.error;
}

}  // namespace flipman
