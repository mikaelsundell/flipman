# SPDX-License-Identifier: BSD-3-Clause
"""BRAW through the public Python Media API; external fixture supplied by CTest."""
from pathlib import Path
import os
import sys
import flipman as sdk

path = Path(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1] else None
if path is None or not path.is_file():
    print("BRAW fixture missing; configure FLIPMAN_BRAW_TEST_FILE")
    sys.exit(1 if os.environ.get("FLIPMAN_REQUIRE_BRAW_DATA") else 77)
runtime = sdk.Runtime()
media = sdk.Media(runtime)
assert media.isSupported("braw")
assert media.open(sdk.File(str(path)), options={"kelvin": 5600, "tint": -10, "exposure": 1.0}), media.error().message()
time = media.read()
assert time.isValid() and time.frames() == 0, media.error().message()
image = media.image()
assert image.isAllocated() and image.channels() == 3
assert image.imageFormat().type() == sdk.ImageFormat.Type.Float
assert media.close()
assert not media.open(sdk.File(str(path)), options={"tint": 99999})
assert media.error().hasError()
assert not media.open(sdk.File(str(path)), options={"apply3dlut": True})
assert media.error().hasError()
