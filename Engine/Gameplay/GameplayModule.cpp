#include "GameplayModule.hpp"

GameplayModule::GameplayModule()
    : EngineModule("GameplayLogic")
{}

void GameplayModule::init()
{
}

void GameplayModule::deinit()
{
}

bool GameplayModule::needsToRun() const
{
    return true;
}

tf::Task GameplayModule::scedule(tf::Taskflow& flow)
{
    tf::Task ret = flow.emplace([]() {
        std::cout << "GameplayLogic\n";
    });

    return ret;
}
