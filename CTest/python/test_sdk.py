# SPDX-License-Identifier: BSD-3-Clause
"""Run one Python binding behavior suite per process, via CTest."""
import gc
import math
import os
from pathlib import Path
import struct
import sys
import tempfile
import threading
import unittest

import flipman as sdk


class TimeTests(unittest.TestCase):
    def test_rational_time_and_ranges(self):
        fps = sdk.Fps(24000, 1001)
        self.assertEqual(fps.numerator(), 24000)
        self.assertEqual(fps.denominator(), 1001)
        self.assertAlmostEqual(fps.real(), 24000 / 1001)
        time = sdk.Time.fromFrames(frame=24, fps=fps)
        self.assertEqual(time.frames(), 24)
        self.assertAlmostEqual(time.seconds(), 1.001)
        self.assertEqual(sdk.Time.convert(time, timeScale=48000).frames(), 24)
        start = sdk.Time.fromFrames(10, fps)
        duration = sdk.Time.fromFrames(5, fps)
        span = sdk.TimeRange(start=start, duration=duration)
        self.assertEqual(span.end().frames(), 15)
        self.assertEqual(span.bound(sdk.Time.fromFrames(18, fps), loop=True).frames(), 13)
        self.assertEqual((start + duration).frames(), 15)
        self.assertEqual((start - duration).frames(), 5)
        self.assertEqual(sdk.SmpteTime(sdk.Time.fromFrames(24, sdk.Fps.fps24())).seconds(), 1)

    def test_invalid_values_raise_instead_of_asserting(self):
        for args in [(24, 0), (0, 1), (-24, 1)]:
            with self.assertRaises(ValueError):
                sdk.Fps(*args)
        with self.assertRaises(ValueError):
            sdk.Fps().real()
        with self.assertRaises(ValueError):
            sdk.Time().frames()
        with self.assertRaises(ValueError):
            sdk.TimeRange().end()
        with self.assertRaises(ValueError):
            sdk.Time.fromFrames(0, sdk.Fps())
        with self.assertRaises(ValueError):
            sdk.Time.fromSeconds(float('nan'), sdk.Fps.fps24())
        with self.assertRaises(ValueError):
            sdk.Time.fromFrames(1, sdk.Fps.fps24()) + sdk.Time.fromFrames(1, sdk.Fps.fps25())
        self.assertEqual(str(sdk.Time()), "invalid")


class CoreTests(unittest.TestCase):
    def test_files_unicode_and_missing_frames(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "åäö 测试.txt"
            path.write_text("fixture")
            file = sdk.File(str(path))
            self.assertTrue(file.exists())
            self.assertEqual(file.fileName(), path.name)
            self.assertEqual(len(sdk.File.listDir(directory)), 1)
            span = sdk.FileRange()
            with self.assertRaises(ValueError):
                span.start()
            span.insertFrame(42, file)
            self.assertEqual(span.start(), 42)
            self.assertEqual(span.frame(42).fileName(), path.name)
            with self.assertRaises(IndexError):
                span.frame(0)

    def test_metadata_python_conversion_and_copy(self):
        meta = sdk.MetaData()
        value = {"标题": [None, True, 42, 0.25, "café"]}
        meta.insert(key="custom", value=value)
        self.assertEqual(meta.value("custom"), value)
        self.assertIn("custom", meta.keys())
        copy = sdk.MetaData(meta)
        copy.insert("custom", "changed")
        self.assertEqual(meta.value("custom"), value)
        group = sdk.MetaData.Group.Camera
        meta.insert(group, "iso", 800)
        self.assertEqual(meta.value(group, "iso"), 800)
        self.assertIn(group, meta.groups())
        self.assertEqual(meta.value("missing"), None)
        with self.assertRaises(TypeError):
            meta.insert("bad", object())
        with self.assertRaises((OverflowError, TypeError, RuntimeError)):
            meta.insert("huge", 2**80)
        error = sdk.Error("test", "message", code=7)
        self.assertTrue(error)
        self.assertEqual(error.code(), 7)


class ImageTests(unittest.TestCase):
    def test_storage_and_owned_bytes(self):
        image = sdk.ImageBuffer(width=4, height=2, channels=4)
        with self.assertRaises(ValueError):
            image.planeBytes()
        image.allocate()
        data = bytes([20, 40, 60, 255]) * 8
        image.setPlaneBytes(data)
        self.assertEqual(image.planeBytes(), data)
        self.assertEqual(image.planeStride(0), 16)
        with self.assertRaises(IndexError):
            image.planeBytes(-1)
        with self.assertRaises(ValueError):
            image.setPlaneBytes(b"short")
        snapshot = image.planeBytes()
        del image
        gc.collect()
        self.assertEqual(snapshot, data)

    def test_planar_layout_and_copy_isolation(self):
        image = sdk.ImageBuffer(4, 2, channels=1)
        image.setPacking(sdk.ImageBuffer.Packing.BiPlanar)
        image.setSubsampling(sdk.ImageBuffer.Subsampling.CS420)
        image.setPixelLayout(sdk.ImageBuffer.PixelLayout.NV12)
        image.allocate()
        self.assertEqual(image.planeCount(), 2)
        self.assertEqual(image.planeStride(1), 4)
        self.assertEqual(image.planeSize(1).height(), 1)
        image.setPlaneBytes(bytes([128]) * 4, plane=1)
        self.assertEqual(image.planeBytes(1), bytes([128]) * 4)
        with self.assertRaises(ValueError):
            image.setPacking(sdk.ImageBuffer.Packing.Planar)
        layer = sdk.ImageLayer()
        layer.setImage(image)
        other = layer.image()
        other.setPlaneBytes(bytes([64]) * 4, plane=1)
        self.assertEqual(image.planeBytes(1), bytes([128]) * 4)
        with self.assertRaises(ValueError):
            sdk.ImageBuffer(0, 2)

    def test_shader_parsing(self):
        definition = sdk.parseShader("vec4 effect(vec4 color, ivec2 pixel, ivec2 size) { return color; }")
        self.assertTrue(definition.isValid())
        self.assertIn("effect", definition.shaderCode())
        with self.assertRaises(RuntimeError):
            sdk.parseShader("vec4 effect(vec4 color) { return color;")
        effect = sdk.ImageEffect()
        effect.setName("identity")
        effect.setShaderDefinition(definition)
        self.assertEqual(effect.name(), "identity")


def fixture_image():
    image = sdk.ImageBuffer(8, 4)
    image.allocate()
    image.setPlaneBytes(bytes([255, 255, 255, 255]) * 32)
    return image


class MovieWriterTests(unittest.TestCase):
    def test_single_frame_movie_is_finalized_at_exact_path(self):
        runtime = sdk.Runtime()
        image = sdk.ImageBuffer(128, 64)
        image.setPixelLayout(sdk.ImageBuffer.PixelLayout.BGRA)
        image.allocate()
        image.setPlaneBytes(bytes([8, 16, 240, 255]) * (128 * 64))
        with tempfile.TemporaryDirectory() as directory:
            path = str(Path(directory) / "movie.mp4")
            result = sdk.writeImage(runtime, image, sdk.File(path))
            self.assertEqual(result.filePath(), path)
            self.assertTrue(result.exists())
            media = sdk.Media(runtime)
            self.assertTrue(media.open(result, timeoutMs=10000))
            self.assertEqual(media.timeRange().duration().frames(), 1)
            self.assertTrue(media.read().isValid())
            self.assertEqual(media.image().dataWindow().width(), 128)
            self.assertFalse(media.read().isValid())
            media.close()
            with self.assertRaises(RuntimeError):
                sdk.writeImage(runtime, image, result)


class MediaTests(unittest.TestCase):
    def test_lifecycle_roundtrip_thread_and_runtime_ownership(self):
        runtime = sdk.Runtime()
        second_runtime = sdk.Runtime()
        media = sdk.Media(runtime)
        del runtime
        gc.collect()
        self.assertFalse(media.isOpen())
        self.assertFalse(media.error())
        with self.assertRaises(RuntimeError):
            media.read()
        self.assertFalse(media.open(sdk.File("missing.unsupported_flipman_extension"), options={"exposure": 1.0}))
        with self.assertRaises(TypeError):
            media.open(sdk.File("missing.braw"), options={"exposure": object()})
        self.assertTrue(media.error())
        with tempfile.TemporaryDirectory() as directory:
            file = sdk.File(str(Path(directory) / "roundtrip.PNG"))
            file = sdk.writeImage(second_runtime, fixture_image(), file)
            self.assertTrue(file.exists())
            self.assertTrue(media.open(file, timeoutMs=1000))
            self.assertTrue(media.isOpen())
            self.assertTrue(media.read().isValid())
            image = media.image()
            self.assertEqual(image.dataWindow().width(), 8)
            self.assertEqual(image.dataWindow().height(), 4)
            self.assertEqual(image.planeBytes()[0], 255)
            failures = []
            def worker():
                try:
                    media.read()
                except RuntimeError as error:
                    failures.append(str(error))
            thread = threading.Thread(target=worker)
            thread.start()
            thread.join(timeout=5)
            self.assertFalse(thread.is_alive())
            self.assertEqual(len(failures), 1)
            self.assertTrue(media.close())
            self.assertFalse(media.isOpen())
            self.assertTrue(image.isAllocated())
            self.assertFalse(media.open(sdk.File("missing.unsupported_flipman_extension")))
            self.assertFalse(media.isValid())
        del media
        gc.collect()
        second_runtime.processEvents()


class RenderTests(unittest.TestCase):
    def test_ocio_gpu_pipeline(self):
        runtime = sdk.Runtime()
        self.assertIn("ACEScg", sdk.colorSpaces())
        profiles = {p["name"]: p for p in sdk.colorProfiles()}
        self.assertEqual(profiles["ARRI LogC3 (EI800)"]["gamut"], "ARRI Wide Gamut 3")
        self.assertEqual(profiles["ARRI LogC3 (EI800)"]["encoding"], "LogC3 (EI800)")
        self.assertEqual(profiles["Rec.1886 Rec.709 - Display"]["gamut"], "Rec.709")
        awg24 = sdk.combinedColorSpace("ARRI Wide Gamut 3", "Gamma 2.4")
        self.assertIn(awg24, sdk.workingSpaces())
        self.assertEqual(profiles[awg24]["gamut"], "ARRI Wide Gamut 3")
        with self.assertRaises((ValueError, RuntimeError)):
            sdk.combinedColorSpace("invented gamut", "Gamma 2.4")
        self.assertIn("ARRI LogC3 (EI800)", sdk.workingSpaces())
        self.assertEqual(sdk.colorSpaceFamily("ARRI LogC3 (EI800)"), "Input/ARRI")
        self.assertIn("Rec.1886 Rec.709 - Display", sdk.colorDisplays())
        self.assertIn("ACES 2.0 - SDR 100 nits (Rec.709)", sdk.colorViews("Rec.1886 Rec.709 - Display"))
        settings = sdk.ColorSettings()
        self.assertFalse(settings.enabled)
        self.assertEqual(settings.workingSpace, "ACEScg")
        settings.enabled = True
        image = fixture_image()
        result = sdk.renderImage(runtime, image, colorSettings=settings,
                                 inputColorSpace="sRGB - Display")
        values = struct.unpack("=" + "e" * (len(result.planeBytes()) // 2), result.planeBytes())
        self.assertTrue(all(math.isfinite(v) for v in values))
        self.assertAlmostEqual(values[0], 1.0, delta=0.01)
        settings.view = "ACES 2.0 - SDR 100 nits (Rec.709)"
        result = sdk.renderImage(runtime, image, colorSettings=settings, inputColorSpace="ACEScg")
        values = struct.unpack("=" + "e" * (len(result.planeBytes()) // 2), result.planeBytes())
        self.assertTrue(all(math.isfinite(v) for v in values))
        self.assertLess(values[0], 1.0)
        with self.assertRaises(RuntimeError):
            sdk.renderImage(runtime, image, colorSettings=settings, inputColorSpace="invalid-source")
        settings.workingSpace = "ARRI LogC3 (EI800)"
        result = sdk.renderImage(runtime, image, colorSettings=settings, inputColorSpace="ACEScg")
        self.assertTrue(result.isAllocated())
        settings.workingSpace = "invalid-working-space"
        with self.assertRaises(RuntimeError):
            sdk.renderImage(runtime, image, colorSettings=settings, inputColorSpace="ACEScg")

    def test_offscreen_image_and_profiler(self):
        runtime = sdk.Runtime()
        sdk.profile.setMode(sdk.profile.Mode.Aggregate)
        sdk.profile.reset()
        with tempfile.TemporaryDirectory() as directory:
            trace = str(Path(directory) / "render.trace")
            recording = sdk.profile.beginRecording("python_render", trace)
            self.assertNotEqual(recording, 0)
            try:
                image = sdk.renderImage(runtime, fixture_image(), width=16, height=8)
                self.assertTrue(image.isAllocated())
                self.assertEqual(image.dataWindow().width(), 16)
                self.assertEqual(image.dataWindow().height(), 8)
                self.assertFalse(image.requiresDecode())
                self.assertEqual(image.imageFormat().type(), sdk.ImageFormat.Type.Half)
                values = struct.unpack("=" + "e" * (len(image.planeBytes()) // 2), image.planeBytes())
                self.assertTrue(all(math.isfinite(value) for value in values))
                middle = ((8 // 2) * 16 + 16 // 2) * 4
                self.assertGreater(values[middle], 0.5)
                self.assertAlmostEqual(values[middle], values[middle + 1], delta=0.02)
                sdk.profile.counter("frames", 1)
            finally:
                sdk.profile.endRecording(recording)
            self.assertFalse(sdk.profile.recordingActive(recording))
            self.assertIn("render::python.renderImage", sdk.profile.summary())
            self.assertIn("COUNT | frames", Path(trace).read_text())
        sdk.profile.setMode(sdk.profile.Mode.Off)


suites = {"time": TimeTests, "core": CoreTests, "image": ImageTests, "media": MediaTests,
          "writer": MovieWriterTests, "render": RenderTests}
if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(suites[sys.argv[1]])
    sys.exit(not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful())
