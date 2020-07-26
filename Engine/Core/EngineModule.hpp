#pragma once

#include <string>
#include <Core/Types.hpp>
#include <taskflow/taskflow.hpp>

class EngineModule
{
public:
    EngineModule(const std::string& name)
        : m_name(name)
    {}
    virtual ~EngineModule() {}

    virtual void init() = 0;
    virtual void deinit() = 0;

    virtual bool needsToRun() const = 0;
    virtual tf::Task scedule(tf::Taskflow& flow) = 0;

    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};
