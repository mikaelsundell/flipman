// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "appconfig.h"
#include "commands/commanddisplay.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/console.h"
#include "core/displayprofiles.h"
#include "core/session.h"
#include "core/settings.h"
#include "mcp/mcpserver.h"
#include "pythoninterpreter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QScopeGuard>
#include <QThread>
#include <QTimer>

#if FLIPMAN_APP_PYTHON
#    include <Python.h>
#endif

namespace flipman {

class PythonInterpreterPrivate {
public:
    struct Data {
#if FLIPMAN_APP_PYTHON
        PyObject* globals = nullptr;
        PyThreadState* threadState = nullptr;
#endif
        bool initialized = false;
        bool ownsInterpreter = false;
        QPointer<Session> session;
    };
    Data d;
};

#if FLIPMAN_APP_PYTHON
namespace {

    Session* sessionFor(PyObject* self)
    {
        auto* pointer = static_cast<QPointer<Session>*>(PyCapsule_GetPointer(self, "flipman.Session"));
        if (!pointer || !*pointer) {
            PyErr_SetString(PyExc_RuntimeError, "The application session has closed");
            return nullptr;
        }
        if ((*pointer)->thread() != QThread::currentThread()) {
            PyErr_SetString(PyExc_RuntimeError, "Use the session on the application thread");
            return nullptr;
        }
        return pointer->data();
    }

    PyObject* state(PyObject* self, PyObject*)
    {
        auto* s = sessionFor(self);
        if (!s)
            return nullptr;

        auto* result = PyDict_New();
        auto number = [result](const char* name, qint64 value) {
            auto* v = PyLong_FromLongLong(value);
            PyDict_SetItemString(result, name, v);
            Py_DECREF(v);
        };
        auto text = [result](const char* name, const QString& value) {
            auto* v = PyUnicode_FromString(value.toUtf8().constData());
            PyDict_SetItemString(result, name, v);
            Py_DECREF(v);
        };
        auto boolean = [result](const char* name, bool value) {
            PyDict_SetItemString(result, name, value ? Py_True : Py_False);
        };
        text("filename", s->filename());
        text("title", s->title());
        number("frame", s->timeline()->time().frames());
        number("frame_count", s->frameCount());
        number("fps_numerator", s->timeline()->fps().numerator());
        number("fps_denominator", s->timeline()->fps().denominator());
        boolean("drop_frame", s->timeline()->fps().dropFrame());
        boolean("playing", s->timeline()->isPlaying());
        boolean("loaded", s->isLoaded());
        boolean("loading", s->isLoading());
        return result;
    }

    PyObject* openFile(PyObject* self, PyObject* args)
    {
        auto* s = sessionFor(self);
        const char* path;
        if (!s || !PyArg_ParseTuple(args, "s", &path))
            return nullptr;

        s->openFile(QString::fromUtf8(path));
        Py_RETURN_NONE;
    }

    PyObject* setFrame(PyObject* self, PyObject* args)
    {
        auto* s = sessionFor(self);
        long long frame;
        if (!s || !PyArg_ParseTuple(args, "L", &frame))
            return nullptr;

        if (!s->isLoaded() || frame < 0 || frame >= s->frameCount()) {
            PyErr_SetString(PyExc_ValueError, "Frame is outside the loaded timeline");
            return nullptr;
        }
        s->commands()->execute(
            timeCommand(s->timeline()->time(), sdk::av::Time::fromFrames(frame, s->timeline()->fps())));
        Py_RETURN_NONE;
    }

    PyObject* setFps(PyObject* self, PyObject* args)
    {
        auto* s = sessionFor(self);
        int n, d, drop;
        if (!s || !PyArg_ParseTuple(args, "iip", &n, &d, &drop))
            return nullptr;

        if (n <= 0 || d <= 0 || double(n) / d > 1000) {
            PyErr_SetString(PyExc_ValueError, "FPS must be positive and at most 1000");
            return nullptr;
        }
        s->commands()->execute(fpsCommand(s->timeline()->fps(), sdk::av::Fps(n, d, drop)));
        Py_RETURN_NONE;
    }

    PyObject* setDisplay(PyObject* self, PyObject* args)
    {
        auto* s = sessionFor(self);
        const char* name;
        if (!s || !PyArg_ParseTuple(args, "s", &name))
            return nullptr;

        sdk::render::DisplayTransform tag;
        const QString profile = QString::fromUtf8(name);
        if (!findDisplayProfile(profile, tag)) {
            PyErr_SetString(PyExc_ValueError, "Unknown display profile");
            return nullptr;
        }
        s->commands()->execute(displayCommand(s->displayTransform(), tag));
        Py_RETURN_NONE;
    }

    PyObject* undo(PyObject* self, PyObject*)
    {
        auto* s = sessionFor(self);
        if (!s)
            return nullptr;

        s->commands()->undo();
        Py_RETURN_NONE;
    }

    PyObject* redo(PyObject* self, PyObject*)
    {
        auto* s = sessionFor(self);
        if (!s)
            return nullptr;

        s->commands()->redo();
        Py_RETURN_NONE;
    }

    PyObject* play(PyObject* self, PyObject*)
    {
        auto* s = sessionFor(self);
        if (!s)
            return nullptr;

        if (s->isLoaded())
            s->timeline()->play();
        Py_RETURN_NONE;
    }

    PyObject* stop(PyObject* self, PyObject*)
    {
        auto* s = sessionFor(self);
        if (!s)
            return nullptr;

        s->timeline()->stop();
        Py_RETURN_NONE;
    }

    PyObject* applicationInfo(PyObject* self, PyObject*)
    {
        if (!sessionFor(self))
            return nullptr;

        return Py_BuildValue("{s:s,s:s,s:O,s:O,s:s}", "settingsDirectory",
                             app()->settingsDirectory().toUtf8().constData(), "settingsFilename",
                             settings()->filename().toUtf8().constData(), "isolated",
                             settings()->isIsolated() ? Py_True : Py_False, "mcpRunning",
                             mcpServer()->isRunning() ? Py_True : Py_False, "mcpUrl",
                             mcpServer()->url().toString().toUtf8().constData());
    }

    PyObject* startMcp(PyObject* self, PyObject*)
    {
        if (!sessionFor(self))
            return nullptr;

        if (!mcpServer()->start()) {
            PyErr_SetString(PyExc_RuntimeError, mcpServer()->errorString().toUtf8().constData());
            return nullptr;
        }
        Py_RETURN_TRUE;
    }

    PyObject* stopMcp(PyObject* self, PyObject*)
    {
        if (!sessionFor(self))
            return nullptr;

        mcpServer()->stop();
        Py_RETURN_NONE;
    }

    PyObject* quitApplication(PyObject* self, PyObject*)
    {
        if (!sessionFor(self))
            return nullptr;

        QTimer::singleShot(100, app(), &QCoreApplication::quit);
        Py_RETURN_NONE;
    }

    PyObject* settingsValue(PyObject* self, PyObject* args)
    {
        const char* key;
        PyObject* fallback = Py_None;
        if (!sessionFor(self) || !PyArg_ParseTuple(args, "s|O", &key, &fallback))
            return nullptr;

        const auto value = settings()->value(QString::fromUtf8(key));
        if (!value.isValid()) {
            Py_INCREF(fallback);
            return fallback;
        }
        if (value.typeId() == QMetaType::QByteArray) {
            const auto bytes = value.toByteArray();
            return PyBytes_FromStringAndSize(bytes.constData(), bytes.size());
        }
        if (value.typeId() == QMetaType::LongLong)
            return PyLong_FromLongLong(value.toLongLong());

        if (value.typeId() == QMetaType::ULongLong)
            return PyLong_FromUnsignedLongLong(value.toULongLong());

        auto* json = PyImport_ImportModule("json");
        if (!json)
            return nullptr;

        const auto bytes = QJsonDocument(QJsonArray { QJsonValue::fromVariant(value) }).toJson(QJsonDocument::Compact);
        auto* list = PyObject_CallMethod(json, "loads", "s", bytes.constData());
        Py_DECREF(json);
        if (!list)
            return nullptr;

        auto* result = PyList_GetItem(list, 0);
        Py_XINCREF(result);
        Py_DECREF(list);
        return result;
    }

    PyObject* setSettingsValue(PyObject* self, PyObject* args)
    {
        const char* key;
        PyObject* value;
        if (!sessionFor(self) || !PyArg_ParseTuple(args, "sO", &key, &value))
            return nullptr;

        if (!*key) {
            PyErr_SetString(PyExc_ValueError, "Preference key must not be empty");
            return nullptr;
        }
        QVariant converted;
        if (PyBytes_Check(value))
            converted = QByteArray(PyBytes_AsString(value), PyBytes_Size(value));
        else if (PyBool_Check(value))
            converted = value == Py_True;
        else if (PyLong_Check(value)) {
            const auto number = PyLong_AsLongLong(value);
            if (PyErr_Occurred())
                return nullptr;

            converted = QVariant::fromValue<qlonglong>(number);
        }
        else {
            auto* json = PyImport_ImportModule("json");
            if (!json)
                return nullptr;

            auto* list = PyList_New(1);
            Py_INCREF(value);
            PyList_SET_ITEM(list, 0, value);
            auto* encoded = PyObject_CallMethod(json, "dumps", "O", list);
            Py_DECREF(list);
            Py_DECREF(json);
            if (!encoded)
                return nullptr;

            const auto document = QJsonDocument::fromJson(QByteArray(PyUnicode_AsUTF8(encoded)));
            Py_DECREF(encoded);
            if (!document.isArray()) {
                PyErr_SetString(PyExc_ValueError, "Preference value must be finite JSON data or bytes");
                return nullptr;
            }
            converted = document.array().first().toVariant();
        }
        settings()->setValue(QString::fromUtf8(key), converted);
        settings()->sync();
        Py_RETURN_NONE;
    }

    PyObject* consoleAction(PyObject* self, PyObject* args)
    {
        const char* action;
        if (!sessionFor(self) || !PyArg_ParseTuple(args, "s", &action))
            return nullptr;

        const QByteArray requested(action);
        if (requested == "start")
            return PyBool_FromLong(console()->start());

        if (requested == "stop") {
            console()->stop();
            Py_RETURN_NONE;
        }
        if (requested == "running")
            return PyBool_FromLong(console()->isRunning());

        if (requested == "text")
            return PyUnicode_FromString(console()->text().toUtf8().constData());

        PyErr_SetString(PyExc_ValueError, "Unknown console operation");
        return nullptr;
    }

    PyMethodDef methods[] = { { "_settings_get", settingsValue, METH_VARARGS, nullptr },
                              { "_settings_set", setSettingsValue, METH_VARARGS, nullptr },
                              { "_console", consoleAction, METH_VARARGS, nullptr },
                              { "_application_info", applicationInfo, METH_NOARGS, nullptr },
                              { "_start_mcp", startMcp, METH_NOARGS, nullptr },
                              { "_stop_mcp", stopMcp, METH_NOARGS, nullptr },
                              { "quit", quitApplication, METH_NOARGS, nullptr },
                              { "_state", state, METH_NOARGS, nullptr },
                              { "_open_file", openFile, METH_VARARGS, nullptr },
                              { "_set_frame", setFrame, METH_VARARGS, nullptr },
                              { "_set_fps", setFps, METH_VARARGS, nullptr },
                              { "_set_display", setDisplay, METH_VARARGS, nullptr },
                              { "_undo", undo, METH_NOARGS, nullptr },
                              { "_redo", redo, METH_NOARGS, nullptr },
                              { "_play", play, METH_NOARGS, nullptr },
                              { "_stop", stop, METH_NOARGS, nullptr },
                              { nullptr, nullptr, 0, nullptr } };

    QString pythonError()
    {
        PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
        PyErr_Fetch(&type, &value, &traceback);
        auto* text = value ? PyObject_Str(value) : nullptr;
        const QString message = text ? QString::fromUtf8(PyUnicode_AsUTF8(text))
                                     : QStringLiteral("Python initialization failed");
        Py_XDECREF(text);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        return message;
    }

}  // namespace
#endif

PythonInterpreter::PythonInterpreter(Session* session)
    : QObject(nullptr)
    , p(new PythonInterpreterPrivate)
{
    p->d.session = session;
}

PythonInterpreter::~PythonInterpreter()
{
#if FLIPMAN_APP_PYTHON
    if (p->d.initialized) {
        if (p->d.threadState)
            PyEval_RestoreThread(p->d.threadState);
        Py_XDECREF(p->d.globals);
        if (p->d.ownsInterpreter)
            Py_FinalizeEx();
    }
#endif
}

QString
PythonInterpreter::executeScript(const QString& script)
{
#if FLIPMAN_APP_PYTHON
    if (thread() != QThread::currentThread())
        return "Python must run on the application thread";

    p->d.session->timeline()->stop();
    if (!p->d.initialized) {
        if (Py_IsInitialized())
            return "An external Python interpreter is already active";

        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.parse_argv = 0;
        config.site_import = 0;
        PyStatus status = PyConfig_SetBytesString(&config, &config.home, FLIPMAN_PYTHON_HOME);
        if (!PyStatus_Exception(status))
            status = PyConfig_SetBytesString(&config, &config.program_name, FLIPMAN_PYTHON_EXECUTABLE);
        if (!PyStatus_Exception(status))
            status = Py_InitializeFromConfig(&config);
        const QString failure = PyStatus_Exception(status)
                                    ? QString::fromUtf8(status.err_msg ? status.err_msg : "Python startup failed")
                                    : QString();
        PyConfig_Clear(&config);
        if (!failure.isEmpty())
            return failure;

        p->d.initialized = p->d.ownsInterpreter = true;
        auto* path = PyUnicode_FromString(FLIPMAN_PYTHON_MODULE_DIR);
        PyList_Insert(PySys_GetObject("path"), 0, path);
        Py_DECREF(path);
        auto* module = PyModule_New("flipman_app");
        auto* capsule = PyCapsule_New(new QPointer<Session>(p->d.session), "flipman.Session", [](PyObject* object) {
            delete static_cast<QPointer<Session>*>(PyCapsule_GetPointer(object, "flipman.Session"));
        });
        for (auto* method = methods; method->ml_name; ++method) {
            auto* function = PyCFunction_NewEx(method, capsule, nullptr);
            PyModule_AddObject(module, method->ml_name, function);
        }
        Py_DECREF(capsule);
        PyDict_SetItemString(PyImport_GetModuleDict(), "flipman_app", module);
        Py_DECREF(module);
        p->d.globals = PyDict_New();
        PyDict_SetItemString(p->d.globals, "__builtins__", PyEval_GetBuiltins());
        auto* result = PyRun_String(FLIPMAN_PYTHON_BOOTSTRAP, Py_file_input, p->d.globals, p->d.globals);
        if (!result) {
            const auto error = pythonError();
            p->d.threadState = PyEval_SaveThread();
            return error;
        }
        Py_DECREF(result);
        p->d.threadState = PyEval_SaveThread();
    }
    const auto gil = PyGILState_Ensure();
    const auto releaseGil = qScopeGuard([gil]() { PyGILState_Release(gil); });
    auto* function = PyDict_GetItemString(p->d.globals, "_run_script");
    if (!function)
        return "Python console initialization failed";

    auto* result = PyObject_CallFunction(function, "s", script.toUtf8().constData());
    if (!result)
        return pythonError();

    const auto output = QString::fromUtf8(PyUnicode_AsUTF8(result));
    Py_DECREF(result);
    return output;
#else
    Q_UNUSED(script);
    return "This build has no embedded Python support";
#endif
}

}  // namespace flipman
