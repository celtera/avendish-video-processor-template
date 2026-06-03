cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(MyVideo CXX)

include(dependencies.cmake)

# This template ships three canonical video objects, one per role:
#
#   * MyVideoGenerator (src/Generator.hpp) - a source: texture output, no input
#   * MyVideoFilter    (src/Filter.hpp)    - a filter: texture input + output
#   * MyVideoSink       (src/Sink.hpp)      - a sink:   texture input, no output
#
# avnd_make_texture instantiates, for each object, the video / texture back-ends:
#   - ossia (for use inside ossia score)
#   - Max/MSP (a Jitter matrix / texture object)
#   - GStreamer (a video/x-raw element: source / transform / sink)
#   - TouchDesigner TOP (Texture Operator)
#   - Godot GDExtension (a Node exposing an ImageTexture)
#
# The relevant SDK paths must be passed to CMake for the optional back-ends to be
# built (see the README). Back-ends whose SDK is not found are silently skipped.
function(add_video_object target main_class c_name source impl)
  add_library(${target} STATIC "${impl}" "${source}")

  avnd_make_texture(
    TARGET ${target}
    MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    MAIN_CLASS ${main_class}
    C_NAME ${c_name}
  )

  # Workaround: when godot-cpp is pulled in via FetchContent, its only targets are
  # `godot-cpp` (+ alias `godot::cpp`) — there is no `godot-cpp::godot-cpp`. Avendish's
  # avnd_make_godot doesn't always resolve the target, so the generated headers under
  # <godot-cpp-build>/gen/include never make it onto the GDExtension's include path and
  # compilation fails with "godot_cpp/classes/node.hpp not found". Re-linking the target
  # here (at a scope where it is visible) propagates godot-cpp's PUBLIC include dirs.
  if(TARGET ${target}_TEXTURE_godot AND TARGET godot-cpp)
    target_link_libraries(${target}_TEXTURE_godot PRIVATE godot-cpp)
  endif()
endfunction()

add_video_object(MyVideoGenerator MyVideoGenerator my_video_generator src/Generator.hpp src/Generator.cpp)
add_video_object(MyVideoFilter    MyVideoFilter    my_video_filter    src/Filter.hpp    src/Filter.cpp)
add_video_object(MyVideoSink      MyVideoSink      my_video_sink      src/Sink.hpp      src/Sink.cpp)
