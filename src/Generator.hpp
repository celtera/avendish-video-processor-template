#pragma once
#include <halp/controls.hpp>
#include <halp/layout.hpp>
#include <halp/meta.hpp>
#include <halp/texture.hpp>

#include <cmath>
#include <cstdint>
#include <numbers>

// MyVideoGenerator is a video "generator" (a source): it has *no* texture input
// and produces one RGBA frame on its texture output every time it is ticked.
//
// The exact same object compiles, without rewrite, to:
//   - a TouchDesigner TOP (Texture Operator)
//   - a Godot GDExtension node exposing an ImageTexture
//   - a Max/MSP Jitter object (a matrix / texture output)
//   - a GStreamer source element (caps: video/x-raw, RGBA)
//   - an ossia score texture process
//
// Because textures are involved, the object runs in the graphics thread at the
// output's frame rate (e.g. 60 Hz), not in the audio thread. No timing info is
// available, so we animate from an internal frame counter.
//
// See examples/Tutorial/TextureGeneratorExample.hpp in Avendish for the reference.
class MyVideoGenerator
{
public:
  halp_meta(name, "My Video Generator")
  halp_meta(c_name, "my_video_generator")
  halp_meta(category, "Generator")
  halp_meta(author, "Avendish")
  halp_meta(description, "Generate an animated RGBA plasma")

  // CHANGE THIS !!
  // - On linux: uuidgen | xargs printf | xclip -selection clipboard
  //   will copy one on the clipboard
  // - uuidgen exists on Mac and Windows too
  halp_meta(uuid, "8d3b1f02-3a64-4f2e-8b1d-2f8e6c0a9d11")

  struct ins
  {
    // Output frame size. The texture is (re)allocated when either changes.
    halp::spinbox_i32<"Width", halp::range{.min = 1, .max = 4096, .init = 480}> width;
    halp::spinbox_i32<"Height", halp::range{.min = 1, .max = 4096, .init = 270}> height;

    // Animation speed (radians of phase advanced per frame).
    halp::hslider_f32<"Speed", halp::range{.min = 0.f, .max = 1.f, .init = 0.05f}> speed;

    // Spatial frequency of the plasma pattern.
    halp::hslider_f32<"Scale", halp::range{.min = 0.5f, .max = 32.f, .init = 8.f}> scale;
  } inputs;

  struct
  {
    // A texture output. Avendish recognizes the object as a generator because it
    // has a texture output and no texture input.
    halp::texture_output<"Out"> image;
  } outputs;

  MyVideoGenerator() noexcept { outputs.image.create(inputs.width, inputs.height); }

  // Recompute one frame. Defined inline (see Generator.cpp for why).
  void operator()();

  struct ui;

private:
  float phase = 0.f;
};

inline void MyVideoGenerator::operator()()
{
  const int w = inputs.width;
  const int h = inputs.height;

  // Reallocate the output texture when the requested size changes.
  if(outputs.image.texture.width != w || outputs.image.texture.height != h)
    outputs.image.create(w, h);

  const float scale = inputs.scale;
  const float t = phase;
  constexpr float tau = 2.f * std::numbers::pi_v<float>;

  for(int y = 0; y < h; ++y)
  {
    const float v = h > 1 ? float(y) / float(h - 1) : 0.f;
    for(int x = 0; x < w; ++x)
    {
      const float u = w > 1 ? float(x) / float(w - 1) : 0.f;

      // A cheap sum-of-sines plasma.
      const float a = std::sin((u * scale + t) * tau);
      const float b = std::sin((v * scale - t) * tau);
      const float c = std::sin(((u + v) * scale * 0.5f + t) * tau);

      const auto chan = [](float s) -> int {
        return int((s * 0.5f + 0.5f) * 255.f);
      };

      outputs.image.set(x, y, chan(a), chan(b), chan(c), 255);
    }
  }

  outputs.image.upload();

  phase += inputs.speed;
  if(phase > 1e6f)
    phase = 0.f;
}

struct MyVideoGenerator::ui
{
  halp_meta(name, "Main")
  halp_meta(layout, halp::layouts::vbox)
  halp_meta(background, halp::colors::mid)

  halp::label header{"My Video Generator"};

  struct
  {
    halp_meta(layout, halp::layouts::hbox)
    halp_meta(background, halp::colors::dark)

    // Field names avoid width/height/x/y/scale: score's layout walker treats
    // members with those names as geometry and tries to read them as qreal.
    // The control each item binds to is the pointer, not the field name.
    halp::item<&MyVideoGenerator::ins::width> width_item;
    halp::item<&MyVideoGenerator::ins::height> height_item;
    halp::item<&MyVideoGenerator::ins::speed> speed;
    halp::item<&MyVideoGenerator::ins::scale> scale_item;
  } widgets;
};
