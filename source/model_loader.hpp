#pragma once

#include <bubble/bubble.hpp>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <expected>
#include <string>
#include <filesystem>

struct Material
{
    std::string name;
    std::optional<bubble::TextureRef> diffuse;
};

struct Mesh
{
    std::optional<bubble::MeshRef> mesh;
    std::string material_name;
};

struct Model
{
    std::string name;
    glm::mat4 transform;
    std::vector<Mesh> meshes;
    std::vector<Model> children;
};

struct Renderable
{
    std::optional<Model> root;
    std::vector<Material> materials;
};

auto load_renderable(std::filesystem::path path) -> std::expected<Renderable, std::string>;
