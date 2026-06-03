"""Unit tests for the Avendish Python back-end of this template.

`avnd_make_python` compiles each object (Generator / Filter / Sink) into an importable
CPython extension module (`pymy_video_*`). These tests import those modules and exercise
the object the way a host would: set parameters, tick it with `process()`, and read back
state. They are a worked example of testing an Avendish object purely from Python.

How the modules are found:
  - In a CMake/CTest run, the `python_bindings` test sets AVND_PYTHON_MODULE_DIR to the
    directory holding the freshly built modules (see CMakeLists.txt).
  - Run standalone with e.g.
        AVND_PYTHON_MODULE_DIR=build/python python -m unittest discover tests -v
    or just point PYTHONPATH at the build's python/ directory.

IMPORTANT: a compiled extension only imports under the *same* Python that built it
(matching ABI). CMake runs this with the interpreter Avendish linked against.
"""

import os
import sys
import unittest

# Make the compiled modules importable.
_module_dir = os.environ.get("AVND_PYTHON_MODULE_DIR")
if _module_dir:
    sys.path.insert(0, _module_dir)
else:
    # Fallback for a manual run from the repo root.
    _fallback = os.path.join(os.path.dirname(__file__), "..", "build", "python")
    if os.path.isdir(_fallback):
        sys.path.insert(0, _fallback)

import pymy_video_generator
import pymy_video_filter
import pymy_video_sink


class TestGenerator(unittest.TestCase):
    """The generator exposes its four value inputs and a process() tick."""

    def test_parameters_round_trip(self):
        gen = pymy_video_generator.my_video_generator()

        gen.Width = 320
        gen.Height = 240
        self.assertEqual(gen.Width, 320)
        self.assertEqual(gen.Height, 240)
        self.assertIsInstance(gen.Width, int)

        # 0.25 / 4.0 are exactly representable as float32, so equality is safe.
        gen.Speed = 0.25
        gen.Scale = 4.0
        self.assertAlmostEqual(gen.Speed, 0.25, places=6)
        self.assertAlmostEqual(gen.Scale, 4.0, places=6)
        self.assertIsInstance(gen.Speed, float)

    def test_process_runs(self):
        gen = pymy_video_generator.my_video_generator()
        gen.Width = 16
        gen.Height = 8
        # A generator produces a frame on every tick; ticking repeatedly must not raise
        # (and must tolerate the internal phase advancing).
        for _ in range(10):
            self.assertIsNone(gen.process())


class TestFilter(unittest.TestCase):
    """The filter exposes its Gain / Invert controls."""

    def test_parameters_round_trip(self):
        flt = pymy_video_filter.my_video_filter()

        flt.Gain = 2.0
        self.assertAlmostEqual(flt.Gain, 2.0, places=6)

        flt.Invert = True
        self.assertIs(flt.Invert, True)
        flt.Invert = False
        self.assertIs(flt.Invert, False)

    def test_process_without_input_is_safe(self):
        flt = pymy_video_filter.my_video_filter()
        # No input frame has been fed (the texture is empty / not yet uploaded), so the
        # filter must early-out gracefully rather than crash.
        self.assertIsNone(flt.process())


class TestSink(unittest.TestCase):
    """The sink exposes its measured Luminance value output."""

    def test_luminance_default(self):
        snk = pymy_video_sink.my_video_sink()
        self.assertIsInstance(snk.Luminance, float)
        # Nothing consumed yet.
        self.assertEqual(snk.Luminance, 0.0)

    def test_process_without_input_is_safe(self):
        snk = pymy_video_sink.my_video_sink()
        self.assertIsNone(snk.process())
        # With no input frame, the measured luminance stays at its default.
        self.assertEqual(snk.Luminance, 0.0)


if __name__ == "__main__":
    unittest.main()
