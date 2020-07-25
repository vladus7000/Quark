#pragma once

#include <Core/EngineModule.hpp>

class GameplayModule : public EngineModule
{
public:
    GameplayModule();

    virtual void init() override;
    virtual void deinit() override;
    virtual bool needsToRun() const override;
    virtual tf::Task scedule(tf::Taskflow& flow) override;
};
