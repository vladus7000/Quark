#pragma once

#include <Core/EngineModule.hpp>

struct GLFWwindow;
class RenderModule : public EngineModule
{
public:
    RenderModule(GLFWwindow* window);

    virtual void init() override;
    virtual void deinit() override;
    virtual bool needsToRun() const override;
    virtual tf::Task scedule(tf::Taskflow& flow) override;
private:
    GLFWwindow* m_window;
};
