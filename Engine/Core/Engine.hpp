#pragma once

#include "Scene.hpp"
#include <entt/entt.hpp>

namespace Quark
{
    struct Engine
    {
        Scene m_scene;
        entt::registry m_modules;
    };

    Engine& GetEngine();
}