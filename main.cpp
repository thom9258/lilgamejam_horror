#include <bubble/bitmap.hpp>
#include <bubble/bubble.hpp>
#include <bubble/input.hpp>
#include <bubble/timer.hpp>

#include <bubble/internal/glm.hpp>
#include <bubble/internal/sdl.hpp>

#include "source/character_mover.hpp"
#include "source/model_loader.hpp"

#include <box3d/box3d.h>

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

  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, -10.0f, 0.0f};
  b3WorldId worldId = b3CreateWorld(&worldDef);

  b3BodyDef groundBodyDef = b3DefaultBodyDef();
  groundBodyDef.position = (b3Vec3){0.0f, -10.0f, 0.0f};

  b3BoxHull groundBox = b3MakeBoxHull(50.0f, 10.0f, 50.0f);

  b3BodyId groundId = b3CreateBody(worldId, &groundBodyDef);
  b3ShapeDef groundShapeDef = b3DefaultShapeDef();
  b3CreateHullShape(groundId, &groundShapeDef, &groundBox.base);

  // b3BodyDef bodyDef = b3DefaultBodyDef();
  // bodyDef.type = b3_dynamicBody;
  // bodyDef.position = (b3Vec3){0.0f, 5.0f, 0.0f};
  // b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);
  // b3BoxHull dynamicBox = b3MakeCubeHull(1.0f);
  // b3ShapeDef shapeDef = b3DefaultShapeDef();
  // shapeDef.density = 1.0f;
  // shapeDef.baseMaterial.friction = 0.3f;
  // b3CreateHullShape(bodyId, &shapeDef, &dynamicBox.base);

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

  auto spaceship =
      load_renderable("/home/th/Projects/lilgamejam_horror/resources/models/"
                      "spaceship_interior_export/spaceship_interior.gltf");
  if (!spaceship.has_value()) {
    std::println("model load error: '{}'", spaceship.error());
    return 1;
  }

  bubble::set_clearcolor(bubble::color::pleasant_blue);

  auto camera =
      bubble::Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  auto character_mover = CharacterMover(worldId, b3Pos(0.0f, 1.0f, 0.0f));

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

    float timeStep = 1.0f / 60.0f;
    int subStepCount = 4;
    b3World_Step(worldId, timeStep, subStepCount);

    bool clipVelocity = false;
    b3Vec2 throttle = b3Vec2(move_direction.z, move_direction.x);
    b3Vec3 forward =
        b3Vec3(camera.forward().x, camera.forward().y, camera.forward().z);
    b3Vec3 right = b3Vec3(camera.right().x, camera.right().y, camera.right().z);
    character_mover.Step(nullptr, 0, deltatime, clipVelocity, throttle, forward, right);

    b3Vec3 position = character_mover.m_transform.p;
    camera.set_position(glm::vec3(position.x, position.y, position.z));

    bubble::set_camera(camera);

    draw_renderable(spaceship.value(),
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)));

    bubble::end_frame();
  }

  bubble::destroy();
  return 0;
}
