#pragma once

#include "EngineModule.hpp"
#include "Gameplay/GameplayModule.hpp"
#include "Renderer/RenderModule.hpp"

namespace Quark
{
    struct ModuleComponent
    {
        ModuleComponent(EngineModule* module)
            : m_module(module)
        {
        }
        EngineModule* m_module;
    };

    struct GameplayComponent : ModuleComponent
    {
    };

    struct RenderComponent : ModuleComponent
    {
    };
}