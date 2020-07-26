#pragma once

#include <glm/mat4x4.hpp>

struct TransformComponent
{
    glm::mat4x4 m_transform = glm::mat4x4(1.0f);
};

struct MeshComponent
{};