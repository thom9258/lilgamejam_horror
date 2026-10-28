#include "fly_camera.hpp"

FlyCamera::FlyCamera()
    : m_camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      m_axis_x(SDLK_d, SDLK_a), m_axis_z(SDLK_s, SDLK_w), m_mouse() {}

auto FlyCamera::update(float deltatime) -> void {
  const auto rotate_speed = 0.045f;
  const auto move_speed = 5.0f;

  m_mouse.update();
  if (m_mouse.right_pressed()) {
    m_mouse.lock();
  }
  if (m_mouse.right_released()) {
    m_mouse.unlock();
  }

  if (m_mouse.locked()) {
    const auto mouse_delta = m_mouse.move_delta();
    const auto rotation = glm::vec2(static_cast<float>(mouse_delta.x),
                                    static_cast<float>(mouse_delta.y));
    m_camera.rotate(rotation * rotate_speed * deltatime);
  }

  m_axis_x.update();
  m_axis_z.update();
  const auto move_direction =
      glm::normalize(glm::vec3(m_axis_x.value(), 0.0f, m_axis_z.value()));

  m_camera.translate_local(move_direction * move_speed * deltatime);
}
