# SPDX-License-Identifier: BSD-3-Clause
"""A misconfigured Debug Qt launch must raise before Qt's fatal plugin loader."""
import sys
import flipman

if sys.argv[1] == "Debug":
    try:
        flipman.Runtime()
    except RuntimeError as error:
        assert "DYLD_IMAGE_SUFFIX" in str(error), str(error)
    else:
        raise AssertionError("missing Debug Qt runtime configuration was accepted")
else:
    runtime = flipman.Runtime()
    runtime.processEvents()
