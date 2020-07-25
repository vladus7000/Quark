#include "RenderModule.hpp"

RenderModule::RenderModule()
    : EngineModule("RenderLogic")
{}

void RenderModule::init()
{
}

void RenderModule::deinit()
{
}

bool RenderModule::needsToRun() const
{
    return true;
}

tf::Task RenderModule::scedule(tf::Taskflow& flow)
{
    tf::Task ret = flow.emplace([]() {
        std::cout << "RenderLogic\n";
    });

    return ret;
}
