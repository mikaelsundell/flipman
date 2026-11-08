// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "bindings.h"
#include <QDateTime>
#include <QRect>
#include <QSize>
#include <QVariant>
#include <cstring>
#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/filerange.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/imageformat.h>
#include <flipmansdk/core/metadata.h>

namespace flipman::sdk::python {
namespace {
    nb::object fromVariant(const QVariant& value)
    {
        switch (value.typeId()) {
        case QMetaType::UnknownType: return nb::none();
        case QMetaType::Bool: return nb::bool_(value.toBool());
        case QMetaType::Int:
        case QMetaType::LongLong: return nb::int_(value.toLongLong());
        case QMetaType::UInt:
        case QMetaType::ULongLong: return nb::int_(value.toULongLong());
        case QMetaType::Float:
        case QMetaType::Double: return nb::float_(value.toDouble());
        case QMetaType::QString: return nb::cast(value.toString());
        case QMetaType::QDateTime: return nb::cast(value.toDateTime().toString(Qt::ISODateWithMs));
        case QMetaType::QStringList: return list(value.toStringList());
        case QMetaType::QVariantList: {
            nb::list result;
            for (const auto& item : value.toList())
                result.append(fromVariant(item));
            return result;
        }
        case QMetaType::QVariantMap: {
            nb::dict result;
            const auto map = value.toMap();
            for (auto it = map.cbegin(); it != map.cend(); ++it)
                result[nb::cast(it.key())] = fromVariant(it.value());
            return result;
        }
        default: throw nb::type_error("unsupported native metadata value type");
        }
    }
}  // namespace
QVariant
toVariant(nb::handle value, unsigned depth)
{
    if (depth > 32)
        throw nb::value_error("metadata nesting exceeds 32 levels");
    if (value.is_none())
        return {};
    if (nb::isinstance<nb::bool_>(value))
        return nb::cast<bool>(value);
    if (nb::isinstance<nb::int_>(value))
        return QVariant::fromValue(nb::cast<qint64>(value));
    if (nb::isinstance<nb::float_>(value))
        return nb::cast<double>(value);
    if (nb::isinstance<nb::str>(value))
        return nb::cast<QString>(value);
    if (nb::isinstance<nb::list>(value) || nb::isinstance<nb::tuple>(value)) {
        QVariantList result;
        for (auto item : nb::borrow<nb::iterable>(value))
            result.append(toVariant(item, depth + 1));
        return result;
    }
    if (nb::isinstance<nb::dict>(value)) {
        QVariantMap result;
        for (auto item : nb::borrow<nb::dict>(value))
            result.insert(nb::cast<QString>(item.first), toVariant(item.second, depth + 1));
        return result;
    }
    throw nb::type_error("metadata requires None, bool, int64, float, str, list, tuple or string-keyed dict");
}
namespace {
    void validPlane(const core::ImageBuffer& image, int plane)
    {
        if (!image.isAllocated())
            throw nb::value_error("image has no allocated storage");
        if (plane < 0 || plane >= image.planeCount())
            throw nb::index_error("plane index out of range");
    }
}  // namespace
void
bindCore(nb::module_& module)
{
    nb::class_<QSize>(module, "Size")
        .def(nb::init<int, int>(), "width"_a, "height"_a)
        .def("width", &QSize::width)
        .def("height", &QSize::height);
    nb::class_<QRect>(module, "Rect")
        .def(nb::init<int, int, int, int>(), "x"_a, "y"_a, "width"_a, "height"_a)
        .def("x", &QRect::x)
        .def("y", &QRect::y)
        .def("width", &QRect::width)
        .def("height", &QRect::height);
    nb::class_<core::Error>(module, "Error")
        .def(nb::init<>())
        .def(nb::init<const QString&, const QString&, int>(), "domain"_a, "message"_a, "code"_a = 0)
        .def("domain", &core::Error::domain)
        .def("message", &core::Error::message)
        .def("code", &core::Error::code)
        .def("isValid", &core::Error::isValid)
        .def("hasError", &core::Error::hasError)
        .def("__bool__", &core::Error::hasError);
    nb::class_<core::File>(module, "File")
        .def(nb::init<>())
        .def(nb::init<const QString&>(), "path"_a)
        .def("filePath", &core::File::filePath)
        .def("absolutePath", &core::File::absolutePath)
        .def("fileName", nb::overload_cast<>(&core::File::fileName, nb::const_))
        .def("fileName", nb::overload_cast<qint64>(&core::File::fileName, nb::const_), "frame"_a)
        .def("extension", &core::File::extension)
        .def("displayName", &core::File::displayName)
        .def("dirName", &core::File::dirName)
        .def("baseName", &core::File::baseName)
        .def("size", &core::File::size)
        .def("exists", &core::File::exists)
        .def("isValid", &core::File::isValid)
        .def("fileRange", &core::File::fileRange)
        .def("__str__", &core::File::filePath)
        .def_static("listDir", [](const QString& path) { return list(core::File::listDir(path)); }, "path"_a);
    nb::class_<core::FileRange>(module, "FileRange")
        .def(nb::init<>())
        .def("size", &core::FileRange::size)
        .def("isValid", &core::FileRange::isValid)
        .def("hasFrame", &core::FileRange::hasFrame, "frame"_a)
        .def(
            "frame",
            [](const core::FileRange& self, qint64 frame) {
                if (!self.hasFrame(frame))
                    throw nb::index_error("frame is not mapped");
                return self.frame(frame);
            },
            "frame"_a)
        .def("start",
             [](const core::FileRange& self) {
                 if (!self.isValid())
                     throw nb::value_error("range is empty");
                 return self.start();
             })
        .def("end",
             [](const core::FileRange& self) {
                 if (!self.isValid())
                     throw nb::value_error("range is empty");
                 return self.end();
             })
        .def("insertFrame", &core::FileRange::insertFrame, "frame"_a, "file"_a);
    auto metadata
        = nb::class_<core::MetaData>(module, "MetaData")
              .def(nb::init<>())
              .def(nb::init<const core::MetaData&>(), "other"_a)
              .def("isValid", &core::MetaData::isValid)
              .def("reset", &core::MetaData::reset)
              .def("groups", [](const core::MetaData& self) { return list(self.groups()); })
              .def(
                  "keys",
                  [](const core::MetaData& self, core::MetaData::Group group) { return list(self.keys(group)); },
                  "group"_a)
              .def(
                  "value", [](const core::MetaData& self, const QString& key) { return fromVariant(self.value(key)); },
                  "key"_a)
              .def(
                  "value",
                  [](const core::MetaData& self, core::MetaData::Group group, const QString& key) {
                      return fromVariant(self.value(group, key));
                  },
                  "group"_a, "key"_a)
              .def(
                  "insert",
                  [](core::MetaData& self, const QString& key, nb::handle value) { self.insert(key, toVariant(value)); },
                  "key"_a, "value"_a)
              .def(
                  "insert",
                  [](core::MetaData& self, core::MetaData::Group group, const QString& key, nb::handle value) {
                      self.insert(group, key, toVariant(value));
                  },
                  "group"_a, "key"_a, "value"_a)
              .def("remove", nb::overload_cast<const QString&>(&core::MetaData::remove), "key"_a)
              .def("remove", nb::overload_cast<core::MetaData::Group, const QString&>(&core::MetaData::remove),
                   "group"_a, "key"_a);
    auto group = nb::enum_<core::MetaData::Group>(metadata, "Group");
    group.value("Container", core::MetaData::Group::Container);
    group.value("Video", core::MetaData::Group::Video);
    group.value("Audio", core::MetaData::Group::Audio);
    group.value("Camera", core::MetaData::Group::Camera);
    group.value("Timecode", core::MetaData::Group::Timecode);
    group.value("Production", core::MetaData::Group::Production);
    group.value("Custom", core::MetaData::Group::Custom);
    auto format = nb::class_<core::ImageFormat>(module, "ImageFormat")
                      .def(nb::init<core::ImageFormat::Type>(), "type"_a)
                      .def("type", &core::ImageFormat::type)
                      .def("size", &core::ImageFormat::size)
                      .def("isValid", &core::ImageFormat::isValid);
    auto pixelType = nb::enum_<core::ImageFormat::Type>(format, "Type");
    pixelType.value("Unknown", core::ImageFormat::Unknown);
    pixelType.value("UInt8", core::ImageFormat::UInt8);
    pixelType.value("Int8", core::ImageFormat::Int8);
    pixelType.value("UInt16", core::ImageFormat::UInt16);
    pixelType.value("Int16", core::ImageFormat::Int16);
    pixelType.value("UInt32", core::ImageFormat::UInt32);
    pixelType.value("Int32", core::ImageFormat::Int32);
    pixelType.value("UInt64", core::ImageFormat::UInt64);
    pixelType.value("Int64", core::ImageFormat::Int64);
    pixelType.value("Half", core::ImageFormat::Half);
    pixelType.value("Float", core::ImageFormat::Float);
    pixelType.value("Double", core::ImageFormat::Double);
    auto image
        = nb::class_<core::ImageBuffer>(module, "ImageBuffer")
              .def(nb::init<>())
              .def(
                  "__init__",
                  [](core::ImageBuffer* self, int width, int height, core::ImageFormat::Type type, int channels) {
                      if (width <= 0 || height <= 0 || channels <= 0 || channels > 16
                          || type == core::ImageFormat::Unknown)
                          throw nb::value_error(
                              "positive image dimensions, 1..16 channels and a valid pixel type are required");
                      const QRect rect(0, 0, width, height);
                      new (self) core::ImageBuffer(rect, rect, core::ImageFormat(type), channels);
                  },
                  "width"_a, "height"_a, "type"_a = core::ImageFormat::UInt8, "channels"_a = 4)
              .def("allocate",
                   [](core::ImageBuffer& self) {
                       if (!self.isValid() || self.dataWindow().isEmpty())
                           throw nb::value_error("image descriptor is invalid");
                       if (self.subsampling() == core::ImageBuffer::Subsampling::CS420
                           && (self.dataWindow().width() % 2 || self.dataWindow().height() % 2))
                           throw nb::value_error("4:2:0 requires even dimensions");
                       if (self.subsampling() == core::ImageBuffer::Subsampling::CS422 && self.dataWindow().width() % 2)
                           throw nb::value_error("4:2:2 requires an even width");
                       self.allocate();
                   })
              .def("isValid", &core::ImageBuffer::isValid)
              .def("isAllocated", &core::ImageBuffer::isAllocated)
              .def("dataWindow", &core::ImageBuffer::dataWindow)
              .def("displayWindow", &core::ImageBuffer::displayWindow)
              .def("imageFormat", &core::ImageBuffer::imageFormat)
              .def("channels", &core::ImageBuffer::channels)
              .def("byteSize", &core::ImageBuffer::byteSize)
              .def("strideSize", &core::ImageBuffer::strideSize)
              .def("planeCount", &core::ImageBuffer::planeCount)
              .def(
                  "planeSize",
                  [](const core::ImageBuffer& self, int plane) {
                      validPlane(self, plane);
                      return self.planeSize(plane);
                  },
                  "plane"_a)
              .def(
                  "planeStride",
                  [](const core::ImageBuffer& self, int plane) {
                      validPlane(self, plane);
                      return self.planeStride(plane);
                  },
                  "plane"_a)
              .def(
                  "planeBytes",
                  [](const core::ImageBuffer& self, int plane) {
                      validPlane(self, plane);
                      return nb::bytes(reinterpret_cast<const char*>(self.planeData(plane)), self.planeByteSize(plane));
                  },
                  "plane"_a = 0)
              .def(
                  "setPlaneBytes",
                  [](core::ImageBuffer& self, nb::bytes data, int plane) {
                      validPlane(self, plane);
                      if (data.size() != self.planeByteSize(plane))
                          throw nb::value_error("byte count does not match plane storage");
                      self.detach();
                      std::memcpy(self.planeData(plane), data.c_str(), data.size());
                  },
                  "data"_a, "plane"_a = 0)
              .def("detach", &core::ImageBuffer::detach)
              .def("requiresDecode", &core::ImageBuffer::requiresDecode)
              .def("isYCbCr", &core::ImageBuffer::isYCbCr)
              .def("isRgb", &core::ImageBuffer::isRgb);
    auto packing = nb::enum_<core::ImageBuffer::Packing>(image, "Packing");
    packing.value("Interleaved", core::ImageBuffer::Packing::Interleaved);
    packing.value("Planar", core::ImageBuffer::Packing::Planar);
    packing.value("BiPlanar", core::ImageBuffer::Packing::BiPlanar);
    packing.value("Packed", core::ImageBuffer::Packing::Packed);
    auto subsampling = nb::enum_<core::ImageBuffer::Subsampling>(image, "Subsampling");
    subsampling.value("None_", core::ImageBuffer::Subsampling::None);
    subsampling.value("CS420", core::ImageBuffer::Subsampling::CS420);
    subsampling.value("CS422", core::ImageBuffer::Subsampling::CS422);
    subsampling.value("CS444", core::ImageBuffer::Subsampling::CS444);
    auto pixellayout = nb::enum_<core::ImageBuffer::PixelLayout>(image, "PixelLayout");
    pixellayout.value("Unknown", core::ImageBuffer::PixelLayout::Unknown);
    pixellayout.value("RGB", core::ImageBuffer::PixelLayout::RGB);
    pixellayout.value("BGR", core::ImageBuffer::PixelLayout::BGR);
    pixellayout.value("RGBA", core::ImageBuffer::PixelLayout::RGBA);
    pixellayout.value("BGRA", core::ImageBuffer::PixelLayout::BGRA);
    pixellayout.value("NV12", core::ImageBuffer::PixelLayout::NV12);
    pixellayout.value("NV21", core::ImageBuffer::PixelLayout::NV21);
    pixellayout.value("UYVY", core::ImageBuffer::PixelLayout::UYVY);
    pixellayout.value("YUYV", core::ImageBuffer::PixelLayout::YUYV);
    pixellayout.value("YVYU", core::ImageBuffer::PixelLayout::YVYU);
    pixellayout.value("VYUY", core::ImageBuffer::PixelLayout::VYUY);
    pixellayout.value("V210", core::ImageBuffer::PixelLayout::V210);
    auto pixelrange = nb::enum_<core::ImageBuffer::PixelRange>(image, "PixelRange");
    pixelrange.value("Unknown", core::ImageBuffer::PixelRange::Unknown);
    pixelrange.value("Full", core::ImageBuffer::PixelRange::Full);
    pixelrange.value("Video", core::ImageBuffer::PixelRange::Video);
    image.def("packing", &core::ImageBuffer::packing);
    image.def(
        "setPacking",
        [](core::ImageBuffer& self, core::ImageBuffer::Packing value) {
            if (self.isAllocated())
                throw nb::value_error("configure layout before allocation");
            self.setPacking(value);
        },
        "value"_a);
    image.def("subsampling", &core::ImageBuffer::subsampling);
    image.def(
        "setSubsampling",
        [](core::ImageBuffer& self, core::ImageBuffer::Subsampling value) {
            if (self.isAllocated())
                throw nb::value_error("configure layout before allocation");
            self.setSubsampling(value);
        },
        "value"_a);
    image.def("pixelLayout", &core::ImageBuffer::pixelLayout);
    image.def(
        "setPixelLayout",
        [](core::ImageBuffer& self, core::ImageBuffer::PixelLayout value) {
            if (self.isAllocated())
                throw nb::value_error("configure layout before allocation");
            self.setPixelLayout(value);
        },
        "value"_a);
    image.def("pixelRange", &core::ImageBuffer::pixelRange);
    image.def(
        "setPixelRange",
        [](core::ImageBuffer& self, core::ImageBuffer::PixelRange value) {
            if (self.isAllocated())
                throw nb::value_error("configure layout before allocation");
            self.setPixelRange(value);
        },
        "value"_a);
    image.def("colorSpace", &core::ImageBuffer::colorSpace);
    image.def("setColorSpace", &core::ImageBuffer::setColorSpace, "value"_a);
    image.def("transferFunction", &core::ImageBuffer::transferFunction);
    image.def("setTransferFunction", &core::ImageBuffer::setTransferFunction, "value"_a);
}
}  // namespace flipman::sdk::python
