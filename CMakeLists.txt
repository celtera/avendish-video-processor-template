cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(MyVideo CXX)

# Use the Avendish the host already provides (e.g. ossia score); otherwise fetch it.
# The same CMakeLists builds these as ossia/score add-on objects or as standalone objects.
find_package(Avendish QUIET)
if(NOT Avendish_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    Avendish
    GIT_REPOSITORY "https://github.com/celtera/avendish"
    GIT_TAG 6d95ff0bbb1524b5846e4222f5dc610f9e8d0b7a)
  FetchContent_Populate(Avendish)
  list(APPEND CMAKE_PREFIX_PATH "${avendish_SOURCE_DIR}")
  find_package(Avendish REQUIRED)
endif()

avnd_addon_init(NAME MyVideo)

# This template ships three canonical video objects, one per role:
#   * MyVideoGenerator (src/Generator.hpp) - a source: texture output, no input
#   * MyVideoFilter    (src/Filter.hpp)    - a filter: texture input + output
#   * MyVideoSink      (src/Sink.hpp)      - a sink:   texture input, no output
#
# BACKENDS lists the texture back-ends (ossia, Max/MSP Jitter, GStreamer video, TD TOP,
# Godot) plus a Python extension module. In a score build the back-end list is ignored
# and only the ossia process is produced. Back-ends whose SDK is absent are skipped.
function(add_video_object main_class c_name source impl)
  avnd_addon_object(
    BASE MyVideo
    C_NAME ${c_name}
    CLASS ${main_class}
    BACKENDS ossia max gstreamer:TEXTURE touchdesigner:TOP godot:TEXTURE python
    MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    SOURCES "${impl}" "${source}")

  # avnd_make_python builds the module as a plain shared library, so it inherits the
  # platform-default name (lib-prefixed .dylib on macOS, .dll on Windows). CPython only
  # imports an extension module named "<module>.so" (Unix) / "<module>.pyd" (Windows)
  # with no "lib" prefix, so force that here (no-op in a score build: no _python target).
  if(TARGET ${c_name}_python)
    set_target_properties(${c_name}_python PROPERTIES PREFIX "")
    if(WIN32)
      set_target_properties(${c_name}_python PROPERTIES SUFFIX ".pyd")
    else()
      set_target_properties(${c_name}_python PROPERTIES SUFFIX ".so")
    endif()
  endif()
endfunction()

add_video_object(MyVideoGenerator my_video_generator src/Generator.hpp src/Generator.cpp)
add_video_object(MyVideoFilter    my_video_filter    src/Filter.hpp    src/Filter.cpp)
add_video_object(MyVideoSink      my_video_sink      src/Sink.hpp      src/Sink.cpp)

avnd_addon_finalize(NAME MyVideo UUID 7ecd8671-33ca-4362-a514-1076c579c1c2 VERSION 1.0.0)

# Unit tests exercising the Python back-end (tests/test_python_bindings.py). Registered
# only when the Python modules were actually built (pybind11 + Python dev headers found).
# Run with: ctest --test-dir <build> --output-on-failure
find_package(Python COMPONENTS Interpreter QUIET)
if(TARGET my_video_generator_python AND Python_EXECUTABLE)
  enable_testing()

  add_test(
    NAME python_bindings
    COMMAND "${Python_EXECUTABLE}" -m unittest discover
            -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p "test_*.py" -v
  )

  set_tests_properties(python_bindings PROPERTIES
    ENVIRONMENT "AVND_PYTHON_MODULE_DIR=$<TARGET_FILE_DIR:my_video_generator_python>"
  )
endif()
