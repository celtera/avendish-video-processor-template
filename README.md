# Avendish video / texture template

This provides a basic, canonical template for making **video / texture** objects with
[Avendish](https://github.com/celtera/avendish): a single C++ object that processes RGBA
frames and is compiled, without rewrite, to several host plug-in formats.

It is the video counterpart of the
[audio processor template](https://github.com/celtera/avendish-audio-processor-template)
and the
[geometry processor template](https://github.com/celtera/avendish-geometry-processor-template):
where the audio one ticks per audio buffer and processes samples, this one ticks per
frame (in the graphics thread, at the output's frame rate) and processes textures.

## The three roles

Video objects come in three shapes, distinguished purely by their texture ports. This
template ships one canonical object for each:

| Object | File | Texture in | Texture out | Role |
|---|---|:--:|:--:|---|
| `MyVideoGenerator` | `src/Generator.hpp` | — | ✔ | **source** — an animated plasma |
| `MyVideoFilter` | `src/Filter.hpp` | ✔ | ✔ | **filter** — gain + invert |
| `MyVideoSink` | `src/Sink.hpp` | ✔ | — | **sink** — average-luminance meter |

A sink may still have non-texture outputs (the sink here emits a `float` luminance value);
what makes it a sink is the absence of a *texture* output.

## What gets built

The `avnd_addon_object(... BACKENDS ...)` calls in `CMakeLists.txt` instantiate the texture back-ends (and a Python module). The same file also builds as an ossia/score add-on via `find_package(Avendish)`.
For each of the three objects you get:

| Back-end | Object kind | SDK required |
|---|---|---|
| **TouchDesigner TOP** | Texture Operator | TouchDesigner Custom Operator SDK |
| **Godot** | `GDExtension` node exposing an `ImageTexture` | none — `godot-cpp` is fetched automatically |
| **Max/MSP** | Jitter matrix / texture object | Max SDK |
| **GStreamer** | `video/x-raw` element (source / transform / sink) | GStreamer development packages |
| **Python** | an importable extension module (`py<c_name>`) | pybind11 + Python dev headers |
| **ossia score** | texture process | libossia |

Back-ends whose SDK is not provided are silently skipped, so you can build just the ones
you need.

> **Note on sinks across back-ends:** a real "sink" (a frame consumer that produces no
> texture) is most meaningful for **GStreamer** (an actual video sink element) and **ossia
> score**. The TouchDesigner TOP, Godot and Max/MSP texture back-ends always emit a
> texture, so for them `MyVideoSink` still builds but its texture side is inert — read its
> `Luminance` value output downstream instead.

## The objects

- `src/Generator.hpp` — fills its `halp::texture_output` with an animated sum-of-sines
  plasma, with controllable size, speed and spatial scale.
- `src/Filter.hpp` — reads each pixel of its `halp::texture_input`, applies a gain and an
  optional inversion, and writes the result to its `halp::texture_output`.
- `src/Sink.hpp` — accumulates the Rec. 601 luminance of every pixel of its
  `halp::texture_input` and exposes the average on a `halp::val_port` value output.

Start from there: change the ports in the `inputs` struct and rewrite `operator()`. See
[`examples/Tutorial/TextureGeneratorExample.hpp`](https://github.com/celtera/avendish/blob/main/examples/Tutorial/TextureGeneratorExample.hpp)
and
[`examples/Tutorial/TextureFilterExample.hpp`](https://github.com/celtera/avendish/blob/main/examples/Tutorial/TextureFilterExample.hpp)
in Avendish for the reference, and `halp/texture.hpp` / `halp/texture_formats.hpp` for the
other ready-made pixel formats (`rgba32f`, `rgb`, single-channel `r8`/`r32f`, custom and
GPU textures).

> **CPU vs GPU:** these objects process pixels on the CPU. That is fine for algorithms that
> are awkward on the GPU (e.g. OpenCV / Dlib), but for plain per-pixel shading prefer a GPU
> shader (the ISF format that score supports) instead.

## Building

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

With no extra options this builds the **Godot** back-end (it fetches `godot-cpp`
automatically). To enable the other back-ends, pass the relevant SDK paths below.

To see a complete build procedure, refer to the
[Github actions workflows](.github/workflows/), which compile the project on clean virtual
machines.

### Godot

`godot-cpp` is fetched via CMake `FetchContent` by default. To build against an existing
checkout instead, pass:

```cmake
-DGODOT_CPP_PATH=path/to/godot-cpp
```

The build produces `build/godot/my_video_*_tex.{so,dll,dylib}`.

### TouchDesigner (TOP)

Point CMake at a TouchDesigner Custom Operator SDK whose `include/` directory contains the
operator base headers. Avendish reads `${TOUCHDESIGNER_SDK_PATH}/include/`:

```cmake
-DTOUCHDESIGNER_SDK_PATH=path/to/CustomOperatorSamples
```

This produces a TouchDesigner **TOP** per object (`build/td/MyVideo*_TOP_td.*`). The TOP
back-end only needs the public
[`TouchDesigner/CustomOperatorSamples`](https://github.com/TouchDesigner/CustomOperatorSamples)
headers (`CPlusPlus_Common.h` + `TOP_CPlusPlusBase.h`).

Note: TouchDesigner uses the MSVC ABI on Windows — build with clang-cl or MSVC, not MinGW.

### Max/MSP

Point CMake at a checkout of [`max-sdk-base`](https://github.com/jcelerier/max-sdk-base):

```cmake
-DAVND_MAXSDK_PATH=path/to/max-sdk-base
```

This produces a Jitter object per object (`build/max/my_video_*.{mxo,mxe64,so}`).

### GStreamer

Install the GStreamer development packages (so that `pkg-config` finds `gstreamer-1.0` and
friends); each object then builds as a `video/x-raw` element under `build/gstreamer/`
(a source for the generator, a transform for the filter, a sink for the sink).

### Python

Install [pybind11](https://github.com/pybind/pybind11) and the Python development headers,
then point CMake at pybind11's CMake package:

```bash
python -m pip install pybind11
cmake -S . -B build -G Ninja \
  -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
cmake --build build
```

This produces one extension module per object under `build/python/` (`pymy_video_*.so` /
`.pyd`), importable straight from Python:

```python
import pymy_video_filter
fx = pymy_video_filter.my_video_filter()
```

#### Testing through the Python back-end

`tests/test_python_bindings.py` is a worked example of unit-testing an Avendish object
from Python: it imports the compiled modules, sets parameters, ticks each object with
`process()`, and reads state back (e.g. the sink's `Luminance` output). When the Python
modules are built, CMake registers it as a CTest test, so:

```bash
ctest --test-dir build --output-on-failure
```

runs it against the freshly built modules with the matching interpreter. The CI runs this
on every platform. (A compiled extension only imports under the same Python it was built
with — the test is pointed at the right interpreter and module directory automatically.)

### ossia score

Install [libossia](https://github.com/ossia/libossia) (or build score and point CMake at
its 3rdparty tree) so that `find_package(ossia)` succeeds; the texture processes then
become available inside ossia score.

## TODO

* Support importing the template with a package manager
* GPU-texture (shader) variants
