#include "model_loader.hpp"
#include "bubble/bubble.hpp"

#include <assimp/material.h>
#include <filesystem>

#include "thirdparty/stb_image.h"

#include <print>

namespace util {

auto get_first_texture_path(aiTextureType type, aiMaterial *material)
    -> std::expected<std::filesystem::path, std::string> {
  if (material == nullptr) {
    return std::unexpected("Invalid material");
  }

  const auto texture_index{0};
  auto aipathstring = aiString{};
  material->GetTexture(type, texture_index, &aipathstring);
  const auto pathstring = std::string(aipathstring.C_Str());
  if (pathstring == "") {
    return std::unexpected("Invalid diffuse path");
  }

  if (pathstring.starts_with("*")) {
    return std::unexpected("Inline textures are not supported");
  }

  return std::filesystem::path(pathstring);
}
auto find_material(std::string_view target, std::span<Material> materials)
    -> Material * {
  for (auto &material : materials) {
    if (material.name == target)
      return &material;
  }

  return nullptr;
}

auto to_glm(aiMatrix4x4 from) -> glm::mat4 {
  glm::mat4 to;
  to[0][0] = from.a1;
  to[1][0] = from.a2;
  to[2][0] = from.a3;
  to[3][0] = from.a4;
  to[0][1] = from.b1;
  to[1][1] = from.b2;
  to[2][1] = from.b3;
  to[3][1] = from.b4;
  to[0][2] = from.c1;
  to[1][2] = from.c2;
  to[2][2] = from.c3;
  to[3][2] = from.c4;
  to[0][3] = from.d1;
  to[1][3] = from.d2;
  to[2][3] = from.d3;
  to[3][3] = from.d4;
  return to;
}

auto to_glm(aiVector3f other) -> glm::vec3 {
  glm::vec3 v;
  v[0] = other.x;
  v[1] = other.y;
  v[2] = other.z;
  return v;
}

auto to_glm(aiQuaternion other) -> glm::quat {
  glm::quat q;
  q.w = other.w;
  q.x = other.x;
  q.y = other.y;
  q.z = other.z;
  return q;
}

} // namespace util

auto load_mesh(const aiScene *aiscene, aiMesh *aimesh)
    -> std::expected<Mesh, std::string> {
  auto mesh = Mesh{};

  auto vertices = std::vector<bubble::Vertex>{};
  vertices.reserve(aimesh->mNumVertices);
  for (unsigned int i = 0; i < aimesh->mNumVertices; i++) {
    auto vertex = bubble::Vertex{};
    vertex.position[0] = aimesh->mVertices[i].x;
    vertex.position[1] = aimesh->mVertices[i].y;
    vertex.position[2] = aimesh->mVertices[i].z;

    // Note we are guaranteed to have normals through our process flags
    vertex.normal[0] = aimesh->mNormals[i].x;
    vertex.normal[1] = aimesh->mNormals[i].y;
    vertex.normal[2] = aimesh->mNormals[i].z;

    if (aimesh->mTextureCoords[0] != nullptr) {
      vertex.texture_coordinate[0] = aimesh->mTextureCoords[0][i].x;
      vertex.texture_coordinate[1] = aimesh->mTextureCoords[0][i].y;
    } else {
      vertex.texture_coordinate[0] = 0.0f;
      vertex.texture_coordinate[1] = 0.0f;
    }

    vertices.push_back(vertex);
  }

  // NOTE we triangulate so we expect 3 indices per face
  auto indices = std::vector<bubble::Index>{};
  for (unsigned int i = 0; i < aimesh->mNumFaces; i++) {
    const auto face = aimesh->mFaces[i];
    for (std::size_t j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  mesh.mesh = bubble::create_mesh(vertices, indices);

  aiMaterial *material = aiscene->mMaterials[aimesh->mMaterialIndex];
  if (material != nullptr) {
    mesh.material_name = std::string(material->GetName().C_Str());
  }

  return mesh;
}

auto load_material(std::filesystem::path basedir, aiMaterial *aimaterial)
    -> std::expected<Material, std::string> {

  auto diffuse_path =
      util::get_first_texture_path(aiTextureType_DIFFUSE, aimaterial);

  if (!diffuse_path.has_value()) {
    return std::unexpected(diffuse_path.error());
  }

  auto material = Material{};
  material.name = aimaterial->GetName().C_Str();
  const auto path = basedir / diffuse_path.value();

  stbi_set_flip_vertically_on_load(false);

  int width, height, channels;
  auto *pixels =
      stbi_load(path.string().c_str(), &width, &height, &channels, 0);

  auto bitmap = bubble::Bitmap(width, height);
  if (channels == 4) {
    std::memcpy(bitmap.pixels(), pixels, width * height * channels);
  } else if (channels == 3) {
      for (std::size_t i = 0; i < width * height; i++) {
          const auto r = pixels[i*3+0]; 
          const auto g = pixels[i*3+1]; 
          const auto b = pixels[i*3+2]; 
          bitmap.pixels()[i] = bubble::Color{r, g, b, 255};
      }
  } else {
    return std::unexpected(std::format("invalid channels: {}", channels));
  }

  material.diffuse = bubble::create_texture(bitmap);
  return material;
}

auto load_model(const aiScene *aiscene, aiNode *ainode,
                std::filesystem::path basedir, std::vector<Material> &materials)
    -> std::expected<Model, std::string> {
  auto model = Model{};
  model.transform = util::to_glm(ainode->mTransformation);
  model.name = std::string(ainode->mName.C_Str());

  for (unsigned int i = 0; i < ainode->mNumMeshes; i++) {
    aiMesh *aimesh = aiscene->mMeshes[ainode->mMeshes[i]];

    auto loaded_mesh = load_mesh(aiscene, aimesh);

    if (loaded_mesh.has_value()) {
      model.meshes.push_back(loaded_mesh.value());
    }

    auto *aimaterial = aiscene->mMaterials[aimesh->mMaterialIndex];
    if (aimaterial != nullptr) {
      const auto material_name = std::string(aimaterial->GetName().C_Str());
      const auto *existing = util::find_material(material_name, materials);

      if (existing == nullptr) {
        std::println("material did not exist {}", material_name);
        auto loaded_material = load_material(basedir, aimaterial);
        if (loaded_material.has_value()) {
          materials.push_back(std::move(*loaded_material));
        } else {
          return std::unexpected(loaded_material.error());
        }
      }
    }
  }

  for (unsigned int i = 0; i < ainode->mNumChildren; i++) {
    auto child = load_model(aiscene, ainode->mChildren[i], basedir, materials);
    if (child.has_value()) {
      model.children.push_back(std::move(child.value()));
    } else {
      return std::unexpected(child.error());
    }
  }

  return model;
}

auto load_renderable(std::filesystem::path path)
    -> std::expected<Renderable, std::string> {
  const auto flags = aiProcess_Triangulate | aiProcess_GenNormals |
                     aiProcess_FlipUVs | aiProcess_FixInfacingNormals |
                     aiProcess_LimitBoneWeights;

  auto importer = Assimp::Importer{};
  const auto *aiscene = importer.ReadFile(path, flags);
  if (!aiscene) {
    return std::unexpected(
        std::format("Importer had no scene '{}'", importer.GetErrorString()));
  }

  if (aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
    return std::unexpected(
        std::format("Importer invalid scene '{}'", importer.GetErrorString()));
  }

  if (!aiscene->mRootNode) {
    return std::unexpected(
        std::format("Importer Missing Root '{}'", importer.GetErrorString()));
  }

  auto renderable = Renderable{};

  const auto basedir = path.parent_path();
  auto root =
      load_model(aiscene, aiscene->mRootNode, basedir, renderable.materials);
  if (root.has_value()) {
    renderable.root = root.value();
  } else {
    return std::unexpected(root.error());
  }

  return renderable;
}
