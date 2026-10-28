#pragma once

#include <bubble/bubble.hpp>
#include <bubble/input.hpp>

class FlyCamera {
  FlyCamera();

  auto update(float deltatime) -> void;

private:
  bubble::Camera m_camera;
  bubble::input::Axis m_axis_x;
  bubble::input::Axis m_axis_z;
  bubble::input::Mouse m_mouse;
};
