#pragma once
#include <halp/controls.hpp>
#include <halp/layout.hpp>
#include <halp/meta.hpp>
#include <halp/texture.hpp>

#include <algorithm>
#include <cstdint>

// MyVideoFilter is a video "filter": it has both a texture input and a texture
// output, transforming each incoming frame into an outgoing one.
//
// Same multi-backend story as the generator (TouchDesigner TOP, Godot, Max/MSP
// Jitter, GStreamer filter element, ossia score).
//
// See examples/Tutorial/TextureFilterExample.hpp in Avendish for the reference.
class MyVideoFilter
{
public:
  halp_meta(name, "My Video Filter")
  halp_meta(c_name, "my_video_filter")
  halp_meta(category, "Filter")
  halp_meta(author, "Avendish")
  halp_meta(description, "Adjust gain and optionally invert an RGBA frame")

  // CHANGE THIS !! (uuidgen)
  halp_meta(uuid, "1c7a5e64-9b2f-4d83-9f0a-6b7c4e2d8a52")

  struct ins
  {
    // The incoming frame.
    halp::texture_input<"In"> image;

    // Per-channel gain applied to every pixel.
    halp::knob_f32<"Gain", halp::range{.min = 0.f, .max = 4.f, .init = 1.f}> gain;

    // Invert the colors (negative image) when on.
    halp::toggle<"Invert"> invert;
  } inputs;

  struct
  {
    // The outgoing frame.
    halp::texture_output<"Out"> image;
  } outputs;

  MyVideoFilter() noexcept { outputs.image.create(1, 1); }

  void operator()();

  struct ui;
};

inline void MyVideoFilter::operator()()
{
  auto& in_tex = inputs.image.texture;

  // GPU readbacks are asynchronous: the input data may not be available yet.
  if(in_tex.bytes == nullptr)
    return;

  // Nothing to do if the input frame hasn't changed since last tick.
  if(!in_tex.changed)
    return;
  in_tex.changed = false;

  const int w = in_tex.width;
  const int h = in_tex.height;

  // Match the output size to the input.
  if(outputs.image.texture.width != w || outputs.image.texture.height != h)
    outputs.image.create(w, h);

  const float gain = inputs.gain;
  const bool invert = inputs.invert;

  const auto apply = [&](int c) -> int {
    int v = std::clamp(int(c * gain), 0, 255);
    return invert ? 255 - v : v;
  };

  for(int y = 0; y < h; ++y)
  {
    for(int x = 0; x < w; ++x)
    {
      auto [r, g, b, a] = inputs.image.get(x, y);
      outputs.image.set(x, y, apply(r), apply(g), apply(b), a);
    }
  }

  outputs.image.upload();
}

struct MyVideoFilter::ui
{
  halp_meta(name, "Main")
  halp_meta(layout, halp::layouts::vbox)
  halp_meta(background, halp::colors::mid)

  halp::label header{"My Video Filter"};

  struct
  {
    halp_meta(layout, halp::layouts::hbox)
    halp_meta(background, halp::colors::dark)

    halp::item<&MyVideoFilter::ins::gain> gain;
    halp::item<&MyVideoFilter::ins::invert> invert;
  } widgets;
};
