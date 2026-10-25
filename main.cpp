#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/input.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/sdl.hpp>

#include <print>

std::pair<std::vector<bubble::Vertex>, std::vector<bubble::Index>>
generateIndexedCube() {

  std::vector<bubble::Vertex> vertices = {
      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}};

  std::vector<bubble::Index> indices = {
      0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
      12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

  return {vertices, indices};
}

int main(int argc, char **argv) {
  static_cast<void>(argc);
  static_cast<void>(argv);

  bubble::Timer init_timer;
  init_timer.start();
  auto init_info = bubble::InitializeInfo{};
  init_info.height = 600;
  init_info.width = init_info.height * (4.0f / 3.0f);
  const auto ok = bubble::initialize(init_info);
  if (!ok) {
    return 1;
  }

  auto duration = init_timer.stop();
  if (duration.has_value()) {
    std::println("Initialized '{}' {}x{} in {}s", init_info.name,
                 init_info.width, init_info.height, duration.value().count());
  }

  auto [cube_vertices, cube_indices] = generateIndexedCube();
  auto cube_mesh = bubble::create_mesh(cube_vertices, cube_indices);
  if (!cube_mesh.has_value()) {
    return 1;
  }

  bubble::set_clearcolor(bubble::color::pleasant_blue);

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  const auto cube_location = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
  const auto cube_color = bubble::color::ugly_pink;

  auto error_bitmap = bubble::Bitmap(64, 64, bubble::Color(255, 255, 0, 255));
  for (std::size_t i = 0; i < 32; i++) {
    for (std::size_t j = 0; j < 32; j++) {
      *error_bitmap.at(i, j) = bubble::color::red;
    }
  }

  for (std::size_t i = 32; i < 64; i++) {
    for (std::size_t j = 32; j < 64; j++) {
      *error_bitmap.at(i, j) = bubble::color::red;
    }
  }

  const auto error_texture = bubble::create_texture(error_bitmap);

  auto button_focus = bubble::input::Button(SDLK_f);
  auto axis_x = bubble::input::Axis(SDLK_d, SDLK_a);
  auto axis_z = bubble::input::Axis(SDLK_s, SDLK_w);

  auto mouse = bubble::input::Mouse();

  auto should_run = true;
  while (should_run) {
    bubble::begin_frame();
    should_run = !bubble::requested_close();
    const auto deltatime = bubble::deltatime().value_or(0.16f);

    mouse.update();

    if (mouse.right_pressed()) {
      mouse.lock();
    }
    if (mouse.right_released()) {
      mouse.unlock();
    }
    if (mouse.locked()) {
      const auto mouse_delta = mouse.move_delta();
      const auto rotate_speed = 0.045f;
      const auto rotation = glm::vec2(static_cast<float>(mouse_delta.x),
                                      static_cast<float>(mouse_delta.y));
      camera.rotate(rotation * rotate_speed * deltatime);
    }

    button_focus.update();
    if (button_focus.pressed()) {
      camera.look_at(glm::vec3(0.0f));
    }

    axis_x.update();
    axis_z.update();
    const auto move_direction =
        glm::normalize(glm::vec3(axis_x.value(), 0.0f, axis_z.value()));
    if (glm::length(move_direction) > 0.1f) {
      const auto move_speed = 2.0f;
      camera.translate_local(move_direction * deltatime * move_speed);
    }

    bubble::set_camera(camera);
    bubble::draw_flatcolor(cube_location, *cube_mesh, cube_color);

    bubble::draw_textured(
        glm::translate(cube_location, glm::vec3(2.0f, 0.0f, 0.0f)), *cube_mesh,
        error_texture.value());

    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
