#pragma once

#include <Core/EngineModule.hpp>

class RenderModule : public EngineModule
{
public:
    RenderModule();

    virtual void init() override;
    virtual void deinit() override;
    virtual bool needsToRun() const override;
    virtual tf::Task scedule(tf::Taskflow& flow) override;
};
