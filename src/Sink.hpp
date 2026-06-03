#pragma once
#include <halp/controls.hpp>
#include <halp/layout.hpp>
#include <halp/meta.hpp>
#include <halp/texture.hpp>

#include <cstdint>

// MyVideoSink is a video "sink": it has a texture input and *no* texture output.
// It consumes each incoming frame and, here, reduces it to a scalar (the average
// luminance), exposed on a plain value output.
//
// A scalar (non-texture) output does not make the object a filter: Avendish
// classifies sinks by the absence of a *texture* output. So this stays a sink.
//
// Backend notes: a sink is most meaningful for GStreamer (a real video sink
// element) and ossia score (a frame consumer producing an analysis value). The
// TouchDesigner TOP, Godot and Max/MSP texture back-ends always emit a texture,
// so for them the object still builds but degenerates to a pass-through-less
// node — keep your analysis logic here and read the value output downstream.
class MyVideoSink
{
public:
  halp_meta(name, "My Video Sink")
  halp_meta(c_name, "my_video_sink")
  halp_meta(category, "Analysis")
  halp_meta(author, "Avendish")
  halp_meta(description, "Consume an RGBA frame and output its average luminance")

  // CHANGE THIS !! (uuidgen)
  halp_meta(uuid, "f4e9c1a7-5d60-4b2c-9a3e-7c8d1b0f6e23")

  struct ins
  {
    // The incoming frame.
    halp::texture_input<"In"> image;
  } inputs;

  struct
  {
    // No texture output -> this is a sink. We do expose a scalar value output
    // carrying the measured average luminance in [0, 1].
    halp::val_port<"Luminance", float> luminance;
  } outputs;

  void operator()();

  struct ui;
};

inline void MyVideoSink::operator()()
{
  auto& in_tex = inputs.image.texture;

  // GPU readbacks are asynchronous: the input data may not be available yet.
  if(in_tex.bytes == nullptr)
    return;

  // Recompute only when the frame changes.
  if(!in_tex.changed)
    return;
  in_tex.changed = false;

  const int w = in_tex.width;
  const int h = in_tex.height;
  const int64_t count = int64_t(w) * h;
  if(count <= 0)
    return;

  // Rec. 601 luma, accumulated over every pixel.
  double sum = 0.;
  for(int y = 0; y < h; ++y)
  {
    for(int x = 0; x < w; ++x)
    {
      auto [r, g, b, a] = inputs.image.get(x, y);
      sum += 0.299 * r + 0.587 * g + 0.114 * b;
    }
  }

  outputs.luminance.value = float(sum / (count * 255.));
}

struct MyVideoSink::ui
{
  halp_meta(name, "Main")
  halp_meta(layout, halp::layouts::vbox)
  halp_meta(background, halp::colors::mid)

  halp::label header{"My Video Sink"};
};
