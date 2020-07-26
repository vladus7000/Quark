#include "Core/Engine.hpp"
#include "Core/Components.hpp"

void DoSomething()
{
    auto view = Quark::GetEngine().m_modules.view<Quark::GameplayComponent>();
    
    for (auto& entity : view)
    {
        auto& render = view.get<Quark::GameplayComponent>(entity);
        std::cout << render.m_module->getName() << std::endl;
    }
}