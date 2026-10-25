#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/input.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/sdl.hpp>

#include "source/model_loader.hpp"

#include <print>

static auto error_texture = std::optional<bubble::TextureRef>{};

auto find_material(std::string_view name, std::span<Material> materials)
    -> Material * {

  for (auto &material : materials) {
    if (material.name == name) {
      return &material;
    }
  }
  return nullptr;
}

auto draw_model(Model &model, glm::mat4 parent_transform,
                std::span<Material> materials) -> void {
  const auto transform = model.transform * parent_transform;
  for (auto &mesh : model.meshes) {
    if (!mesh.mesh.has_value()) {
      continue;
    }

    const auto *material = find_material(mesh.material_name, materials);
    if (material == nullptr) {
      bubble::draw_textured(transform, mesh.mesh.value(),
                            error_texture.value());
    } else {
      const auto texture = material->diffuse.value_or(error_texture.value());
      bubble::draw_textured(transform, mesh.mesh.value(), texture);
    }
  }

  for (auto &child : model.children) {
    draw_model(child, transform, materials);
  }
}

auto draw_renderable(Renderable &renderable, glm::mat4 transform) -> void {
  if (renderable.root.has_value()) {
    draw_model(renderable.root.value(), transform, renderable.materials);
  }
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

  auto fox = load_renderable("/home/th/Assets/Fox/glTF/Fox.gltf");
  if (!fox.has_value()) {
    std::println("model load error: '{}'", fox.error());
    return 1;
  }

  std::println("fox textures: {}", fox->materials.size());

  bubble::set_clearcolor(bubble::color::pleasant_blue);

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f));

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

  error_texture = bubble::create_texture(error_bitmap);

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

    draw_renderable(fox.value(), glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)));

    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
